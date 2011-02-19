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

#ifndef LIBEVENTDISP_EVENT_DISPATCHER_INTERFACE_H_
#define LIBEVENTDISP_EVENT_DISPATCHER_INTERFACE_H_

namespace nyu_libeventdisp {
class EventDispatcherInterface {
  // Adds a new task to the event dispatcher. The ownership of the task is also
  // transferred to this object.
  //
  // Returns true if successful
  virtual bool enqueueTask(std::tr1::function<void()> *newTask) = 0;

  // Finishes all remaining tasks and stops accepting new task. Any new tasks
  // that is going to be enqueued will be discarded.
  virtual void stop(void) = 0;

  // Resumes this dispatcher from a stopped state. Does nothing if the
  // dispatcher is not stopped.
  virtual void resume(void) = 0;
};
}

#endif
