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

namespace nyu_libeventdisp {
typedef std::tr1::function<void (int, void*, size_t)> IOCallback;
void aio_read(int fd, void *buff, size_t len, IOCallback callback);
void aio_write(int fd, void *buff, size_t len, IOCallback callback);
}

#endif

