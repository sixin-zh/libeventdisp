#include "gtest/gtest.h"
#include "event.h"
#include "test_helper.h"

#include <cstdlib>

using nyu_libeventdisp::EventDispatcher;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp_test::delayedInc;
using std::tr1::bind;

TEST(EventDispatcherTest, EventLoop) {
  EventDispatcher dispatcher;
  const size_t MAX_ITER = 10;
  size_t val = 0;

  srand(time(NULL));

  for (size_t x = 0; x < MAX_ITER; x++) {
    EXPECT_TRUE(dispatcher.enqueueTask(new UnitTask(
        bind(delayedInc, &dispatcher, &val, rand() % MAX_ITER))));
  }

  while (dispatcher.busy()) {
    // Keep on looping until all task are done
  }
  
  EXPECT_EQ(MAX_ITER, val);
}

