#include "test_helper.h"
#include "event.h"

//#include <tr1/functional>

namespace nyu_libeventdisp_test {
using nyu_libeventdisp::EventDispatcherInterface;
using nyu_libeventdisp::UnitTask;
using std::tr1::bind;

void delayedInc(EventDispatcherInterface *dispatcher, size_t *val, int delay) {
  if (--delay > 0) {
    dispatcher->enqueueTask(new UnitTask(bind(delayedInc, dispatcher,
                                              val, delay)));
  }
  else {
    (*val)++;
  }
}
}
