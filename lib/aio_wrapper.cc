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
using nyu_libeventdisp::IOErrCallback;

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
void checkIOProgress(aiocb *aioCB, IOCallback *callback,
                     IOErrCallback *errorCallback) {
  int status = aio_error(aioCB);
  
  if (status == EINPROGRESS) {
    Dispatcher::instance()->enqueue(bind(checkIOProgress, aioCB,
                                         callback, errorCallback));
  }
  else {
    ssize_t ioResult = aio_return(aioCB);
    
    if (status == 0 && ioResult > 0) {
      (*callback)(aioCB->aio_fildes, aioCB->aio_buf, ioResult);
    }
    else {
      (*errorCallback)(aioCB->aio_fildes, status);
    }

    delete callback;
    delete errorCallback;
    delete aioCB;
  }
}

// The entry point of all aio_read wrapper functions
int aio_read_(int fd, void *buff, off_t offset, size_t len, IOCallback *callback,
              IOErrCallback *errorCallback) {
  aiocb *aioCB = new aiocb();

  memset(reinterpret_cast<char *>(aioCB), 0, sizeof(aiocb));
  aioCB->aio_buf = buff;
  aioCB->aio_fildes = fd;
  aioCB->aio_nbytes = len;
  aioCB->aio_offset = offset;
  
  int ret = aio_read(aioCB);

  if (ret >= 0) {
    checkIOProgress(aioCB, callback, errorCallback);
  }
  else {
    delete callback;
    delete errorCallback;
    delete aioCB;
  }

  return ret;
}

// The entry point of all aio_write wrapper functions
int aio_write_(int fd, void *buff, off_t offset, size_t len, IOCallback *callback,
               IOErrCallback *errorCallback) {
  aiocb *aioCB = new aiocb();

  memset(reinterpret_cast<char *>(aioCB), 0, sizeof(aiocb));
  aioCB->aio_buf = buff;
  aioCB->aio_fildes = fd;
  aioCB->aio_nbytes = len;
  aioCB->aio_offset = offset;
  
  int ret = aio_write(aioCB);

  if (ret >= 0) {
    checkIOProgress(aioCB, callback, errorCallback);
  }
  else {
    delete callback;
    delete errorCallback;
    delete aioCB;
  }

  return ret;
}

} //namespace

namespace nyu_libeventdisp {
int aio_read(int fd, void *buff, size_t len, IOCallback callback) {
  return aio_read_(fd, buff, DEFAULT_OFFSET, len,
                   new function<void (int, volatile void*, ssize_t)>(callback),
                   new function<void (int, int)>(dummyErrorCallback));
}

int aio_read(int fd, void *buff, size_t len, IOCallback callback,
             IOErrCallback errorCallback) {
  return aio_read_(fd, buff, DEFAULT_OFFSET, len,
                   new function<void (int, volatile void*, ssize_t)>(callback),
                   new function<void (int, int)>(errorCallback));
}

int aio_write(int fd, void *buff, size_t len, IOCallback callback) {
  return aio_write_(fd, buff, DEFAULT_OFFSET, len,
                    new function<void (int, volatile void*, ssize_t)>(callback),
                    new function<void (int, int)>(dummyErrorCallback));
}

int aio_write(int fd, void *buff, size_t len, IOCallback callback,
              IOErrCallback errorCallback) {
  return aio_write_(fd, buff, DEFAULT_OFFSET, len,
                    new function<void (int, volatile void*, ssize_t)>(callback),
                    new function<void (int, int)>(errorCallback));
}
}

