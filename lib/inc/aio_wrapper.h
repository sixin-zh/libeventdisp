// Copyright 2011 libeventdisp
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AIO_WRAPPER_H_
#define AIO_WRAPPER_H_

#include <tr1/functional>
#include <sys/types.h>
#include "unit_task.h"

namespace nyu_libeventdisp {
// This file contains wrapper functions for the linux asynchronous I/O with
// integration with the event dispatcher. Dispatcher::init() should be called
// first before calling any of the functions.

typedef std::tr1::function<void (int, void*, ssize_t, int)> IOOkCallback;
typedef std::tr1::function<void (int, int)> IOErrCallback;

// Container for storing callbacks
struct IOCallback {
  const TaskGroupID id;
  IOOkCallback *okCB;
  IOErrCallback *errCB;

  // Creates a new IOCallback object.
  //
  // Params:
  //  ok - pointer to a callback function to be called when I/O was performed
  //    successfully. Ownership is passed to this object.
  //  err - pointer to a callback function to be called when I/O failed.
  //    Ownership is passed to this object.
  //  id - the id for this callback. Callbacks with the same ids are guaranteed
  //    to be executed in a certain sequential order and not run concurrently.
  IOCallback(IOOkCallback *ok, IOErrCallback *err);
  IOCallback(IOOkCallback *ok, IOErrCallback *err, TaskGroupID id);
  
  ~IOCallback();
};

// Reads contents of a file descriptor given the maximum length.
//
// Params:
//  fd - the file descriptor
//  buff - the buffer to contain the data. Caller is responsible for making sure
//    that buff is still valid (not deallocated) when the callback is called.
//    The caller is also responsible for deallocating buff.
//  len - the maximum length in bytes to read.
//  offset - the offset to start reading.
//  callback - pointer to the callback function. Ownership is passed to this
//    method.
//    Params for okCB:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the data read.
//      3rd arg - the number of bytes read. Contains a value of -1 if the read
//        operation failed.
//      4th arg - Contains the error code. Corresponds to the errno after
//        performing a system call on read. Please refer to the man page of read
//        for more details. Valid only if 3rd arg is -1.
//    Params for errCB (can be NULL):
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the error code. Please refer to aio_error for more
//        details
//
// Returns 0 if read operation is successfully queued. Returns -1 otherwise and
// the errno variable will be set appropriately.
int aio_read(int fd, void *buff, size_t len, off_t offset,
             IOCallback *callback);

// Writes contents of the buffer to the given file descriptor.
//
// Params:
//  fd - the file descriptor
//  buff - the buffer to contain the data. Caller is responsible for making sure
//    that buff is still valid (not deallocated) until the write succeeds. The
//    caller is also responsible for deallocating buff.
//  len - the length in bytes to write.
//  offset - the offset to start writing.
//  callback - pointer to the callback function. Ownership is passed to this
//    method.
//    Params for okCB:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the data used to write.
//      3rd arg - the number of bytes written. Contains a value of -1 if the read
//        operation failed.
//      4th arg - Contains the error code. Corresponds to the errno after
//        performing a system call on read. Please refer to the man page of read
//        for more details. Valid only if 3rd arg is -1.
//    Params for errCB (if not NULL):
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the error code. Please refer to aio_error for more
//        details.
//
// Returns 0 if write operation is successfully queued. Returns -1 otherwise and
// the errno variable will be set appropriately.
int aio_write(int fd, void *buff, size_t len, off_t offset,
              IOCallback *callback);
}

#endif

