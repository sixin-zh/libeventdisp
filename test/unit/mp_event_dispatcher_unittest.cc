#include "gtest/gtest.h"
#include "event.h"
#include "test_helper.h"

using nyu_libeventdisp::MPEventDispatcher;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp_test::delayedInc;
using std::tr1::bind;

TEST(MPEventDispatcherTest, EventLoop) {
  const size_t ID_TYPE_COUNT = 10;
  const size_t MAX_ITER = 10;
  size_t val[ID_TYPE_COUNT];

  MPEventDispatcher dispatcher(ID_TYPE_COUNT);
  
  // Initialize the val array so we can easily determine the index just by
  // looking at the value of the element
  for (size_t x = 0; x < ID_TYPE_COUNT; x++) {
    val[x] = x * MAX_ITER;
  }

  srand(time(NULL));

  for (size_t x = 0; x < MAX_ITER; x++) {
    for (size_t y = 0; y < ID_TYPE_COUNT; y++) {
      EXPECT_TRUE(dispatcher.enqueueTask(new UnitTask(
          bind(delayedInc, &dispatcher, &(val[y]), rand() % MAX_ITER))));
    }
  }

  while (dispatcher.busy()) {
    // Keep on looping until all task are done
  }

  for (size_t x = 0; x < ID_TYPE_COUNT; x++) {
    EXPECT_EQ(MAX_ITER * (x + 1), val[x]);
  }
}
