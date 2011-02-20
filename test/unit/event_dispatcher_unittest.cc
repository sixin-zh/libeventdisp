#include "gtest/gtest.h"
#include "event.h"

#include <cstdlib>

using nyu_libeventdisp::EventDispatcher;
using std::tr1::bind;

void delayedInc(EventDispatcher *dispatcher, size_t *val, int delay) {
  if (--delay > 0) {
    dispatcher->enqueueTask(bind(delayedInc, dispatcher, val, delay));
  }
  else {
    (*val)++;
  }
}

TEST(EventDispatcherTest, EventLoop) {
  EventDispatcher dispatcher;
  const size_t MAX_ITER = 10;
  size_t val = 0;

  srand(time(NULL));

  for (size_t x = 0; x < MAX_ITER; x++) {
    EXPECT_TRUE(dispatcher.enqueueTask(bind(delayedInc, &dispatcher, &val,
                                            rand() % MAX_ITER)));
  }

#if 0
  while (dispatcher.busy()) {
    // Keep on looping until all task are done
  }
#else
  // TODO: change to be time independent. Reason for using sleep - the loop
  // above is not fool proof, as the last task can be dequeued (thus making
  // the queue empty) while still executing.
  sleep(2);
#endif
  
  EXPECT_EQ(MAX_ITER, val);
}

