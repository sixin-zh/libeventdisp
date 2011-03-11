#include "gtest/gtest.h"
#include "event.h"

#include <cstdlib>

using nyu_libeventdisp::EventDispatcher;
using nyu_libeventdisp::UnitTask;
using std::tr1::bind;

namespace {
void delayedInc(EventDispatcher *dispatcher, size_t *val, int delay) {
  if (--delay > 0) {
    dispatcher->enqueueTask(new UnitTask(bind(delayedInc, dispatcher,
                                              val, delay)));
  }
  else {
    (*val)++;
  }
}
} //namespace

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

