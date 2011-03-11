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

#ifndef LIBEVENTDISP_DISPATCHER_H_
#define LIBEVENTDISP_DISPATCHER_H_

#include "event_dispatcher_interface.h"
#include "util.h"

namespace nyu_libeventdisp {
// Singleton class for the event dispatcher. The init method should be called
// first before any operation.
class Dispatcher {
 public:
  // Initializes the singleton object.
  //
  // Params:
  //  concurrentDispatchers - The number of concurrent dispatchers to create.
  //    This value is ignored if multiProcessorMode is false.
  //  multiProcessorMode - set to true to run multiple threads of event
  //    dispatchers.
  //
  // Note: This is not thread-safe
  static void init(size_t concurrentDispatchers, bool multiProcessorMode);
  
  // Initializes the singleton object for a single threaded dispatcher.
  //
  // Note: This is not thread-safe
  static void init(void);
  
  // Gets the singleton instance
  static Dispatcher* instance();

  // Enqueues a new task to the queue. Ownership of newTask is passed to this
  // object.
  //
  // Returns true if successful
  bool enqueue(UnitTask *newTask);

  // Enqueues a new task to the queue with the given id. id is ignored when not
  // in multiprocessor mode.
  //
  // Returns true if successful
  bool enqueue(const UnitTask &newTask, const TaskGroupID &id);
  
 private:
  static Dispatcher *instance_;
  EventDispatcherInterface *eventDispatcher_;
  
  Dispatcher(size_t concurrentDispatchers, bool multiprocessorMode);
  DISALLOW_COPY_AND_ASSIGN(Dispatcher);
};
}

#endif

