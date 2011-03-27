#include "delayed_task.h"
#include "dispatcher.h"
#include <time.h>
#include <cassert>
#include <cstring>

namespace nyu_libeventdisp {
using base::ScopedLock;

DelayedTask::DelayedTask(UnitTask *newTask, const timespec &delay) :
    task_(newTask), hasCompleted_(false) {
  sigevent sigEv;
  sigEv.sigev_notify = SIGEV_THREAD;
  sigEv.sigev_value.sival_ptr = this;
  sigEv.sigev_notify_function = DelayedTask::timeoutHandler;
  sigEv.sigev_notify_attributes = NULL;

  assert(timer_create(ITIMER_REAL, &sigEv, &id_) == 0);
  
  timespec interval;
  memset(&interval, 0, sizeof(interval));

  itimerspec timerSpec;
  timerSpec.it_interval = interval;
  timerSpec.it_value = delay;
  
  assert(timer_settime(id_, 0, &timerSpec, NULL) == 0);
}

DelayedTask::~DelayedTask() {
  ScopedLock sl(&mutex_);

  if (!hasCompleted_) {
    timer_delete(id_);
    delete task_;
    hasCompleted_ = true;
  }
}

void DelayedTask::resetTimer(const timespec &newDelay) {
  timespec interval;
  memset(&interval, 0, sizeof(interval));

  itimerspec timerSpec;
  timerSpec.it_interval = interval;
  timerSpec.it_value = newDelay;
  
  assert(timer_settime(id_, 0, &timerSpec, NULL) == 0);
}

void DelayedTask::timeoutHandler(sigval_t val) {
  DelayedTask *delayedTask = reinterpret_cast<DelayedTask *>(val.sival_ptr);
  delayedTask->timeoutHandler();
}

void DelayedTask::timeoutHandler(void) {
  ScopedLock sl(&mutex_);

  if (!hasCompleted_) {
    Dispatcher::instance()->enqueue(task_);
    timer_delete(id_);
    hasCompleted_ = true;
  }
}

}

