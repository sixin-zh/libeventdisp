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

// Source file used with permission from Alberto Lerner.

#ifndef MCP_BASE_LOCK_HEADER
#define MCP_BASE_LOCK_HEADER

#include <pthread.h>
#include <sys/time.h> // gettimeofday


#include <stdio.h>

// Convenient wrappers around
// + pthread_mutex
// + pthread_cond
// + pthread_rwlock
//
// And a mutex wrapper that locks at construction and unlocks at
// destruction. It can be used for "scope locking."

namespace base {

class Mutex {

public:
  Mutex()         { 
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_, &m_attr);    
    //m_ = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  }
  ~Mutex()        { pthread_mutex_destroy(&m_); }

  int lock()     { return pthread_mutex_lock(&m_); }
  int unlock()   { return pthread_mutex_unlock(&m_); }

private:

  friend class ConditionVar;

  pthread_mutex_t m_;

  // Non-copyable, non-assignable
  Mutex(Mutex &);
  Mutex& operator=(Mutex&);
};

class ScopedLock {
public:
  explicit ScopedLock(Mutex* lock) : m_(lock) { 
    m_->lock();
    //printf("%ld ready to lock %p\n", (long int) pthread_self(), m_);
    //printf("%ld lock[%p] status = %d\n", (long int) pthread_self(), m_, m_->lock());
  }
  ~ScopedLock()   { 
    m_->unlock();
    //    printf("%ld ready to unlock %p\n", (long int) pthread_self(), m_);
    //printf("%ld unlock[%p] status = %d\n", (long int) pthread_self(), m_, m_->unlock());
  }

private:
  Mutex* m_;

  // Non-copyable, non-assignable
  ScopedLock(ScopedLock&);
  ScopedLock& operator=(ScopedLock&);
};

class ConditionVar {
public:
  ConditionVar()          { pthread_cond_init(&cv_, NULL); }
  ~ConditionVar()         { pthread_cond_destroy(&cv_); }

  void wait(Mutex* mutex) { pthread_cond_wait(&cv_, &(mutex->m_)); }
  void signal()           { pthread_cond_signal(&cv_); }
  void signalAll()        { pthread_cond_broadcast(&cv_); }

  void timedWait(Mutex* mutex, const struct timespec* timeout) {
    pthread_cond_timedwait(&cv_, &(mutex->m_), timeout);
  }

  void timedWait(Mutex* mutex, long timeout) {
    timeval now;
    timespec wakeupTime;
    gettimeofday(&now, NULL);
    wakeupTime.tv_sec = now.tv_sec + timeout;
    wakeupTime.tv_nsec = 0;

    timedWait(mutex, &wakeupTime);
  }

private:
  pthread_cond_t cv_;

  // Non-copyable, non-assignable
  ConditionVar(ConditionVar&);
  ConditionVar& operator=(ConditionVar&);
};

class RWMutex {
public:
  RWMutex()      { pthread_rwlock_init(&rw_m_, NULL); }
  ~RWMutex()     { pthread_rwlock_destroy(&rw_m_); }

  void rLock()   { pthread_rwlock_rdlock(&rw_m_); }
  void wLock()   { pthread_rwlock_wrlock(&rw_m_); }
  void unlock()  { pthread_rwlock_unlock(&rw_m_); }

private:
  pthread_rwlock_t rw_m_;

  // Non-copyable, non-assignable
  RWMutex(RWMutex&);
  RWMutex& operator=(RWMutex&);

};

// A semaphore implementation that allows a negative intial value.
class Semaphore {
 public:
  Semaphore(int initVal) : val_(initVal) {
  }

  // Increments the semaphore value.
  // Warning: Can overflow if current value is max value for int
  void up(void) {
    ScopedLock sl(&mutex_);
    val_++;

    if (val_ > 0) {
      cond_.signal();
    }
  }

  // Tries to decrement the semaphore value but blocks until the value
  // becomes > 0.
  // Warning: Can overflow if current value is min value for int
  void down(void) {
    ScopedLock sl(&mutex_);
    
    while (val_ <= 0) {
      cond_.wait(&mutex_);
    }

    val_--;
  }
  
 private:
  int val_;
  Mutex mutex_;
  ConditionVar cond_;
};

} // namespace base

#endif  // MCP_BASE_LOCK_HEADER
