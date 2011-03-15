#include "aio_wrapper.h"
#include "dispatcher.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <functional>
#include <iostream>

using namespace nyu_libeventdisp;
using namespace std;
using std::tr1::bind;
using namespace std::tr1::placeholders;

void errOccured(int fd, int err) {
  perror(strerror(err));

  close(fd);
  exit(0);
}

void copyDone(int fd, void* contents, ssize_t len) {
  assert (len >= 0);
  char *data = new char[len];
  memset(data, 0, len);

  strncpy(data, reinterpret_cast<char *>(contents), static_cast<size_t>(len));
  cout << "Successfully written: " << data << endl;

  delete[] static_cast<char *>(contents);
  delete[] data;
  
  close(fd);
  exit(0);
}

void copyTo(int destFd, int srcFd, void* contents, ssize_t len) {
  assert (len >= 0);
  IOOkCallback *writeOkCB = new IOOkCallback(copyDone);
  IOErrCallback *errCB = new IOErrCallback(errOccured);
  IOCallback *ioCB = new IOCallback(writeOkCB, errCB);

  int ret = aio_write(destFd, contents, len, 0, ioCB);
  assert(ret == 0);
  
  close(srcFd);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    cerr << "Usage: copy <src file> <dest file>";
    return -1;
  }

  // Initiates the event dispatcher. This is very important!
  Dispatcher::init();
  
  char *srcFile = argv[1];
  char *destFile = argv[2];

  // Open the source and destination files
  int sourceFd = open(srcFile, O_RDONLY);
  assert(sourceFd > 0);
  int destFd = open(destFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
  assert(destFd > 0);

  // Create the callback objects
  IOOkCallback *readOkCB = new IOOkCallback(bind(copyTo, destFd, _1, _2, _3));
  IOErrCallback *errCB = new IOErrCallback(errOccured);
  IOCallback *ioCB = new IOCallback(readOkCB, errCB);

  // Check for the size of the source file and allocate a buffer to store it
  struct stat statBuf;
  fstat(sourceFd, &statBuf);
  size_t fileSize = statBuf.st_size;
  char *data = new char[fileSize];
  memset(data, 0, fileSize);

  // Perform the asynchronous read
  int ret = aio_read(sourceFd, static_cast<void *>(data), fileSize, 0, ioCB);
  assert(ret == 0);

  while (true) {
    // Wait for copyDone() or errOccured() to call exit()
  }

  return 0;
}
