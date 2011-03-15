#include <websvrd.h>

#define BACKLOG  1024   // max # of connections : ECONNREFUSED
#define MAXLINES 4096
#define MAXBUFFS 8192

using nyu_libeventdisp::Dispatcher;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::aio_read;
using nyu_libeventdisp::IOOkCallback;
using nyu_libeventdisp::IOErrCallback;
using nyu_libeventdisp::IOCallback;

Dispatcher *disper;

int  svr_init(int &);  // socket, bind, listen
void svr_parse_cb(int);
void svr_reply_cb(int connfd, char *data, size_t status);
void svr_close_cb(int);

int main(int argc, char **argv) { 

  printf("[server] started ... \n");
  
  int listenfd;

  // Init
  if ( (svr_init(listenfd) < 0) ) {
    err_sys("[fail] server init.\n");
    return -1;
  }

  Dispatcher::init();
  disper = Dispatcher::instance();

  // Loop
  while (true) {
    // accept the connection
    int connfd = accept(listenfd, (SA *) NULL, NULL);
    if (connfd < 0) {
      printf("error code: %d\n", errno);
      continue;
    }

    printf("[ok] client socket accetped: %d. \n", connfd);
    
    // parse the request
    UnitTask *newTask = new UnitTask(BIND(svr_parse_cb, connfd), connfd);  
    if ( disper->enqueue(newTask) == false ) {
      // wait a second ? switch to polling
    } 
    

  } // end loop

  close(listenfd);
  return 0;

} // end main

void svr_parse_cb(int connfd) {

  char buff[MAXBUFFS];
  int n = read(connfd, buff, sizeof buff);

  // TODO: Parse buff for GET
  //  printf("%d:\n%s\n", n, buff);

  // GET index.html
  char srcFile[] = "index.html";
  int sourceFd = open(srcFile, O_RDONLY);
  //  assert(sourceFd > 0);

  // allocate a buffer with source file size
  struct stat statBuf;
  fstat(sourceFd, &statBuf);
  size_t fileSize = statBuf.st_size; 
  char *rdata = new char[fileSize];
  printf("rdata size: %lu, fileSize: %lu\n", strlen(rdata), fileSize); 
  memset(rdata, 0, fileSize);

  // create the callback objects
  IOOkCallback *readOkCB = new IOOkCallback(BIND(svr_reply_cb, connfd, rdata, 1));
  IOErrCallback *errCB = new IOErrCallback(BIND(svr_reply_cb, connfd, rdata, 0));
  IOCallback *ioCB = new IOCallback(readOkCB, errCB);
  // TODO:  who is to delete ioCB ??

  int ret = aio_read(sourceFd, static_cast<void *>(rdata), fileSize, 0, ioCB);
  //  assert(ret == 0);
  
  return;

} //

void svr_reply_cb(int connfd, char *data, size_t status) {

  if (status == 1) {
    const char replyOK[] = "HTTP/1.1 200\n";
    write(connfd, replyOK, strlen(replyOK));
    write(connfd, data, strlen(data));
    write(connfd, "\0", 1);
    printf("data size: %lu\n", strlen(data)); 
  } else {
    const char replyErr[] = "HTTP/1.1 404\n";
    write(connfd, replyErr, sizeof replyErr);
  }

  delete[] data;

  // close the client connfd
  UnitTask *newTask = new UnitTask(BIND(svr_close_cb, connfd), connfd);
  if ( disper->enqueue(newTask) == false ) {
    // wait a second ? 
  }
  
  return;

} //

void svr_close_cb(int connfd) {
  if ( close(connfd) < 0 ) {
    err_sys("[fail] close client socket\n");
    return;
  }
  printf("[ok] close client socket: %d \n", connfd);

} //

int svr_init(int & listenfd) {
  
  // Socket Address
  struct sockaddr_in servaddr;
  
  // Init socket
  if ( (listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    err_sys("[fail] create socket.\n");
    return -1;
  } // TODO: set the O_NONBLOCK flag    

  // Init servaddr
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family      = AF_INET;
  servaddr.sin_port        = SVR_PORT;
  if (INADDR_ANY)  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind servaddr to socket
  if ( (bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0) ) {
    err_sys("[fail] bind servaddr to socket.\n");
    close(listenfd);
    return -2;
  }

  // Listen: connection-mode
  if ( (listen(listenfd, BACKLOG) < 0)) {
    err_sys("[fail] listen.\n");
    return -3;
  }

  printf("[ok] server listening on: %x:%d\n", ntohl(servaddr.sin_addr.s_addr), ntohs(servaddr.sin_port));

  return 0;
} //
