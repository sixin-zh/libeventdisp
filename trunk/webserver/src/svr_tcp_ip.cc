#include <svr_common.h>
#include <svr_tcp_ip.h>
#include <svr_http.h>


int svr_conn_listen(Conn * cn) {
  
  int listenfd;

  // Socket Address
  struct sockaddr_in servaddr;
  
  // Init socket
  if ( (listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    err_sys("[tcpip] fail to create socket.\n");
    return -1;
  } // TODO: set the O_NONBLOCK flag    

  // Init servaddr
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family      = AF_INET;
  servaddr.sin_port        = SVR_PORT;
  if (INADDR_ANY) 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //  servaddr.sin_addr.s_addr = SVR_IPV4;

  // Bind servaddr to socket
  if ( (bind(listenfd, (SVR_SA *) &servaddr, sizeof(servaddr)) < 0) ) {
    err_sys("[tcpip] fail to bind servaddr to serv socket.\n");
    close(listenfd);
    return -2;
  }

  // Listen: connection-mode
  if ( (listen(listenfd, BackLog) < 0)) {
    err_sys("[tcpip] fail to listen.\n");
    return -3;
  }
  
  // TODO: err_std
  printf("[tcpip] server listening on %d, %s:%d \n", listenfd, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

  cn->connst = CS_CONNECTED;
  cn->connfd = listenfd;

  return 0;

}

// accept new conn from cn
Conn * svr_conn_accept(Conn * cn) {

  Conn * pn;

  // wait to accept
  int connfd = accept(cn->connfd, (SVR_SA *) (SVR_SA *) NULL, NULL);
  if (connfd < 0) {

    printf("[tcpip] errorno for accept: %d\n", errno); // when fd or accept limit exceeded
    pn = NULL;

  } else {

    // estabilished
    pn = new Conn;
    pn->connst = CS_CONNECTED;
    pn->connfd = connfd;

    // track
    struct sockaddr_in cl_addr; socklen_t cl_addrlen;
    getpeername(connfd,  (SVR_SA *) &cl_addr, &cl_addrlen);
    err_std("[tcpip] client socket accetped: %d, %s:%d \n", connfd, inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));

  }

  return pn;

}

// close the connection
int svr_conn_close(Conn * cn) {
  
  int connfd = cn->connfd;

  // TODO: delete all related HPKGS (impliecitly)
  delete cn;
  cn = NULL; // avold re-deletion in pool
  // TODO: Find cn in the pool, and delete the pointer (batch)
  
  if ( close(connfd) < 0 ) {
    err_sys("[tcpip] fail to close socket, return.\n");
    return -5;
  } else {
    printf("[tcpipk] socket closed: %d \n", connfd);
  }
  
  return 0;

}

