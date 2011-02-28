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

namespace nyu_libeventdisp {
// This file contains wrapper functions for the linux asynchronous I/O with
// integration with the event dispatcher.

typedef std::tr1::function<void (int, volatile void*, ssize_t)> IOCallback;
typedef std::tr1::function<void (int, int)> IOErrCallback;

// Reads contents of a file descriptor given the maximum length.
//
// Params:
//  fd - the file descriptor
//  buff - the buffer to contain the data. Caller is responsible for making sure
//    that buff is still valid (not deallocated) when the callback is called.
//    The caller is also responsible for deallocating buff.
//  len - the maximum length in bytes to read.
//  callback - the function to call when the read is completed successfully.
//    Params:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the data read.
//      3rd arg - the number of bytes read.
//  errorCallback - the function to call when the read encountered an error
//    Params:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the error code. Please refer to aio_error for more
//        details
//
// Returns 0 if read operation is successfully queued. Returns -1 otherwise and
// the errno variable will be set appropriately.
int aio_read(int fd, void *buff, size_t len, IOCallback callback);
int aio_read(int fd, void *buff, size_t len, IOCallback callback,
             IOErrCallback errorCallback);

// Writes contents of the buffer to the given file descriptor.
//
// Params:
//  fd - the file descriptor
//  buff - the buffer to contain the data. Caller is responsible for making sure
//    that buff is still valid (not deallocated) until the write succeeds. The
//    caller is also responsible for deallocating buff.
//  len - the length in bytes to write.
//  callback - the function to call when the write is completed successfully.
//    Params:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the data used to write.
//      3rd arg - the number of bytes written.
//  errorCallback - the function to call when the read encountered an error
//    Params:
//      1st arg - contains the file descriptor passed to this function.
//      2nd arg - contains the error code. Please refer to aio_error for more
//        details.
//
// Returns 0 if write operation is successfully queued. Returns -1 otherwise and
// the errno variable will be set appropriately.
int aio_write(int fd, void *buff, size_t len, IOCallback callback);
int aio_write(int fd, void *buff, size_t len, IOCallback callback,
              IOErrCallback errorCallback);
}

#endif

