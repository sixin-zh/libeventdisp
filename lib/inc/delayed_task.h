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

#ifndef LIBEVENTDISP_DELAYED_TASK_H_
#define LIBEVENTDISP_DELAYED_TASK_H_

#include <signal.h>
#include "util.h"
#include "lock.h"

struct timespec;

namespace nyu_libeventdisp {
class UnitTask;

// A container class for delaying the execution of a given task. 
// Dispatcher::init() should be called before creating any instance of this
// class.
class DelayedTask {
 public:
  // Creates a delayed task that enqueues a task that will be executed at a
  // future time. Ownership of newTask is passed to this object.
  //
  // Params:
  //  newTask - the task to perform.
  //  delay - the amount of time relative to the current time to wait before
  //    executing the task. Note that the lower bound of the delay is
  //    guaranteed to be greater than delay but the upper bound is not strict
  DelayedTask(UnitTask *newTask, const timespec &delay);

  // Destroys this object. Also cancels the future task if not yet performed.
  ~DelayedTask();
  
  // Resets the timer to a new time relative to now.
  void resetTimer(const timespec &newDelay);
  
 private:
  timer_t id_;
  UnitTask *task_;
  bool hasCompleted_;
  base::Mutex mutex_;

  // Handler that is to be executed when the timeout is triggered.
  static void timeoutHandler(sigval_t val);
  void timeoutHandler(void);
  
  DISALLOW_COPY_AND_ASSIGN(DelayedTask);
};
}

#endif

