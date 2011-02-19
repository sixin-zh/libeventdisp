#include "event_dispatcher.h"

namespace nyu_libeventdisp {

using std::tr1::function;

EventDispatcher::EventDispatcher(void) {
}

EventDispatcher::~EventDispatcher() {
}

bool EventDispatcher::enqueueTask(const function<void()> &newTask) {
  return true;
}

void EventDispatcher::stop(void) {
}

void EventDispatcher::resume(void) {
}

}

