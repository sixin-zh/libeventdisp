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

#ifndef LIBEVENTDISP_EVENT_DISPATCHER_H_
#define LIBEVENTDISP_EVENT_DISPATCHER_H_

#include <tr1/functional>
#include <vector>

#include "event_dispatcher_interface.h"
#include "util.h"

namespace nyu_libeventdisp {
// A simple event dispatcher that can handle one task at a time. The enqueued
// tasks are executed in FIFO order.
class EventDispatcher : public EventDispatcherInterface {
 public:
  // Creates a new event dispatcher object.
  EventDispatcher(void);
  
  // Destroys this object. All remaining tasks in the queue are discarded.
  virtual ~EventDispatcher();
  
  bool enqueueTask(const std::tr1::function<void()> &newTask);
  void stop(void);
  void resume(void);

 private:
  std::vector<std::tr1::function<void()> *> taskQueue_;
  
  DISALLOW_COPY_AND_ASSIGN(EventDispatcher);
};
}

#endif

