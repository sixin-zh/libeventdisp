#include <svr_common.h>
#include <svr_http.h>
#include <svr_tcp_ip.h>
#include <dispatcher.h>
#include <aio_wrapper.h>

#define BIND std::tr1::bind

using nyu_libeventdisp::Dispatcher;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::aio_read;
using nyu_libeventdisp::aio_write;
using nyu_libeventdisp::IOOkCallback;
using nyu_libeventdisp::IOErrCallback;
using nyu_libeventdisp::IOCallback;
using namespace std::tr1::placeholders;

// Dispatcher (already init in websvrd::main)
Dispatcher * disper =  Dispatcher::instance();

// New connection just accepted
// Loop to recieve related packages (round-robin, polling, ...)
int svr_http_init(Conn * cn) {
  
  // 

  // Check if still connnected
  if (cn->connst != CS_CLOSING) {
    
  // // read the request
  // UnitTask *newTask = new UnitTask(BIND(svr_http_read_head, connfd), connfd);  
  // if ( disper->enqueue(newTask) == false ) { 
  //   printf("[fail] can't enqueue the parse task, (close) the connection: %d.\n", connfd);
  //   return svr_http_end();
  //   // svr_tcp_ip_close(connfd); // TODO: http/1.1
  // }
  } else {
    
  }
  return 0;
}

int svr_http_read(HPKG * pk) {

  // char req_head[MAXHEADB];
  // memset(req_head, 0, sizeof req_head);

  // int req_len = read(connfd, req_head, sizeof req_head);
  // printf("Request Head:\n%s\n", req_head);

  // svr_http_write_head(connfd, "Connection: close\n",  SVR_HTTP_S_OK);

}


int svr_http_parse(HPKG * pk) {

  // parse line by line in head

  
  // read the body



  // allocate a buffer with source file size
  // struct stat statBuf;
  // fstat(sourceFd, &statBuf);
  // size_t fileSize = statBuf.st_size; 
  // char *rdata = new char[fileSize];
  // printf("rdata size: %lu, fileSize: %lu\n", strlen(rdata), fileSize); 
  // memset(rdata, 0, fileSize);

  // create the callback objects
  // IOOkCallback *readOkCB = new IOOkCallback(BIND(svr_http_inter));
  // IOErrCallback *errCB = new IOErrCallback(BIND(svr_http_inter));
  // IOCallback *ioCB = new IOCallback(readOkCB, errCB);

  
  return 0;

} //


int svr_http_inter(HPKG * pk) { 

  // GET /index.html
  // char srcFile[] = "./index.html";
  // int sourceFd = open(srcFile, O_RDONLY);
  //  assert(sourceFd > 0);
  
  //  char *aiodata = new char[MAXFILEB];
  // async read
  // int ret = aio_read(sourceFd, static_cast<void *>(aiodata), fileSize, 0, ioCB);
  // if (ret<0) {
  //   printf("[fail] aio_read init.\n");
    
  // }

}

int svr_http_write(HPKG * pk) {

  // if (cn == SVR_HTTP_S_OK) {
  //   const char replyOK[] = "HTTP/1.1 200\n";
  //   write(connfd, replyOK, strlen(replyOK));
  //   write(connfd, data, strlen(data));
  //   //write(connfd, "\0", 1);
  //   printf("data size: %lu\n", strlen(data)); 
  //   return svr_http_end();
  // } else {
  //   const char replyErr[] = "HTTP/1.1 404\n";
  //   write(connfd, replyErr, sizeof replyErr);
  // }

  //delete[] data;

  // // close the client connfd
  // UnitTask *newTask = new UnitTask(BIND(svr_close, connfd), connfd);
  // if ( disper->enqueue(newTask) == false ) {
  //   // wait a second ? make
  // }
  
  return 0;

}

 
int svr_http_final(HPKG * pk) {
  // delete poolSTR, poolUFD
  // delete[] poolSTR
  // close poolUFD
  //   delete pk;

  return 0;
}
