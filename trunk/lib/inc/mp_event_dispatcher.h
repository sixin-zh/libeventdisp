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

#ifndef LIBEVENTDISP_MP_EVENT_DISPATCHER_H_
#define LIBEVENTDISP_MP_EVENT_DISPATCHER_H_

#include <tr1/functional>
#include <vector>

#include "event_dispatcher_interface.h"
#include "util.h"

namespace nyu_libeventdisp {
// A simple event dispatcher that can run tasks concurrently. Tasks with the
// same TaskGroupID are guaranteed with sequential consistency.
class MPEventDispatcher : public EventDispatcherInterface {
 public:
  // Creates a new dispatcher that can have a maximum of concurrently running
  // tasks specified by concurrentTaskCount.
  explicit MPEventDispatcher(size_t concurrentTaskCount);
  
  // Destroys this object. All remaining tasks in the queue are discarded.
  virtual ~MPEventDispatcher();

  // Adds a new task to the event dispatcher.
  //
  // Returns true if successful
  bool enqueueTask(UnitTask *newTask);

  void stop(void);
  void resume(void);
  
 private:
  std::vector<EventDispatcherInterface*> dispatchers_;
  
  DISALLOW_COPY_AND_ASSIGN(MPEventDispatcher);
};
}

#endif

