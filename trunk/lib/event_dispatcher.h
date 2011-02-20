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
#include <queue>

#include "event_dispatcher_interface.h"
#include "util.h"
#include "lock.h"

namespace nyu_libeventdisp {
// A simple event dispatcher that can handle one task at a time. The enqueued
// tasks are executed in FIFO order.
class EventDispatcher : public EventDispatcherInterface {
 public:
  // Creates a new event dispatcher object.
  EventDispatcher(void);
  
  // Destroys this object.
  //
  // Warning: All remaining tasks in the queue are discarded.
  // Mutex usage: stoppedMutex_
  virtual ~EventDispatcher();

  // Mutex usage: queueMutex_
  bool enqueueTask(const std::tr1::function<void()> &newTask);

  // Mutex usage: stoppedMutex_
  void stop(void);

  // Mutex usage: stoppedMutex_
  void resume(void);

  // Returns true if there there is still pending task.
  //
  // Mutex usage: queueMutex_
  bool busy(void);

 private:
  base::Mutex queueMutex_;
  base::Mutex stoppedMutex_;
  
  base::ConditionVar newTaskCond_;
  pthread_t eventLoopThread_;

  // Guarded by queueMutex_
  std::queue<std::tr1::function<void()> *> taskQueue_;

  // Guarded by stoppedMutex_
  bool isStopped;
  bool isDying;
  
  // Runs the event loop
  //
  // Mutex usage: queueMutex_
  void eventLoop(void);
  
  DISALLOW_COPY_AND_ASSIGN(EventDispatcher);
};
}

#endif

