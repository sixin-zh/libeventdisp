#include "dispatcher.h"
#include "lock.h"
#include "count_helper.h"

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

void reduce(CountBlock *count, size_t c) {
  count->inc(c, 1);
}

void map(size_t *randChars, CountBlock **count, size_t splitCount,
         size_t splitSize, size_t start, size_t end, Semaphore *sem) {
  for (size_t x = start; x < end; x++) {
    size_t current = randChars[x];
    size_t destination = current / splitSize;
    size_t offset = current % splitSize;

    Dispatcher::instance()->enqueue(
        new UnitTask(bind(reduce, count[destination], offset), destination));
  }

  for (size_t x = 0; x < splitCount; x++) {
    // Insert a sempahore up for synchronization
    Dispatcher::instance()->enqueue(new UnitTask(bind(checkpoint, sem), x));
  }
}

// Checks if the contents of two char blocks are equal.
bool checkEqual(CountBlock **count1, CountBlock **count2,
                size_t splitCount) {
  bool ret = true;

  for (size_t partition = 0; partition < splitCount; partition++) {
    CountBlock *block1 = count1[partition];
    CountBlock *block2 = count2[partition];

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

void doTest(size_t splitSize, size_t splitCount, size_t generateCount) {
  const size_t MAX_WORD = splitCount * splitSize;

  // Initialize the pseudo-random array of characters. The initialization
  // code uses a very deterministic sequence since the character distribution
  // greatly impacts the queue size of each dispatcher (which in turn, would
  // affect the total run time)
  size_t *randomWords = new size_t[generateCount]();
  for (size_t x = 0; x < generateCount; x++) {
    size_t nextChar = x % MAX_WORD;
    randomWords[x] = nextChar;
  }

  CountBlock **count = new CountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    count[x] = new CountBlock(splitSize);
  }

  Semaphore taskDoneSem(-1 * splitCount * splitCount + 1);
  
  {
    size_t start = 0;
    size_t wordsPerSplit = generateCount / splitCount;
    size_t end = wordsPerSplit;

    for (size_t partition = 0; partition < splitCount; partition++) {
      Dispatcher::instance()->enqueue(
          new UnitTask(bind(map, randomWords, count, splitCount, splitSize,
                            start, end, &taskDoneSem), partition));
      
      start = end;
      end += wordsPerSplit;
    }
  }

  // Wait for all the partitions to finish.
  taskDoneSem.down();
  
#ifdef CHECK_CORRECTNESS
  CountBlock **countS = new CountBlock*[splitCount]();
  for (size_t x = 0; x < splitCount; x++) {
    countS[x] = new CountBlock(splitSize);
  }

  for (size_t x = 0; x < generateCount; x++) {
    size_t current = randomWords[x];
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

  delete[] countS;
#endif
  
  for (size_t x = 0; x < splitCount; x++) {
    delete count[x];
  }

  delete[] count;
}
} // namespace

// A simple program that performs a simple word count using the libeventdisp
// library. For simplicity, each word is represented as unique number.
//
// Params:
//  1st arg - the number of concurrent threads to use.
//  2nd arg - the number of unique words to designate per thread
//  3rd arg - the number of words to test. Note that the program will generate
//   sequence of characters that will make sure that the resulting distribution
//   is even among threads.
int main(int argc, char **argv) {
  if (argc != 4) {
    cerr << "usage: " << argv[0] << " <thread count> <char per thread>"
         << " <char to generate per thread>"
         << endl;
    return -1;
  }

  const size_t splitSize = static_cast<size_t>(atoi(argv[2]));
  const size_t splitCount = static_cast<size_t>(atoi(argv[1]));
  const size_t generateCount = static_cast<size_t>(atoi(argv[3]));
  const size_t threadCount = splitCount;
  
  Dispatcher::init(threadCount, true);
  doTest(splitSize, splitCount, generateCount);
  
  return 0;
}

