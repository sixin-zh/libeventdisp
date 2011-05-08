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

void map(char *randChars, CharCountBlock *count, size_t splitCount,
         size_t splitSize, size_t start, size_t end, Semaphore *sem) {
  for (size_t x = start; x < end; x++) {
    char current = randChars[x];
    size_t destination = current / splitSize;

    Dispatcher::instance()->enqueue(
        new UnitTask(bind(reduce, count, static_cast<size_t>(current)),
                     destination));
  }

  for (size_t x = 0; x < splitCount; x++) {
    Dispatcher::instance()->enqueue(new UnitTask(bind(checkpoint, sem), x));
  }
}

// Checks if the contents of two char blocks are equal.
bool checkEqual(CharCountBlock *count1, CharCountBlock *count2) {
  bool ret = true;

  assert(count1->size == count2->size);
  const size_t size = count1->size;
    
  for (size_t x = 0; x < size; x++) {
    if (count1->get(x) != count2->get(x)) {
      cerr << "Error at element[" << x << "]: "
           << count1->get(x) << " != " << count2->get(x) << endl;
      ret = false;
    }
  }
  
  return ret;
}

// Method is designed to have a max(splitCount) == 4
void doTest(size_t splitSize, size_t splitCount,
            size_t generateCount, bool fitToCacheLine) {
  assert(splitCount <= 4);
  
  const char MAX_CHAR = splitCount * splitSize;
  const size_t TEST_SIZE = splitCount * generateCount;

  // Initialize the pseudo-random array of characters. The initialization
  // code uses a very deterministic sequence since the character distribution
  // greatly impacts the queue size of each dispatcher (which in turn, would
  // affect the total run time)
  char *randomChars = new char[TEST_SIZE]();
  for (size_t x = 0; x < TEST_SIZE; x++) {
    char nextChar = static_cast<char>(x % MAX_CHAR);
    randomChars[x] = nextChar;
  }

  CharCountBlock *count =
      new CharCountBlock(static_cast<size_t>(MAX_CHAR), fitToCacheLine);
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
  CharCountBlock *countS =
      new CharCountBlock(static_cast<size_t>(MAX_CHAR), fitToCacheLine);

  for (size_t x = 0; x < TEST_SIZE; x++) {
    countS->inc(static_cast<size_t>(randomChars[x]), 1);
  }

  if (checkEqual(countS, count)) {
    cout << "Correctness test passed!" << endl;
  }
  
  delete countS;
#endif
  
  delete count;
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

