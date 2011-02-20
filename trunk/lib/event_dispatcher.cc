#include <sys/time.h>  // gettimeofday

#include "event_dispatcher.h"
#include "thread.h"

namespace nyu_libeventdisp {

using std::tr1::function;
using std::tr1::bind;
using std::queue;
using base::ScopedLock;
using base::makeThread;

EventDispatcher::EventDispatcher(void) : isStopped(false), isDying(false) {
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

bool EventDispatcher::enqueueTask(const function<void()> &newTask) {
  ScopedLock sl(&queueMutex_);

  // It's okay to accept the new task if the stop method was called beyond this
  // point. So no need to acquire stoppedMutex_.
  if (!isStopped) {
    taskQueue_.push(new function<void()>(newTask));
    newTaskCond_.signal();
  }
  
  return true;
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

void EventDispatcher::eventLoop(void) {
  static const long WAIT_TIMEOUT = 1; //sec
  
  while (true) {
    function<void()> *nextTask;
    
    {
      ScopedLock sl(&queueMutex_);
      while (taskQueue_.empty() && !isDying) {
        timeval now;
        timespec wakeupTime;
        gettimeofday(&now, NULL);
        wakeupTime.tv_sec = now.tv_sec + WAIT_TIMEOUT;
        wakeupTime.tv_nsec = 0;

        newTaskCond_.timedWait(&queueMutex_, &wakeupTime);
      }

      if (isDying && taskQueue_.empty()) {
        return;
      }

      nextTask = taskQueue_.front();
      taskQueue_.pop();
    }

    if (nextTask != NULL) {
      if (!isDying) {
        (*nextTask)();
      }

      delete nextTask;
    }
  }
}

}

