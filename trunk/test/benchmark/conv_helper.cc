#include "conv_helper.h"
#include "lock.h"

#include <ctime>
#include <cstdlib>

namespace nyu_libeventdisp_helper {
using base::Semaphore;

Coord::Coord(size_t x, size_t y) : x(x), y(y) {
}

Matrix2D::Matrix2D(size_t width, size_t height)
    : width(width), height(height),
      array_(new int[sizeof(int)*width*height]) {
}

Matrix2D::~Matrix2D() {
  delete[] array_;
}

RandMatrix2D::RandMatrix2D(size_t length, size_t width)
    : Matrix2D(length, width) {
  initArray(array_, length*width);
}

void RandMatrix2D::initArray(int *arr, size_t size) {
  srand(time(NULL));
  
  for (size_t x = 0; x < size; x++) {
    arr[x] = rand();
  }
}

int applyMask(const Matrix2D *input, const Matrix2D *mask, const Coord &loc) {
  int sum = 0;
  Coord inputOffset(loc.x - mask->width/2, loc.y - mask->height/2);
  
  for (size_t y = 0; y < mask->width; y++) {
    for (size_t x = 0; x < mask->width; x++) {
      sum += input->get(inputOffset.x + x, inputOffset.y + y) * mask->get(x, y);
    }
  }

  return sum;
}

void partialConv(const Matrix2D *input, const Matrix2D *mask,
                 const Coord &start, const Coord &end,
                 Matrix2D *output) {
  for (size_t y = start.y; y < end.y; y++) {
    for (size_t x = start.x; x < end.x; x++) {
      output->set(x, y, applyMask(input, mask, Coord(x, y)));
    }
  }
}

void partialConv(const Matrix2D *input, const Matrix2D *mask,
                 const Coord &start, const Coord &end,
                 Matrix2D *output, Semaphore *taskDoneSem) {
  partialConv(input, mask, start, end, output);
  taskDoneSem->up();
}

} // namespace

