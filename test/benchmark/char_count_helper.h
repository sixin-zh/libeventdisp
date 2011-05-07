#ifndef LIBEVENTDISP_CHAR_COUNT_HELPER_H_
#define LIBEVENTDISP_CHAR_COUNT_HELPER_H_

#include <cstddef>
#include <stdint.h>

namespace nyu_libeventdisp_helper {
// A simple container for holding counters
class CharCountBlock {
 public:
  static const size_t CACHE_LINE;
  const size_t size; // number of counter slots, excluding the padding

  // Class constructor.
  //
  // Params:
  //  uniqueChars - the number of unique chars to create a counter for.
  //  fitToCacheLine - whether to pad the data structure in order to fit the
  //    cache line. Requirement: size should be able to fit in cache line
  CharCountBlock(size_t uniqueChars, bool fitToCacheLine);
  ~CharCountBlock();
  
  inline uint32_t get(size_t idx) {
    return count_[idx];
  }

  // Increment by amount
  inline void inc(size_t idx, uint32_t amount) {
    count_[idx] += amount;
  }
  
 private:
  uint32_t *count_;
};

} //namespace

#endif

