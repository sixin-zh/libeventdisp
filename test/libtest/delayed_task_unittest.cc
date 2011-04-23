#include "gtest/gtest.h"
#include "delayed_task.h"
#include "unit_task.h"
#include "lock.h"

#include <time.h> //clock_gettime, link with -lrt
#include <tr1/functional>

using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::DelayedTask;
using base::Semaphore;
using std::tr1::bind;

namespace {
const float LOWER_TIME_DIFF_TOLERANCE = 0.9;
//const float UPPER_TIME_DIFF_TOLERANCE = 1.5; // very lenient upper bound

void increment(int *x, Semaphore *sem) {
  (*x)++;
  sem->up();
}

// returns time2 - time1
timespec timeDiff(const timespec &time1, const timespec &time2) {
  timespec timeDiff;
  timeDiff.tv_nsec = time2.tv_nsec - time1.tv_nsec;
  timeDiff.tv_sec = time2.tv_sec - time1.tv_sec;

  // Adjust timespec to borrow from seconds if needed
  if (time2.tv_nsec < time1.tv_nsec) {
    timeDiff.tv_sec--;
    timeDiff.tv_nsec += 1000000000;
  }

  return timeDiff;
}
} //namespace

TEST(DelayedTaskTest, DelayedEnqueue) {
  static const long NSEC_DELAY = 1000000; //1 msec

  timespec delay;
  delay.tv_sec = 0;
  delay.tv_nsec = NSEC_DELAY;

  timespec startTime;
  ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &startTime));

  int x = 0;
  Semaphore sem(0);
  UnitTask *task = new UnitTask(bind(increment, &x, &sem));
  DelayedTask delayedTask(task, delay);

  sem.down();
  
  timespec endTime;
  ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &endTime));
  timespec diff = timeDiff(startTime, endTime);

  EXPECT_EQ(1, x);
  EXPECT_LE(0, diff.tv_sec);
  EXPECT_GT(diff.tv_nsec, static_cast<long>(NSEC_DELAY *
                                            LOWER_TIME_DIFF_TOLERANCE));
  //EXPECT_LT(diff.tv_nsec, static_cast<long>(NSEC_DELAY *
  //                                          UPPER_TIME_DIFF_TOLERANCE));
}

TEST(DelayedTaskTest, ResetTimer) {
  static const long NSEC_DELAY = 1000000; //1 msec
  static const long NSEC_DELAY2 = NSEC_DELAY * 4; //3 msec

  timespec delay;
  delay.tv_sec = 0;
  delay.tv_nsec = NSEC_DELAY;

  timespec startTime;
  ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &startTime));

  int x = 0;
  Semaphore sem(0);
  UnitTask *task = new UnitTask(bind(increment, &x, &sem));
  DelayedTask delayedTask(task, delay);

  timespec delay2;
  delay2.tv_sec = 0;
  delay2.tv_nsec = NSEC_DELAY2;
  delayedTask.resetTimer(delay2);
  
  sem.down();
  
  timespec endTime;
  ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &endTime));
  timespec diff = timeDiff(startTime, endTime);

  EXPECT_EQ(1, x);
  EXPECT_LE(0, diff.tv_sec);
  EXPECT_GT(diff.tv_nsec, static_cast<long>(NSEC_DELAY2 *
                                            LOWER_TIME_DIFF_TOLERANCE));
  //EXPECT_LT(diff.tv_nsec, static_cast<long>(NSEC_DELAY2 *
  //                                          UPPER_TIME_DIFF_TOLERANCE));
}

