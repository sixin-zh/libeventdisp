#include "gtest/gtest.h"
#include "event.h"

using nyu_libeventdisp::EventDispatcher;

void dummyTask(int) {
}

TEST(EventDispatcherTest, Test) {
  EventDispatcher dispatcher;
  EXPECT_TRUE(dispatcher.enqueueTask(std::tr1::bind(dummyTask, 1)));
}

