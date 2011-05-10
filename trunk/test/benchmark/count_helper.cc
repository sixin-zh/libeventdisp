#include "count_helper.h"

#include <cstring>
#include <cassert>

namespace nyu_libeventdisp_helper {
CountBlock::CountBlock(size_t size) :
    size(size) {
  count_ = new uint32_t[size]();
  memset(count_, 0, size*sizeof(uint32_t));
}

CountBlock::~CountBlock() {
  delete[] count_;
}

} // namespace

