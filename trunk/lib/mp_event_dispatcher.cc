#include "mp_event_dispatcher.h"

namespace nyu_libeventdisp {

using std::tr1::function;

MPEventDispatcher::MPEventDispatcher(void) {
}

MPEventDispatcher::MPEventDispatcher(size_t concurrentTaskCount) {
}

MPEventDispatcher::~MPEventDispatcher() {
}

bool MPEventDispatcher::enqueueTask(const UnitTask &newTask) {
  return true;
}

bool MPEventDispatcher::enqueueTask(const UnitTask &newTask, const TaskGroupID &id) {
  return true;
}

void MPEventDispatcher::stop(void) {
}

void MPEventDispatcher::resume(void) {
}

}

