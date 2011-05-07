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

void map(char *randChars, CharCountBlock **count, size_t splitCount,
         size_t splitSize, size_t start, size_t end, Semaphore *sem) {
  for (size_t x = start; x < end; x++) {
    char current = randChars[x];
    size_t destination = current / splitSize;
    size_t offset = current % splitSize;

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

// Method is designed to have a max(splitCount) == 4
void doTest(size_t splitSize, size_t splitCount, bool fitToCacheLine) {
  assert(splitCount <= 4);
  
  const size_t CHARS_PER_SPLIT = 16;
  const char MAX_CHAR = splitCount * CHARS_PER_SPLIT;
  const size_t TEST_SIZE = splitCount * splitSize;

  // Initialize the pseudo-random array of characters. The initialization
  // code uses a very deterministic sequence since the character distribution
  // greatly impacts the queue size of each dispatcher (which in turn, would
  // affect the total run time)
  char *randomChars = new char[TEST_SIZE]();
  for (size_t x = 0; x < TEST_SIZE; x++) {
    char nextChar = static_cast<char>(x % MAX_CHAR);
    randomChars[x] = nextChar;
  }

  CharCountBlock **count = new CharCountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    count[x] = new CharCountBlock(CHARS_PER_SPLIT, fitToCacheLine);
  }

  Semaphore taskDoneSem(-1 * splitCount * splitCount + 1);
  
  {
    size_t start = 0;
    size_t end = splitSize;

    for (size_t partition = 0; partition < splitCount; partition++) {
      Dispatcher::instance()->enqueue(
          new UnitTask(bind(map, randomChars, count, splitCount, splitSize,
                            start, end, &taskDoneSem), partition));
      
      start = end;
      end += splitSize;
    }
  }

  // Wait for all the partitions to finish.
  taskDoneSem.down();
  
#ifdef CHECK_CORRECTNESS
  CharCountBlock **countS = new CharCountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    countS[x] = new CharCountBlock(CHARS_PER_SPLIT, false);
  }

  for (size_t x = 0; x < TEST_SIZE; x++) {
    char current = randomChars[x];
    size_t destination = current / splitSize;
    size_t offset = current % splitSize;

    countS[destination]->inc(offset, 1);
  }

  if (checkEqual(countS, count, splitCount)) {
    cout << "Correctness test passed!" << endl;
  }
  
  for (size_t x = 0; x < splitCount; x++) {
    delete countS[x];
  }

  delete countS;
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
//  3rd arg (optional) - whether to try to fit the shared data structure used
//    per thread into the cache line or not. Defaults to false.
//
int main(int argc, char **argv) {
  if (argc < 3 || argc > 4) {
    cerr << "usage: " << argv[0] << " <thread count> <char per thread>"
         << " [<consider cache line?(0/1)>]" << endl;
    return -1;
  }

  const size_t splitSize = static_cast<size_t>(atoi(argv[2]));
  const size_t splitCount = static_cast<size_t>(atoi(argv[1]));
  bool fitToCacheLine =  false;
  const size_t threadCount = splitCount;
  
  if (argc == 4) {
    fitToCacheLine = atoi(argv[3]);
  }
  
  Dispatcher::init(threadCount, true);
  doTest(splitSize, splitCount, fitToCacheLine);
  
  return 0;
}

