#include "gtest/gtest.h"
#include "dispatcher.h"

#include <getopt.h>
#include <cstdlib>

using nyu_libeventdisp::Dispatcher;

enum PROG_OPTION {
  MULTIPROCESSOR_OPTION,
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  
  size_t concurrentDispatchers = 1;
  int optIndex;
  static option opt[] = {
    {"multi_mode", required_argument, NULL, MULTIPROCESSOR_OPTION},
    {0,0,0,0},
  };
  
  bool parseDone = false;
  while (!parseDone) {
    switch (getopt_long(argc, argv, "", opt, &optIndex)) {
      case MULTIPROCESSOR_OPTION:
        concurrentDispatchers = static_cast<size_t>(atoi(optarg));
        break;
      
      case -1:
        parseDone = true;
        break;
    }
  }

  if (concurrentDispatchers > 1) {
    Dispatcher::init(concurrentDispatchers, true);
  }
  else {
    Dispatcher::init();
  }
  
  return RUN_ALL_TESTS();
}

