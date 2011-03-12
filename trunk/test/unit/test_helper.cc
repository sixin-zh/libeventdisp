#include "test_helper.h"
#include "event.h"

namespace nyu_libeventdisp_test {
using nyu_libeventdisp::EventDispatcherInterface;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::TaskGroupID;

using std::tr1::bind;

void delayedInc(EventDispatcherInterface *dispatcher, size_t *val,
                int delay, TaskGroupID id) {
  if (--delay > 0) {
    dispatcher->enqueueTask(new UnitTask(bind(delayedInc, dispatcher,
                                              val, delay, id), id));
  }
  else {
    (*val)++;
  }
}
}
