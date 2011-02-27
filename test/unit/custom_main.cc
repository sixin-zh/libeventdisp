#include "gtest/gtest.h"
#include "dispatcher.h"

using nyu_libeventdisp::Dispatcher;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  Dispatcher::init();
  return RUN_ALL_TESTS();
}

