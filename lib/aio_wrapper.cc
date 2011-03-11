#include "aio_wrapper.h"
#include "dispatcher.h"

#include <tr1/functional>
#include <aio.h>
#include <cstring>
#include <cerrno>

using nyu_libeventdisp::Dispatcher;
using std::tr1::bind;
using std::tr1::function;

namespace {
using nyu_libeventdisp::IOCallback;
using nyu_libeventdisp::UnitTask;

// Default offset for read & write
const off_t DEFAULT_OFFSET = 0;

// Serves as a dummy callback that does nothing and conforms with the signature
// for IOErrCallback.
void dummyErrorCallback(int fd, int errorCode) {
  // Do nothing
}

// Checks the progress of the queued asynchronous I/O job and calls callback
// upon successful completion or errorCallback upon failure.
//
// Params:
//  aioCB - this should contain the necessary info regarding the asynchronous
//    I/O job.
//  callback - this function will be called upon successful completion.
//  errorCallback - this function will be called upon failure.
void checkIOProgress(aiocb *aioCB, IOCallback *callback) {
  int status = aio_error(aioCB);
  
  if (status == EINPROGRESS) {
    Dispatcher::instance()->enqueue(new UnitTask(
        bind(checkIOProgress, aioCB, callback)));
  }
  else {
    ssize_t ioResult = aio_return(aioCB);
    
    if (status == 0 && ioResult > 0) {
      (*callback->okCB)(aioCB->aio_fildes, aioCB->aio_buf, ioResult);
    }
    else if (callback->errCB != NULL) {
      (*callback->errCB)(aioCB->aio_fildes, status);
    }

    delete callback;
    delete aioCB;
  }
}
} //namespace

namespace nyu_libeventdisp {
IOCallback::IOCallback(IOOkCallback *ok, IOErrCallback *err)
    : id(UnitTask::DEFAULT_ID), okCB(ok), errCB(err) {
}

IOCallback::IOCallback(IOOkCallback *ok, IOErrCallback *err, TaskGroupID id)
    : id(id), okCB(ok), errCB(err) {
}

IOCallback::~IOCallback() {
  if (okCB != NULL) {
    delete okCB;
  }

  if (errCB != NULL) {
    delete errCB;
  }
}

int aio_read(int fd, void *buff, size_t len, off_t offset,
             IOCallback *callback) {
  aiocb *aioCB = new aiocb();

  memset(reinterpret_cast<char *>(aioCB), 0, sizeof(aiocb));
  aioCB->aio_buf = buff;
  aioCB->aio_fildes = fd;
  aioCB->aio_nbytes = len;
  aioCB->aio_offset = offset;
  
  int ret = aio_read(aioCB);

  if (ret >= 0) {
    checkIOProgress(aioCB, callback);
  }
  else {
    delete callback;
    delete aioCB;
  }

  return ret;
}

int aio_write(int fd, void *buff, size_t len, off_t offset,
              IOCallback *callback) {
  aiocb *aioCB = new aiocb();

  memset(reinterpret_cast<char *>(aioCB), 0, sizeof(aiocb));
  aioCB->aio_buf = buff;
  aioCB->aio_fildes = fd;
  aioCB->aio_nbytes = len;
  aioCB->aio_offset = offset;
  
  int ret = aio_write(aioCB);

  if (ret >= 0) {
    checkIOProgress(aioCB, callback);
  }
  else {
    delete callback;
    delete aioCB;
  }

  return ret;
}
}

