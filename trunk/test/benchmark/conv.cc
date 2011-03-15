#include "dispatcher.h"
#include <ctime>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <time.h> //clock_gettime, link with -lrt
#include "lock.h"

using std::cerr;
using std::endl;
using std::cout;
using std::tr1::bind;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::Dispatcher;
using base::Semaphore;

namespace {
static const size_t MASK_DIMENSION = 7;

// Immutable coordinate representation
struct Coord {
  const size_t x;
  const size_t y;
  
  Coord(size_t x, size_t y) : x(x), y(y) {
  }
};

// A container for a fixed size 2 dimensional matrix.
class Matrix2D {
 public:
  const size_t width;
  const size_t height;
  
  Matrix2D(size_t width, size_t height)
      : width(width), height(height),
        array_(new int[sizeof(int)*width*height]) {
  }
  
  virtual ~Matrix2D() {
    delete[] array_;
  }

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
  RandMatrix2D(size_t length, size_t width)
      : Matrix2D(length, width) {
    initArray(array_, length*width);
  }

 private:
  static void initArray(int *arr, size_t size) {
    srand(time(NULL));
  
    for (size_t x = 0; x < size; x++) {
      arr[x] = rand();
    }
  }
};

// Applies the mask on a specific location in a matrix.
//
// Returns the result.
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

// Performs a convolution operation specified by the given bounds.
void partialConv(const Matrix2D *input, const Matrix2D *mask,
                 const Coord &start, const Coord &end,
                 Semaphore *taskDoneSem, Matrix2D *output) {
  for (size_t y = start.y; y < end.y; y++) {
    for (size_t x = start.x; x < end.x; x++) {
      output->set(x, y, applyMask(input, mask, Coord(x, y)));
    }
  }

  taskDoneSem->up();
}

// Creates a random matrix given the dimensions and measures the time it
// takes to perform a straight-forward convolution on a random square mask
// given the dimension without fancy optimizations.
//
// Params:
//  inputHeight - the height of the input matrix
//  inputWidth - the width of the input matrix
//  maskDimension - the length of one side of the square mask. This should be
//    an odd number.
//  partitions - the number of partitions to split the input matrix.
//
// Returns the elapsed time disregarding the input/output matrix and mask
// initialization.
timespec conv(size_t inputHeight, size_t inputWidth, size_t maskDimension,
              size_t partitions) {
  assert(maskDimension % 2 != 0);
  
  const RandMatrix2D mask(maskDimension, maskDimension);
  const size_t padding = maskDimension - 1;
  
  // Add paddings to surround the input matrix to make sure that all partitions
  // will have an equal amount of work required.
  const RandMatrix2D input(inputWidth + padding, inputHeight + padding);
  RandMatrix2D output(inputWidth, inputHeight);

  const size_t leftMargin = maskDimension / 2;
  const size_t &topMargin = leftMargin;
  const size_t rightMargin = input.width - leftMargin;
  const size_t yOffset = inputHeight / partitions;

  Semaphore taskDoneSem(-1 * partitions + 1);
  timespec startTime;
  assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
  
  for (size_t n = 0, yPos = topMargin; n < partitions; n++) {
    Coord start(leftMargin, yPos);
    
    yPos += yOffset;
    Coord end(rightMargin, yPos);
    
    Dispatcher::instance()->enqueue(
        new UnitTask(bind(partialConv, &input, &mask, start, end, &taskDoneSem,
                          &output), n));
  }

  // Wait for all the partitions to finish.
  taskDoneSem.down();
  
  timespec endTime;
  assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);

  timespec timeDiff;
  timeDiff.tv_nsec = endTime.tv_nsec - startTime.tv_nsec;
  timeDiff.tv_sec = endTime.tv_sec - startTime.tv_sec;

  // Adjust timespec to borrow from seconds if needed
  if (endTime.tv_nsec < startTime.tv_nsec) {
    timeDiff.tv_sec--;
    timeDiff.tv_nsec += 1000000000;
  }

  return timeDiff;
}

} //namespace

// A simple program that outputs the time it takes to perform a simple
// convolution using the libeventdisp library.
//
// Params:
//  1st arg - the number of partitions for the output matrix. More partitions
//    creates more chances for parallelism.
//  2nd arg - the height for one partition matrix.
//  3rd arg - the height of the input matrix.
//  4th arg (optional) - the number of concurrent threads to use. Will be
//    assigned with the value of partition count if not specified.
//
// Forcing the user to specify the partition height instead of the actual height
// ensures that the resulting height can be evenly distributed between
// partitions.
//
int main(int argc, char **argv) {
  size_t threadCount = 0;
  
  if (argc < 4 || argc > 5) {
    cerr << "usage: conv <partitions> <partitionHeight> <width> [<threads>]"
         << endl;
    return -1;
  }

  const size_t partitions = static_cast<size_t>(atoi(argv[1]));
  const size_t height = static_cast<size_t>(atoi(argv[2]) * partitions);
  const size_t width =  static_cast<size_t>(atoi(argv[3]));
  
  if (argc == 4) {
    threadCount = partitions;
  }
  else {
    threadCount =  static_cast<size_t>(atoi(argv[4]));
  }
  
  Dispatcher::init(threadCount, true);
  timespec duration = conv(height, width, MASK_DIMENSION, partitions);

  cout << width << "x" << height
       << " with " << partitions << " partitions took "
       << duration.tv_sec << " sec "
       << duration.tv_nsec << " nsec."
       << endl;
  
  return 0;
}

