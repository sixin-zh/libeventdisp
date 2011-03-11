#include "dispatcher.h"
#include "event.h"

namespace nyu_libeventdisp {
Dispatcher *Dispatcher::instance_ = NULL;

void Dispatcher::init(size_t concurrentDispatchers, bool multiProcessorMode) {
  instance_ = new Dispatcher(concurrentDispatchers, multiProcessorMode);
}
  
void Dispatcher::init(void) {
  init(1, false);
}

Dispatcher::Dispatcher(size_t concurrentDispatchers, bool multiprocessorMode) {
  if (multiprocessorMode) {
    eventDispatcher_ = new MPEventDispatcher(concurrentDispatchers);
  }
  else {
    eventDispatcher_ = new EventDispatcher();
  }
}
  
Dispatcher* Dispatcher::instance() {
  return instance_;
}

bool Dispatcher::enqueue(UnitTask *newTask) {
  return eventDispatcher_->enqueueTask(newTask);
}

}

