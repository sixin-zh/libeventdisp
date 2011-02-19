#include "mp_event_dispatcher.h"

namespace nyu_libeventdisp {

using std::tr1::function;

MPEventDispatcher::MPEventDispatcher(void) {
}

MPEventDispatcher::MPEventDispatcher(size_t concurrentTaskCount) {
}

MPEventDispatcher::~MPEventDispatcher() {
}

bool MPEventDispatcher::enqueueTask(function<void()> *newTask) {
  return true;
}

bool MPEventDispatcher::enqueueTask(function<void()> *newTask,
                                    const TaskGroupID &id) {
  return true;
}

void MPEventDispatcher::stop(void) {
}

void MPEventDispatcher::resume(void) {
}

}

