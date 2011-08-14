#include "aio_wrapper.h"
#include "dispatcher.h"

#include <tr1/functional>
#include <aio.h>
#include <cstring>
#include <cerrno>

#include <stdio.h>

using nyu_libeventdisp::Dispatcher;
using std::tr1::bind;
using std::tr1::function;

namespace {
using nyu_libeventdisp::IOCallback;
using nyu_libeventdisp::UnitTask;

// Default offset for read & write
const off_t DEFAULT_OFFSET = 0;

// Convenience container for holding IOCallback and aiocb pointers.
struct AIOSigHandlerInfo {
  IOCallback* const callback;
  aiocb* const aioCB;

  // Creates a new container. The ownership of the aioCB and callback are both
  // passed to this object.
  AIOSigHandlerInfo(aiocb *aioCB, IOCallback *callback)
      : callback(callback), aioCB(aioCB) {
  }

  ~AIOSigHandlerInfo() {
    if (callback != NULL) {
      delete callback;
    }

    delete aioCB;
  }
};

// Callback function for completion of asynchronous read/write.
void aioDone(sigval_t signal) {

  AIOSigHandlerInfo* origInfo =
      reinterpret_cast<AIOSigHandlerInfo*>(signal.sival_ptr);
  aiocb *aioCB = origInfo->aioCB;
  IOCallback *callback = origInfo->callback;
  
  int status = aio_error(aioCB);
  if (status == 0) {
    if (callback != NULL && callback->okCB != NULL) {
      ssize_t ioResult = aio_return(aioCB);
    
      Dispatcher::instance()->enqueue(
          new UnitTask(bind(*callback->okCB, aioCB->aio_fildes,
                            const_cast<void *>(aioCB->aio_buf),
                            ioResult), callback->id));
    }
  }
  else if (callback != NULL && callback->errCB != NULL) {
    Dispatcher::instance()->enqueue(
        new UnitTask(bind(*callback->errCB, aioCB->aio_fildes,
                          status), callback->id));
  }

  delete origInfo;
}

void aio_done(int signo, siginfo_t *info, void *context) {

  // printf("signo %d\n", signo);

  if (info->si_signo == SIGIO) {   
     AIOSigHandlerInfo* origInfo =
       reinterpret_cast<AIOSigHandlerInfo*>(info->si_value.sival_ptr);

     aiocb *aioCB = origInfo->aioCB;
     IOCallback *callback = origInfo->callback;
     
     int status = aio_error(aioCB);
     if (status == 0) {
       if (callback != NULL && callback->okCB != NULL) {
	 ssize_t ioResult = aio_return(aioCB);
	 Dispatcher::instance()->enqueue
	   (new UnitTask(bind(*callback->okCB, aioCB->aio_fildes,
			      const_cast<void *>(aioCB->aio_buf),
			      ioResult), callback->id));
       }
     }
     else if (callback != NULL && callback->errCB != NULL) {
       Dispatcher::instance()->enqueue
	 (new UnitTask(bind(*callback->errCB, 
			    aioCB->aio_fildes,
			    status), 
		       callback->id));
     }
     delete origInfo;
  }
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
        bind(checkIOProgress, aioCB, callback), callback->id));
  }
  else {
    if (callback != NULL) {
      if (status == 0) {
        if (callback->okCB != NULL) {
          ssize_t ioResult = aio_return(aioCB);
          Dispatcher::instance()->enqueue(
              new UnitTask(bind(*callback->okCB, aioCB->aio_fildes,
                                const_cast<void *>(aioCB->aio_buf),
                                ioResult), callback->id));
        }
      }
      else if (callback->errCB != NULL) {
        Dispatcher::instance()->enqueue(
            new UnitTask(bind(*callback->errCB, aioCB->aio_fildes,
                              status), callback->id));
      }

      delete callback;
    }
    
    delete aioCB;
  }
}

int aioSkeletonFunction(int (*aio_func)(aiocb *), int fd, void *buff,
                        size_t len, off_t offset, IOCallback *callback) {


  aiocb *aioCB = new aiocb();

  memset(reinterpret_cast<char *>(aioCB), 0, sizeof(aiocb));
  aioCB->aio_buf = buff;
  aioCB->aio_fildes = fd;
  aioCB->aio_nbytes = len;
  aioCB->aio_offset = offset;

#ifdef LIBEVENT_USE_SIG

  struct sigaction sig_act;
  
  // handler
  sigemptyset(&sig_act.sa_mask);
  sig_act.sa_flags = SA_SIGINFO;
  sig_act.sa_sigaction = aio_done;

  // singal
  aioCB->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
  aioCB->aio_sigevent.sigev_signo = SIGIO;
  aioCB->aio_sigevent.sigev_value.sival_ptr =
    static_cast<void *>(new AIOSigHandlerInfo(aioCB, callback));
  
  // aioCB->aio_sigevent.sigev_notify = SIGEV_THREAD;
  // aioCB->aio_sigevent.sigev_notify_function = aioDone;
  // aioCB->aio_sigevent.sigev_notify_attributes = NULL;
  
  // register
  sigaction(SIGIO, &sig_act, NULL); 

#endif
  
  int ret = aio_func(aioCB);

  if (ret >= 0) {
#ifndef LIBEVENT_USE_SIG
    checkIOProgress(aioCB, callback);
#endif
  }
  else {
    if (callback != NULL) {
      if (callback->errCB != NULL) {
        Dispatcher::instance()->enqueue(
            new UnitTask(bind(*callback->errCB, aioCB->aio_fildes,
                              errno), callback->id));
      }

      delete callback;
    }

#ifdef LIBEVENT_USE_SIG
    delete reinterpret_cast<AIOSigHandlerInfo *>(
        aioCB->aio_sigevent.sigev_value.sival_ptr);
#endif
    
    delete aioCB;
  }

  return ret;
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
  return aioSkeletonFunction(::aio_read, fd, buff, len, offset, callback);
}

int aio_write(int fd, void *buff, size_t len, off_t offset,
              IOCallback *callback) {
  return aioSkeletonFunction(::aio_write, fd, buff, len, offset, callback);
}
}

