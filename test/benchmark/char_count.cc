#include "dispatcher.h"
#include "lock.h"
#include "char_count_helper.h"

#include <cassert>
#include <iostream>
#include <cstdlib>

using std::cerr;
using std::endl;
using std::cout;
using std::tr1::bind;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::Dispatcher;
using namespace nyu_libeventdisp_helper;
using base::Semaphore;

namespace {
void checkpoint(Semaphore *sem) {
  sem->up();
}

void reduce(CharCountBlock *count, size_t c) {
  count->inc(c, 1);
}

void map(unsigned char *randChars, CharCountBlock **count, size_t splitCount,
         size_t splitSize, size_t start, size_t end, Semaphore *sem) {
  for (size_t x = start; x < end; x++) {
    unsigned char current = randChars[x];
    unsigned char destination = current / splitSize;
    unsigned char offset = current % splitSize;

    Dispatcher::instance()->enqueue(
        new UnitTask(bind(reduce, count[destination], offset), destination));
  }

  for (size_t x = 0; x < splitCount; x++) {
    Dispatcher::instance()->enqueue(new UnitTask(bind(checkpoint, sem), x));
  }
}

// Checks if the contents of two char blocks are equal.
bool checkEqual(CharCountBlock **count1, CharCountBlock **count2,
                size_t splitCount) {
  bool ret = true;

  for (size_t partition = 0; partition < splitCount; partition++) {
    CharCountBlock *block1 = count1[partition];
    CharCountBlock *block2 = count2[partition];

    assert(block1->size == block2->size);
    const size_t size = block1->size;
    
    for (size_t x = 0; x < size; x++) {
      if (block1->get(x) != block2->get(x)) {
          cerr << "Error at block[" << partition << "], "
               << "element[" << x << "]: "
               << block1->get(x) << " != " << block2->get(x) << endl;

        ret = false;
      }
    }
  }
  
  return ret;
}

void doTest(size_t splitSize, size_t splitCount,
            size_t generateCount, bool fitToCacheLine) {
  const unsigned char MAX_CHAR = splitCount * splitSize;
  const size_t TEST_SIZE = splitCount * generateCount;

  // Initialize the pseudo-random array of characters. The initialization
  // code uses a very deterministic sequence since the character distribution
  // greatly impacts the queue size of each dispatcher (which in turn, would
  // affect the total run time)
  unsigned char *randomChars = new unsigned char[TEST_SIZE]();
  for (size_t x = 0; x < TEST_SIZE; x++) {
    unsigned char nextChar = static_cast<char>(x % MAX_CHAR);
    randomChars[x] = nextChar;
  }

  CharCountBlock **count = new CharCountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    count[x] = new CharCountBlock(splitSize, fitToCacheLine);
  }

  Semaphore taskDoneSem(-1 * splitCount * splitCount + 1);
  
  {
    size_t start = 0;
    size_t end = generateCount;

    for (size_t partition = 0; partition < splitCount; partition++) {
      Dispatcher::instance()->enqueue(
          new UnitTask(bind(map, randomChars, count, splitCount, splitSize,
                            start, end, &taskDoneSem), partition));
      
      start = end;
      end += generateCount;
    }
  }

  // Wait for all the partitions to finish.
  taskDoneSem.down();
  
#ifdef CHECK_CORRECTNESS
  CharCountBlock **countS = new CharCountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    countS[x] = new CharCountBlock(splitSize, false);
  }

  for (size_t x = 0; x < TEST_SIZE; x++) {
    unsigned char current = randomChars[x];
    unsigned char destination = current / splitSize;
    unsigned char offset = current % splitSize;

    countS[destination]->inc(offset, 1);
  }

  if (checkEqual(countS, count, splitCount)) {
    cout << "Correctness test passed!" << endl;
  }
  
  for (size_t x = 0; x < splitCount; x++) {
    delete countS[x];
  }

  delete[] countS;
#endif
  
  for (size_t x = 0; x < splitCount; x++) {
    delete count[x];
  }

  delete[] count;
}
} // namespace

// A simple program that performs a simple character count using the
// libeventdisp library.
//
// Params:
//  1st arg - the number of concurrent threads to use.
//  2nd arg - the range of characters to designate per thread
//  3rd arg - the number of characters to be distributed per thread. Note that
//   the program will generate sequence of characters that will make sure that
//   the resulting distribution is even among threads.
//  4th arg (optional) - whether to try to fit the shared data structure used
//    per thread into the cache line or not. Defaults to false.
//
int main(int argc, char **argv) {
  if (argc < 4 || argc > 5) {
    cerr << "usage: " << argv[0] << " <thread count> <char per thread>"
         << " <char to generate per thread> [<consider cache line?(0/1)>]"
         << endl;
    return -1;
  }

  const size_t splitSize = static_cast<size_t>(atoi(argv[2]));
  const size_t splitCount = static_cast<size_t>(atoi(argv[1]));
  const size_t generateCount = static_cast<size_t>(atoi(argv[3]));
  bool fitToCacheLine =  false;
  const size_t threadCount = splitCount;
  
  if (argc == 5) {
    fitToCacheLine = atoi(argv[4]);
  }
  
  Dispatcher::init(threadCount, true);
  doTest(splitSize, splitCount, generateCount, fitToCacheLine);
  
  return 0;
}

