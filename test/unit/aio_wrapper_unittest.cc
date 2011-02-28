#include "gtest/gtest.h"
#include "event.h"
#include "aio_wrapper.h"
#include "lock.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>

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
const long IO_TIMEOUT = 1; //sec

void okCallback(Mutex *mutex, ConditionVar *cond, int fd, volatile void *buff,
                ssize_t len) {
  ScopedLock sl(mutex);
  cond->signal();
}

void errCallback(Mutex *mutex, ConditionVar *cond, int fd, int err,
                 int *errOccured) {
  ScopedLock sl(mutex);
  *errOccured = 1;
  cond->signal();
}

} //namespace

TEST(AioWrapperTest, ReadTest) {
  int blockingInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(blockingInputFd, 0);

  char blockingBuff[BUFF_SIZE];
  ASSERT_GE(read(blockingInputFd, static_cast<void *>(blockingBuff),
                 BUFF_SIZE), 0);
  close(blockingInputFd);

  int aioInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(aioInputFd, 0);

  char aioBuff[BUFF_SIZE];
  Mutex mutex;
  ConditionVar readDoneCond;
  ASSERT_EQ(0, aio_read(aioInputFd, static_cast<void *>(aioBuff), BUFF_SIZE,
                        bind(okCallback, &mutex, &readDoneCond, _1, _2, _3)));

  {
    ScopedLock sl(&mutex);
    readDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }
  
  EXPECT_STREQ(blockingBuff, aioBuff);
}

TEST(AioWrapperTest, ReadErrTest) {
  int aioInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(aioInputFd, 0);

  char aioBuff[BUFF_SIZE];
  Mutex mutex;
  ConditionVar readDoneCond;
  int errOccured = 0;
  
  ASSERT_EQ(0, aio_read(aioInputFd, static_cast<void *>(aioBuff), BUFF_SIZE,
                        bind(okCallback, &mutex, &readDoneCond, _1, _2, _3),
                        bind(errCallback, &mutex, &readDoneCond,
                             _1, _2, &errOccured)));
  
  aio_cancel(aioInputFd, NULL);

  {
    ScopedLock sl(&mutex);
    readDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }
  
  EXPECT_EQ(1, errOccured);
}

