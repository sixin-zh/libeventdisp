#include <sys/time.h>  // gettimeofday

#include "event_dispatcher.h"
#include "thread.h"

namespace nyu_libeventdisp {

using std::tr1::function;
using std::tr1::bind;
using base::ScopedLock;
using base::makeThread;

EventDispatcher::EventDispatcher(void) :
    isExecuting(false), isStopped(false), isDying(false) {
  eventLoopThread_ = makeThread(bind(&EventDispatcher::eventLoop, this));
}

EventDispatcher::~EventDispatcher() {
  {
    ScopedLock sl(&stoppedMutex_);
    isStopped = true;
    isDying = true;
  }

  // Wait for the eventLoop to cleanly dispose of pending tasks.
  pthread_join(eventLoopThread_, NULL);
}

bool EventDispatcher::enqueueTask(const UnitTask &newTask) {
  ScopedLock sl(&queueMutex_);

  // It's okay to accept the new task if the stop method was called beyond this
  // point. So no need to acquire stoppedMutex_.
  if (!isStopped) {
    taskQueue_.push(new function<void ()>(newTask));
    newTaskCond_.signal();
  }
  
  return true;
}

bool EventDispatcher::enqueueTask(const UnitTask &newTask,
                                  const TaskGroupID &id) {
  return enqueueTask(newTask);
}

void EventDispatcher::stop(void) {
  ScopedLock sl(&stoppedMutex_);
  isStopped = true;
}

void EventDispatcher::resume(void) {
  ScopedLock sl(&stoppedMutex_);
  if (!isDying) {
    isStopped = false;
  }
}

bool EventDispatcher::busy(void) {
  ScopedLock sl(&queueMutex_);
  return !taskQueue_.empty();
}

// Main loop flow:
//
// 1. Get task from queue.
// 2. Execute the task.
// 3. Remove the task from queue.
// 4. Go back to #1.
//
// Invariant: isExecuting should be true between steps 1 and 4.
void EventDispatcher::eventLoop(void) {
  static const long WAIT_TIMEOUT = 1; //sec
  
  while (true) {
    UnitTask *nextTask;
    
    {
      ScopedLock sl(&queueMutex_);
      while (taskQueue_.empty() && !isDying) {
        // Use timed wait to break from waiting to allow destructor to proceed
        newTaskCond_.timedWait(&queueMutex_, WAIT_TIMEOUT);
      }

      if (isDying && taskQueue_.empty()) {
        return;
      }

      nextTask = taskQueue_.front();
      isExecuting = true;
    }

    if (nextTask != NULL) {
      if (!isDying) {
        // Note: There is a chance that nextTask will call enqueueTask
        (*nextTask)();
      }

      delete nextTask;
    }

    {
      ScopedLock sl(&queueMutex_);
      taskQueue_.pop();
      isExecuting = false;
    }
  }
}

}

