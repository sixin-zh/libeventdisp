#include <sys/time.h>  // gettimeofday

#include "event_dispatcher.h"
#include "thread.h"

namespace nyu_libeventdisp {

// Notes:
// Always follow this precedence when acquiring locks to avoid deadlock:
// queueMutex_ -> taskCountMutex_ -> stoppedMutex_

using std::tr1::function;
using std::tr1::bind;
using base::ScopedLock;
using base::makeThread;

EventDispatcher::EventDispatcher(void) :
    isExecuting_(false), isStopped_(false), isDying_(false) {
  eventLoopThread_ = makeThread(bind(&EventDispatcher::eventLoop, this));
}

EventDispatcher::~EventDispatcher() {
  {
    ScopedLock sl(&stoppedMutex_);
    isStopped_ = true;
    isDying_ = true;
  }

  // Wait for the eventLoop to cleanly dispose of pending tasks.
  pthread_join(eventLoopThread_, NULL);
}

bool EventDispatcher::enqueueTask(UnitTask *newTask) {
  ScopedLock scopedQueue(&queueMutex_);

  // It's okay to accept the new task if the stop method was called beyond this
  // point. So no need to acquire stoppedMutex_.
  if (!isStopped_ && newTask != NULL) {
    taskQueue_.push(newTask);

    {
      ScopedLock ScopedTaskCount(&taskCountMutex_);
      taskCount_[newTask->id]++;
    }
    
    newTaskCond_.signal();
  }
  else if (newTask != NULL) {
    delete newTask;
  }
  
  return true;
}

void EventDispatcher::stop(void) {
  ScopedLock sl(&stoppedMutex_);
  isStopped_ = true;
}

void EventDispatcher::resume(void) {

  ScopedLock sl(&stoppedMutex_);
  if (!isDying_) {
    isStopped_ = false;
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
// Invariant: isExecuting_ should be true between steps 1 and 4.
void EventDispatcher::eventLoop(void) {
  static const long WAIT_TIMEOUT = 1; //sec
  
  while (true) {
    UnitTask *nextTask;
    
    {
      ScopedLock sl(&queueMutex_);
      while (taskQueue_.empty() && !isDying_) {
        // Use timed wait to break from waiting to allow destructor to proceed
        newTaskCond_.timedWait(&queueMutex_, WAIT_TIMEOUT);
      }

      if (isDying_ && taskQueue_.empty()) {
        return;
      }

      nextTask = taskQueue_.front();
      isExecuting_ = true;
    }

    if (!isDying_) {
      // Note: There is a chance that nextTask will call enqueueTask
      nextTask->task();
    }

    {
      ScopedLock sl(&queueMutex_);
      taskQueue_.pop();
      isExecuting_ = false;

      ScopedLock ScopedTaskCount(&taskCountMutex_);
      taskCount_[nextTask->id]--;
    }

    delete nextTask;
  }
}

size_t EventDispatcher::pendingTasks(void) const {
  ScopedLock sl(&taskCountMutex_);
  
  size_t count = 0;
  for (TaskCountMap::const_iterator iter = taskCount_.begin();
       iter != taskCount_.end(); ++iter) {
    count += iter->second;
  }
  
  return count;
}
}

