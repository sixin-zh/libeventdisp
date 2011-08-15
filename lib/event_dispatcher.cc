#include <sys/time.h>  // gettimeofday

#include "event_dispatcher.h"
#include "thread.h"

#include "pthread.h"
#include "signal.h"
#include "stdio.h"

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

  // printf("enqueue: %ld get queueMutex\n", (long int) pthread_self());
  ScopedLock scopedQueue(&queueMutex_);
  //printf("enqueue: %ld got queueMutex\n", (long int) pthread_self());

  // It's okay to accept the new task if the stop method was called beyond this
  // point. So no need to acquire stoppedMutex_.
  if (!isStopped_ && newTask != NULL) {
    taskQueue_.push(newTask);

    {
      //      printf("enqueue: %ld get taskCountMutex\n", (long int) pthread_self());
      ScopedLock ScopedTaskCount(&taskCountMutex_);
      //printf("enqueue: %ld got taskCountMutex\n", (long int) pthread_self());
      taskCount_[newTask->id]++;
    }
    //printf("enqueue: %ld release taskCountMutex\n", (long int) pthread_self());
    
    newTaskCond_.signal();
  }
  else if (newTask != NULL) {
    delete newTask;
  }
  
  //  printf("enqueue: %ld to release queuetMutex\n", (long int) pthread_self());
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


  // // block SIGIO to avoid deadlock
  // static sigset_t   signal_mask;  // signals to block
  // sigemptyset(&signal_mask);
  // sigaddset(&signal_mask, SIGIO);
  // //  sigaddset (&signal_mask, SIGTERM);
  // if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
  //   perror("can't mask thread");
  //   return;
  //   //exit(1);
  // }

  while (true) {
    UnitTask *nextTask;
  
    {
      //      printf("front: %ld get queueMutex\n", (long int) pthread_self());
      ScopedLock sl(&queueMutex_);
      //printf("front: %ld got queueMutex\n", (long int) pthread_self());
 
      while (taskQueue_.empty() && !isDying_) {
	// Use timed wait to break from waiting to allow destructor to proceed
	printf("event wait: %ld\n", (long int) pthread_self());
	newTaskCond_.timedWait(&queueMutex_, WAIT_TIMEOUT);
      }

      if (isDying_ && taskQueue_.empty()) {
	return;
      }
 
      nextTask = taskQueue_.front();
      isExecuting_ = true;
    }
    //    printf("front: %ld release queuetMutex\n", (long int) pthread_self());

    if (!isDying_) {
      // Note: There is a chance that nextTask will call enqueueTask
      //printf("executing: %ld\n", (long int) pthread_self());
      nextTask->task(); // ! NON-REENTRANT
    }

    {
      //printf("pop: %ld get queueMutex\n", (long int) pthread_self());
      ScopedLock sl(&queueMutex_);
      //printf("pop: %ld got queueMutex\n", (long int) pthread_self());

      taskQueue_.pop();
      isExecuting_ = false;

      //printf("pop: %ld get taskCountMutex\n", (long int) pthread_self());
      ScopedLock ScopedTaskCount(&taskCountMutex_);
      //printf("pop: %ld got taskCountMutex\n", (long int) pthread_self());
      taskCount_[nextTask->id]--;
    }
    //printf("pop: %ld release queuetMutex\n", (long int) pthread_self());
    //printf("pop: %ld release taskCountMutex\n", (long int) pthread_self());

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

size_t EventDispatcher::getSize(TaskGroupID id) const {
  return pendingTasks();
}

}

