#include "mp_event_dispatcher.h"
#include "event_dispatcher.h"

#include <algorithm>

namespace nyu_libeventdisp {
// Notes:
// To avoid deadlock, locks should be acquired in the following order:
// tokenMutex_ -> taskTableMutex_

using std::pair;
using base::ScopedLock;

MPEventDispatcher::MPEventDispatcher(size_t concurrentTaskCount)
#ifdef USE_HASHING
    : concurrentTaskCount_(concurrentTaskCount) {
#else
    : concurrentTaskCount_(concurrentTaskCount), roundRobinToken_(0) {
#endif
  for (size_t x = 0; x < concurrentTaskCount; x++) {
    dispatchers_.push_back(new EventDispatcher());
  }
}

MPEventDispatcher::~MPEventDispatcher() {
  std::for_each(dispatchers_.begin(), dispatchers_.end(),
                deleteObj<EventDispatcher>);
}

bool MPEventDispatcher::enqueueTask(UnitTask *newTask) {
  size_t taskDestination;

#ifdef USE_HASHING
  taskDestination = hasher_(newTask->id) % concurrentTaskCount_;
#else
  {
    ScopedLock scopedToken(&tokenMutex_);
    ScopedLock scopedTable(&taskTableMutex_);
    
    pair<TaskIDTable::iterator, bool> newEntry =
        taskIDTable_.insert(std::make_pair(newTask->id, roundRobinToken_));

    if (!newEntry.second) {
      taskDestination = newEntry.first->second;
    }
    else {
      taskDestination = roundRobinToken_++;

      if (roundRobinToken_ >= concurrentTaskCount_) {
        roundRobinToken_ = 0;
      }
    }
  }
#endif

  dispatchers_[taskDestination]->enqueueTask(newTask);
  
  return true;
}

void MPEventDispatcher::stop(void) {
}

void MPEventDispatcher::resume(void) {
}

bool MPEventDispatcher::busy(void) {
  for (std::vector<EventDispatcher*>::iterator iter = dispatchers_.begin();
       iter != dispatchers_.end(); ++iter) {
    if ((*iter)->busy()) {
      return true;
    }
  }

  return false;
}
}

