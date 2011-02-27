#include "gtest/gtest.h"
#include "event.h"
#include "aio_wrapper.h"
#include "lock.h"

#include <sys/stat.h>
#include <sys/time.h> // gettimeofday
#include <fcntl.h>

using nyu_libeventdisp::aio_read;
using nyu_libeventdisp::aio_write;
using base::Mutex;
using base::ScopedLock;
using base::ConditionVar;
using std::tr1::bind;
using namespace std::tr1::placeholders;

namespace {
using namespace std;

const char *READ_TEST_FILE = "read_test";
const char *WRITE_TEST_FILE = "write_test";

const size_t BUFF_SIZE = 1024;
const long IO_TIMEOUT = 3; //sec

void readCallback(Mutex *mutex, ConditionVar *cond, int fd, void *buff,
                  size_t len) {
  ScopedLock sl(mutex);
  cond->signal();
}
} //namespace

TEST(AioWrapperTest, ReadTest) {
  int blockingInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_TRUE(blockingInputFd > 0);

  char blockingBuff[BUFF_SIZE];
  ASSERT_TRUE(read(blockingInputFd, static_cast<void *>(blockingBuff),
                   BUFF_SIZE) >= 0);

  int aioInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_TRUE(aioInputFd > 0);

  char aioBuff[BUFF_SIZE];
  Mutex mutex;
  ConditionVar readDoneCond;
  aio_read(aioInputFd, static_cast<void *>(aioBuff), BUFF_SIZE,
           bind(readCallback, &mutex, &readDoneCond, _1, _2, _3));

  {
    ScopedLock sl(&mutex);
    timeval now;
    timespec wakeupTime;
    gettimeofday(&now, NULL);
    wakeupTime.tv_sec = now.tv_sec + IO_TIMEOUT;
    wakeupTime.tv_nsec = 0;

    readDoneCond.timedWait(&mutex, &wakeupTime);
  }
  
  EXPECT_STREQ(blockingBuff, aioBuff);
}

