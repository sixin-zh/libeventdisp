#include <svr_common.h>
#include <svr_tcp_ip.h>
#include <svr_http.h>

#include <fcntl.h>
#include <netinet/tcp.h>

// TODO: use zero-copy socket for large request
// TODO: set the O_NONBLOCK flag (for listen socket)

// Keepalive (socket)
int socketSetBlockingAndTimeout(int sockfd) {
  int ret;
  // set recv timeout
  timeval tv;
  tv.tv_usec = ReadTimeoutUSEC; tv.tv_sec = ReadTimeoutSEC;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
  if (ret < 0) return -1; //  printf("*****setsocket (fd=%d)  recv timeout ret = %d, errno = %d*****\n", sockfd, ret, errno);

  return 0;
}


ErrConn svr_conn_listen(Conn * &cn) {
  
  int listenfd;

  // Socket Address
  struct sockaddr_in servaddr;
  
  // init socket
  if ( (listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    if (DBGL >= 2) printf("[svr_conn_listen] socket create error.\n");
    return ERRCONN_CR;
  } 

  // init servaddr
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_port        = SVR_PORT;
  if (INADDR_ANY) servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //  servaddr.sin_addr.s_addr = SVR_IPV4;

  // set SO_REUSEADDR
  int optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  // bind servaddr to socket
  if ( (bind(listenfd, (SVR_SA *) &servaddr, sizeof(servaddr)) < 0) ) {
    if (DBGL >= 2) printf("[svr_conn_listen] socket bind error.\n");
    close(listenfd);
    return ERRCONN_BI;
  }
  
  // listen: connection-mode
  if ( (listen(listenfd, MaxCSL) < 0)) {
    if (DBGL >= 2) printf("[svr_conn_listen] socket listen error.\n");
    return ERRCONN_LI;
  }
  
  // TODO: err_std
  if (DBGL >= 2) printf("[svr_conn_listen] server listening on %d, %s:%d \n", listenfd, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

  cn->cst = CS_LISTENING;
  cn->cfd = listenfd;
  cn->csa = servaddr;
  cn->cpp = NULL;

  return ERRCONN_OK;

}


// Accept new conn pn from the listening server cn
ErrConn svr_conn_accept(Conn * &cn, Conn * &pn) {

  pn = NULL; // peer connection

  if ((cn==NULL) || (cn->cst != CS_LISTENING)) return ERRCONN_AC;

  // when accept limit exceeded
  while (cn->nc > MaxACCEPT) {
    if (DBGL >= 0) printf("[svr_conn_accept] max accept pool exceeded: %zu\n", cn->nc); // when accept limit exceeded
    sleep(ExpectedLifeTIME);
  }

  // wait to accept (in an 'endless' loop)
  int connfd = accept(cn->cfd, (SVR_SA *) (SVR_SA *) NULL, NULL);

  // while (errno == EINTR) {
  // printf("accept fd = %d, errno = %d \n", connfd, errno);
  //   connfd = accept(cn->cfd, (SVR_SA *) (SVR_SA *) NULL, NULL);
  // }

  if (connfd < 0) {
    if (DBGL >= 2) printf("[svr_conn_accept] accept fail, errorno: %d\n", errno); // when fd limit exceeded or ...
    //    if (errno != EINTR)
    //  sleep(ExpectedLifeTIME);
    return ERRCONN_AC;
  }
  
  // keepalive: set BLOCKING and I/O TIMEOUT
  if (socketSetBlockingAndTimeout(connfd) < 0) {
    if (DBGL >= 0) printf("[svr_conn_accept] fail to set nonblicking and timeout\n");
    return ERRCONN_AC;
  }

  // client connected
  pn = new Conn(cn);
  pn->cfd = connfd;
  pn->cst = CS_CONNECTED;

  // ip track
  if (DBGL >= 2) {
    struct sockaddr_in cl_addr; socklen_t cl_addrlen;
    getpeername(connfd,  (SVR_SA *) &cl_addr, &cl_addrlen);
    printf("[svr_conn_accept] new socket accetped: svr=%p, peer=%p, cfd=%d, ip=%s:%d \n", cn, pn, connfd, inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
  }

  HPKG * pk = new HPKG(pn);   // create HPKG (req)

  if (DBGL >= 5) printf("[svr_conn_accept] svr=%p, peer=%p, hpkg=%p\n", cn, pk->cpn, pk);

  // read header: let's go!
  if (svr_http_read(pk) == EHTTP_READ) 
    return ERRCONN_AC;

  return ERRCONN_OK;
}



// Connect pn->cfd to pn->csa
ErrConn svr_conn_connect(Conn * &pn) {

  if (pn == NULL) return ERRCONN_CO;

  pn->cst = CS_CONNECTING;
 
  // connect
  if (connect(pn->cfd, (SVR_SA *) &pn->csa, sizeof pn->csa) < 0) {
    if (DBGL >= 2) printf("[svr_conn_connect] socket connect failed.\n");
    return ERRCONN_CO;
  }

  pn->cst = CS_CONNECTED;
  // ip track
  if (DBGL >= 2) {
    struct sockaddr_in cl_addr; socklen_t cl_addrlen;
    getsockname(pn->cfd,  (SVR_SA *) &cl_addr, &cl_addrlen);
    printf("[svr_conn_connect] socket connected: %d, %s:%d \n", pn->cfd, inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
  }

  return ERRCONN_OK;

}

// Close the connection
ErrConn svr_conn_close(Conn * &cn) {

  if (DBGL >= 0) assert(cn != NULL);

  if (DBGL >= 4) printf("[svr_conn_close] close cn=%p\n", cn);

  // close itself
  int connfd = cn->cfd;
  if (close(connfd) < 0) {
    if (DBGL >= 2) { printf("[svr_conn_close] close sockek failed.\n");  fflush(stdout); }
    return ERRCONN_CL;
  } else {
    cn->cst = CS_CLOSED;
    if (DBGL >= 2) { printf("[svr_conn_close] socket closed = %d \n", connfd); fflush(stdout); }
  }
  Conn * pn = cn->cpp; // parent
  delete cn; cn = NULL;

  if (DBGL >= 2) { printf("[svr_conn_close] conn removed ->  pool size = %zu\n", pn->nc);  fflush(stdout); }  
  return ERRCONN_OK;

}
