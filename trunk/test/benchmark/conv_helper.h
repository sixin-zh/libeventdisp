#ifndef LIBEVENTDISP_CONV_HELPER_H_
#define LIBEVENTDISP_CONV_HELPER_H_

#include <cstddef>

namespace base {
class Semaphore;
}

namespace nyu_libeventdisp_helper {

// Immutable coordinate representation
struct Coord {
  const size_t x;
  const size_t y;
  
  Coord(size_t x, size_t y);
};

// A container for a fixed size 2 dimensional matrix.
class Matrix2D {
 public:
  const size_t width;
  const size_t height;
  
  Matrix2D(size_t width, size_t height);
  virtual ~Matrix2D();

  inline int get(size_t x, size_t y) const {
    return array_[calcOffset(x, y)];
  }

  // Warning: Does not check bounds! Client should be careful.
  inline void set(size_t x, size_t y, int val) {
    array_[calcOffset(x, y)] = val;
  }
  
 protected:
  int* const array_;

 private:
  inline size_t calcOffset(size_t x, size_t y) const {
    return y * width + x;
  }
};

// A container for a 2D matrix with elements initialized with random values.
class RandMatrix2D : public Matrix2D {
 public:
  RandMatrix2D(size_t length, size_t width);

 private:
  static void initArray(int *arr, size_t size);
};

// Applies the mask on a specific location in a matrix.
//
// Returns the result.
int applyMask(const Matrix2D *input, const Matrix2D *mask, const Coord &loc);

// Performs a convolution operation specified by the given bounds
// (start <= range < end)
void partialConv(const Matrix2D *input, const Matrix2D *mask,
                 const Coord &start, const Coord &end,
                 Matrix2D *output);

void partialConv(const Matrix2D *input, const Matrix2D *mask,
                 const Coord &start, const Coord &end,
                 Matrix2D *output, base::Semaphore *taskDoneSem);
} //namespace

#endif

