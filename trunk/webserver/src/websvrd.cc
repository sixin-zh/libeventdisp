#include <websvrd.h>

#define BACKLOG 1024

using nyu_libeventdisp::Dispatcher;

int main(int argc, char **argv) { 

  printf("[server] launching. \n");

  int listenfd;
  struct sockaddr_in servaddr;
  
  // socket
  if ( (listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    err_sys("[server] socket create error.\n");
    return -1;
  }
    
#ifdef DEBUG_SVR_
  printf("\t[server] socket created: %d\n",listenfd);
#endif
  
  // init servaddr
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family      = AF_INET;
  servaddr.sin_port        = SVR_PORT;
  servaddr.sin_addr.s_addr = SVR_IPV4;

  // bind
  if ( (bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0) ) {
    err_sys("[server] socket bind error");
    close(listenfd);
    return -2;
  }


#ifdef DEBUG_SVR_ 
  printf("\t[server] socket binded: %x:%d\n", ntohl(servaddr.sin_addr.s_addr), ntohs(servaddr.sin_port));
#endif

  // listen
  if ( (listen(listenfd, BACKLOG) < 0)) {
    err_sys("[server] listen error");
    return -3;
  }

  printf("\t[server] listening on: %x:%d\n", ntohl(servaddr.sin_addr.s_addr), ntohs(servaddr.sin_port));

  // Processing
  int connfd; FILE *connio;

  Dispatcher::init();
  Dispatcher* dispatcher = Dispatcher::instance();

  while (true) {
    connfd = accept(listenfd, (SA *) NULL, NULL);
    printf("\t[server] client socket accetped: %d \n", connfd);

    if ((connio = fdopen(connfd, "rb")) == NULL) {
      err_sys("\t[server] client socket open failed.\n");
      close(connfd);
      return -4;
    }
    
    char ch;
    while (fscanf(connio, "%c", &ch) != EOF) {
      printf("%c", ch);
    }

    fclose(connio);
    close(connfd);
  }

  close(listenfd);
  return 0;

} // end main

