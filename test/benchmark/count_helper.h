#ifndef LIBEVENTDISP_CHAR_COUNT_HELPER_H_
#define LIBEVENTDISP_CHAR_COUNT_HELPER_H_

#include <cstddef>
#include <stdint.h>

namespace nyu_libeventdisp_helper {
// A simple container for holding counters
class CountBlock {
 public:
  const size_t size; // number of counter slots, excluding the padding

  // Class constructor.
  //
  // Params:
  //  size - the size of the block
  explicit CountBlock(size_t size);
  ~CountBlock();
  
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

