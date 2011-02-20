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

// Source file used with permission from Alberto Lerner.

#ifndef MCP_BASE_THREAD_HEAD
#define MCP_BASE_THREAD_HEAD

#include <pthread.h>
#include <tr1/functional>

namespace base {

// makeThread creates a new thread and returns its thread ID. The
// thread will run 'body', which can be any function or object method
// with no arguments and no return. 
//
//
// Usage:
//   #include <tr1/functional>
//   using std::tr1::bind;
//
//   class Server {
//   public:
//     Server(...);
//     void doTask();
//   ...
//   };
//
//   Server my_server(...);
//
//   // Let's run my_server.doTask() on a separate thread, go do something else,
//   // and then wait for that thread to finish
//   pthread_t my_thread = makeThread(bind(&Server::doTask, &my_server);
//   ... go do something else ...
//   pthread_join(my_thread, NULL);
//
typedef std::tr1::function<void()> ThreadBody;
pthread_t makeThread(ThreadBody body);

}  // namespace base

#endif // MCP_BASE_THREAD_HEAD
