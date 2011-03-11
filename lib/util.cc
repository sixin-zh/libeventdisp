#include "util.h"
#include <cstdlib>

namespace nyu_libeventdisp {
template<typename T>
void deleteObj(T *obj) {
  if (obj != NULL) {
    delete obj;
  }
}
} //namespace

