#include "mp_event_dispatcher.h"
#include "event_dispatcher.h"
#include "util.h"

#include <algorithm>

namespace nyu_libeventdisp {
MPEventDispatcher::MPEventDispatcher(size_t concurrentTaskCount) {
  for (size_t x = 0; x < concurrentTaskCount; x++) {
    dispatchers_.push_back(new EventDispatcher());
  }
}

MPEventDispatcher::~MPEventDispatcher() {
  //TODO: std::for_each(dispatchers_.begin(), dispatchers_.end(), deleteObj);
}

bool MPEventDispatcher::enqueueTask(UnitTask *newTask) {
  return true;
}

void MPEventDispatcher::stop(void) {
}

void MPEventDispatcher::resume(void) {
}

}

