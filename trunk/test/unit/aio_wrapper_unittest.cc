#include "gtest/gtest.h"
#include "event.h"
#include "aio_wrapper.h"
#include "lock.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>
#include <cstring>

using namespace nyu_libeventdisp;

using base::Mutex;
using base::ScopedLock;
using base::ConditionVar;

using std::tr1::bind;
using namespace std::tr1::placeholders;

namespace {
using namespace std;

const char *READ_TEST_FILE = "read_test";
const char *WRITE_TEST_FILE = "write_test";
const char *WRITE_TEST_BUFF = "This is a very simple test\r\n<>qwerty\n";

const size_t BUFF_SIZE = 1024;
const off_t OFFSET = 0;
const long IO_TIMEOUT = 1; //sec

void okCallback(Mutex *mutex, ConditionVar *cond, bool *wasCalled,
                int fd, void *buff, ssize_t len) {
  ScopedLock sl(mutex);
  *wasCalled = true;
  cond->signal();
}

void errCallback(Mutex *mutex, ConditionVar *cond, bool *wasCalled, int fd) {
  ScopedLock sl(mutex);
  *wasCalled = true;
  cond->signal();
}
} //namespace

TEST(AioWrapperTest, ReadTest) {
  int blockingInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(blockingInputFd, 0);

  char blockingBuff[BUFF_SIZE];
  memset(blockingBuff, 0, BUFF_SIZE*sizeof(char));
  
  ASSERT_GE(read(blockingInputFd, static_cast<void *>(blockingBuff),
                 BUFF_SIZE), 0);
  close(blockingInputFd);

  int aioInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(aioInputFd, 0);

  char aioBuff[BUFF_SIZE];
  memset(aioBuff, 0, BUFF_SIZE*sizeof(char));
  
  Mutex mutex;
  ConditionVar readDoneCond;
  bool okCallbackCalled = false;
  IOOkCallback *okCB = new IOOkCallback(bind(okCallback, &mutex,
                                             &readDoneCond, &okCallbackCalled,
                                             _1, _2, _3));
  IOCallback *ioCB = new IOCallback(okCB, NULL);
  ASSERT_EQ(0, aio_read(aioInputFd, static_cast<void *>(aioBuff),
                        BUFF_SIZE, OFFSET, ioCB));

  {
    ScopedLock sl(&mutex);
    readDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }

  close(aioInputFd);

  EXPECT_TRUE(okCallbackCalled);
  EXPECT_STREQ(blockingBuff, aioBuff);
}

TEST(AioWrapperTest, ReadErrTest) {
  int aioInputFd = open(READ_TEST_FILE, O_RDONLY);
  ASSERT_GT(aioInputFd, 0);

  char aioBuff[BUFF_SIZE];
  memset(aioBuff, 0, BUFF_SIZE*sizeof(char));
  
  Mutex mutex;
  ConditionVar readDoneCond;
  bool errOccured = false;
  bool okCallbackCalled = false;
  
  IOOkCallback *okCB = new IOOkCallback(bind(okCallback, &mutex,
                                             &readDoneCond, &okCallbackCalled,
                                             _1, _2, _3));
  IOErrCallback *errCB =
      new IOErrCallback(bind(errCallback, &mutex, &readDoneCond,
                             &errOccured, _1));
  IOCallback *ioCB = new IOCallback(okCB, errCB);
  
  ASSERT_EQ(0, aio_read(aioInputFd, static_cast<void *>(aioBuff),
                        BUFF_SIZE, OFFSET, ioCB));
  
  aio_cancel(aioInputFd, NULL);
  close(aioInputFd);

  {
    ScopedLock sl(&mutex);
    readDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }
  
  EXPECT_TRUE(errOccured);
  EXPECT_FALSE(okCallbackCalled);
}

TEST(AioWrapperTest, WriteTest) {
  int aioOutputFd = open(WRITE_TEST_FILE, O_RDWR | O_TRUNC | O_CREAT,
                         S_IRUSR | S_IWUSR);
  ASSERT_GT(aioOutputFd, 0);

  char aioBuff[BUFF_SIZE];
  memset(aioBuff, 0, BUFF_SIZE*sizeof(char));
  
  Mutex mutex;
  ConditionVar writeDoneCond;
  bool okCallbackCalled = false;
  
  strcpy(aioBuff, WRITE_TEST_BUFF);
  IOOkCallback *okCB = new IOOkCallback(bind(okCallback, &mutex,
                                             &writeDoneCond, &okCallbackCalled,
                                             _1, _2, _3));
  IOCallback *ioCB = new IOCallback(okCB, NULL);
  
  ASSERT_EQ(0, aio_write(aioOutputFd, static_cast<void *>(aioBuff),
                         BUFF_SIZE, OFFSET, ioCB));

  {
    ScopedLock sl(&mutex);
    writeDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }

  close(aioOutputFd);
  int inputFd = open(WRITE_TEST_FILE, O_RDONLY);
  char inputBuff[BUFF_SIZE];
  memset(inputBuff, 0, BUFF_SIZE*sizeof(char));
  
  ASSERT_GE(read(inputFd, static_cast<void *>(inputBuff), BUFF_SIZE), 0);
  close(inputFd);

  EXPECT_TRUE(okCallbackCalled);
  EXPECT_STREQ(WRITE_TEST_BUFF, inputBuff);
}

TEST(AioWrapperTest, WriteErrTest) {
  int aioOutputFd = open(WRITE_TEST_FILE, O_RDWR | O_TRUNC | O_CREAT,
                         S_IRUSR | S_IWUSR);
  ASSERT_GT(aioOutputFd, 0);

  char aioBuff[BUFF_SIZE];
  memset(aioBuff, 0, BUFF_SIZE*sizeof(char));
  
  Mutex mutex;
  ConditionVar writeDoneCond;
  bool okCallbackCalled = false;
  bool errOccured = false;
  
  strcpy(aioBuff, WRITE_TEST_BUFF);

  IOOkCallback *okCB = new IOOkCallback(bind(okCallback, &mutex,
                                             &writeDoneCond, &okCallbackCalled,
                                             _1, _2, _3));
  IOErrCallback *errCB =
      new IOErrCallback(bind(errCallback, &mutex, &writeDoneCond,
                             &errOccured, _1));
  IOCallback *ioCB = new IOCallback(okCB, errCB);
  
  ASSERT_EQ(0, aio_write(aioOutputFd, static_cast<void *>(aioBuff),
                         BUFF_SIZE, OFFSET, ioCB));
  
  aio_cancel(aioOutputFd, NULL);
  close(aioOutputFd);
  
  {
    ScopedLock sl(&mutex);
    writeDoneCond.timedWait(&mutex, IO_TIMEOUT);
  }
  
  EXPECT_TRUE(errOccured);
  EXPECT_FALSE(okCallbackCalled);
}

