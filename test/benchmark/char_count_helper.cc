#include "char_count_helper.h"

#include <cstring>
#include <cassert>

namespace nyu_libeventdisp_helper {
const size_t CharCountBlock::CACHE_LINE = 64;

CharCountBlock::CharCountBlock(size_t uniqueChars, bool fitToCacheLine) :
    size(uniqueChars) {
  size_t toAllocate = uniqueChars;
  
  if (fitToCacheLine) {
    size_t maxSize = CACHE_LINE / sizeof(uint32_t);
    assert(size < maxSize);
    toAllocate += (maxSize - uniqueChars);
  }
  
  count_ = new uint32_t[toAllocate]();
  memset(count_, 0, toAllocate*sizeof(uint32_t));
}

CharCountBlock::~CharCountBlock() {
  delete[] count_;
}

} // namespace

