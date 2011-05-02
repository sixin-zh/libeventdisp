#include <svr_common.h>
#include <svr_tcp_ip.h>
#include <svr_http.h>

#include <fcntl.h>
#include <netinet/tcp.h>

// TODO: use zero-copy socket for large request
// TODO: set the O_NONBLOCK flag (for listen socket)


//
//
//
// Keepalive (socket)
int socketSetBlockingAndTimeout(int sockfd) {
  // set blocking and nodelay
  // unsigned int flags = fcntl(sockfd, F_GETFL, 0);
  // if (flags < 0) return -1;
  // if (fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK | O_NDELAY) < 0) return -1;


  ////
  // unsigned int flags = socket_fcntl(sockfd, F_GETFL, 0); 
  // flags |= O_NONBLOCK;
  // socket_fcntl(sockfd, F_SETFL, flags);

  int ret;
  // set recv timeout
  timeval tv;
  tv.tv_usec = ReadTimeoutUSEC; tv.tv_sec = ReadTimeoutSEC;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
  if (ret < 0) return -1; //  printf("*****setsocket (fd=%d)  recv timeout ret = %d, errno = %d*****\n", sockfd, ret, errno);

  // tv.tv_usec = 0; tv.tv_sec = KeepAliveTimeout;
  // ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(struct timeval));
  // if (ret < 0) return -1; //  printf("*****setsocket (fd=%d)  recv timeout ret = %d, errno = %d*****\n", sockfd, ret, errno);

  //  tv.tv_sec = KeepAliveTimeout;
  // setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(struct timeval)); // send blocking timeout

  // // nodelay
  // int flag = 1;
  // ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof (int));
  // if (ret < 0) return -1; //  printf("*****setsocket (fd=%d)  recv timeout ret = %d, errno = %d*****\n", sockfd, ret, errno);

  return 0;
}




//
//
//
//
ErrConn svr_conn_listen(Conn * &cn) {
  
  int listenfd;

  // Socket Address
  struct sockaddr_in servaddr;
  
  // init socket
  if ( (listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    if (DBGL >= 2) err_sys("[svr_conn_listen] socket create error.\n");
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

  // // keepalive
  // if (socketSetBlockingAndTimeout(listenfd) < 0) {
  //   if (DBGL >= 2) printf("[svr_conn_accept]  fail to set nonblicking and timeout\n"); // when fd or accept limit exceeded
  //   return ERRCONN_AC;
  // }

  // bind servaddr to socket
  if ( (bind(listenfd, (SVR_SA *) &servaddr, sizeof(servaddr)) < 0) ) {
    if (DBGL >= 2) err_sys("[svr_conn_listen] socket bind error.\n");
    close(listenfd);
    return ERRCONN_BI;
  }
  
  // listen: connection-mode
  if ( (listen(listenfd, MaxCSL) < 0)) {
    if (DBGL >= 2) err_sys("[svr_conn_listen] socket listen error.\n");
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


#define ACSLEEPTIME_U 500000
// Accept new conn pn from server cn (listening)
ErrConn svr_conn_accept(Conn * &cn, Conn * &pn) {

  pn = NULL; // peer connection

  if ((cn==NULL) || (cn->cst != CS_LISTENING)) return ERRCONN_AC;

  if (MaxACCEPT < cn->csp.size()) usleep(ACSLEEPTIME_U);; // TODO

  // wait to accept (in an 'endless' loop)
  int connfd = accept(cn->cfd, (SVR_SA *) (SVR_SA *) NULL, NULL);
  if (connfd < 0) {
    if (DBGL >= 2) printf("[svr_conn_accept] accept fail, errorno: %d\n", errno); // when fd or accept limit exceeded

    usleep(ACSLEEPTIME_U); // TODO
    
    return ERRCONN_AC;
  }
   
  // keepalive: set BLOCKING and I/O TIMEOUT
  if (socketSetBlockingAndTimeout(connfd) < 0) {
    if (DBGL >= 2) printf("[svr_conn_accept] fail to set nonblicking and timeout\n"); // when fd or accept limit exceeded
    return ERRCONN_AC;
  }

  // client connected
  pn = new Conn;
  pn->cfd = connfd;
  pn->cst = CS_CONNECTED;
  pn->cpp = &cn; // conn -> parent conn

  // ip track
  if (DBGL >= 3) {
    struct sockaddr_in cl_addr; socklen_t cl_addrlen;
    getpeername(connfd,  (SVR_SA *) &cl_addr, &cl_addrlen);
    printf("[svr_conn_accept] new socket accetped: %d, %s:%d \n", connfd, inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
  }

  pthread_mutex_lock(&(cn->pkgl));
  cn->csp.push_back(pn);                  // pooling
  HPKG * pk = new HPKG(cn->csp.back());   // create HPKG (req)
  if (DBGL >= 2) {  printf("svr_http_accept] new pool size %d\n", cn->csp.size()); }
  pthread_mutex_unlock(&(cn->pkgl));

  //  if (DBGL >= 6) printf("[svr_conn_accept] push ptr: %x\n",  &pn);
  //  if (DBGL >= 6) printf("[svr_conn_accept] pushed ptr: %x\n", &(cn->csp.back()));
  if (DBGL >= 2) printf("[svr_conn_accept] svr=%x, peer=%x, hpkg=%x\n", cn, *(pk->cpn), pk);

  // read header
  if (svr_http_read(pk) == EHTTP_READ) return ERRCONN_AC;

  return ERRCONN_OK;

}


//
//
//
// Connect pn->cfd to pn->csa
ErrConn svr_conn_connect(Conn * &pn) {

  if (pn == NULL) return ERRCONN_CO;

  pn->cst = CS_CONNECTING;
 

  // // keepalive: set BLOCKING and I/O TIMEOUT
  // if (socketSetBlockingAndTimeout(pn->cfd) < 0) {
  //   if (DBGL >= 2) printf("[svr_conn_connect] error setting nonblicking and timeout\n"); // when fd or accept limit exceeded
  //   return ERRCONN_CO;
  // }

  // connect
  if (connect(pn->cfd, (SVR_SA *) &pn->csa, sizeof pn->csa) < 0) {
    if (DBGL >= 2) printf("[svr_conn_connect] socket connect failed.\n");
    return ERRCONN_CO;
  }

  // // keepalive: set BLOCKING and I/O TIMEOUT
  // if (socketSetBlockingAndTimeout(pn->cfd) < 0) {
  //   if (DBGL >= 2) printf("[svr_conn_connect] error setting nonblicking and timeout\n"); // when fd or accept limit exceeded
  //   return ERRCONN_CO;
  // }

  pn->cst = CS_CONNECTED;
  // ip track
  if (DBGL >= 2) {
    struct sockaddr_in cl_addr; socklen_t cl_addrlen;
    getsockname(pn->cfd,  (SVR_SA *) &cl_addr, &cl_addrlen);
    printf("[svr_conn_connect] socket connected: %d, %s:%d \n", pn->cfd, inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
  }

  return ERRCONN_OK;

}


//
//
//
// if conn is not closed (by client or server), then close it and all sub-conns ..
ErrConn svr_conn_close(Conn * &cn) {

  if (DBGL >= 3) printf("[svr_conn_close] close cn=%x\n", cn);
  if (DBGL >= 1) assert(cn != NULL);
  
  if (cn == NULL) return ERRCONN_CO; // ! this should never happen for websvrd
  
  // close all sub connections of cn (if exist)
  POOL<Conn*>::L::iterator itr = cn->csp.begin();
  while (itr != cn->csp.end()) {
    svr_conn_close(*itr++);
  }

  // close itself
  int connfd = cn->cfd;
  if (close(connfd) < 0) {
    if (DBGL >= 1) err_sys("[svr_conn_close] close sockek failed.\n");
    return ERRCONN_CL;
  } else {
    cn->cst = CS_CLOSED; // cn == NULL
    if (DBGL >= 2) printf("[svr_conn_close] closed socket = %d \n", connfd);
  }
  Conn * & pn = *cn->cpp;
  delete cn; cn = NULL;

  // remove from parent pool
  if (DBGL >= 3) if ((&pn != NULL) && (pn != NULL)) printf("[svr_conn_close] svr pool remove pn=%x\n", pn);
  if ((&pn != NULL) && (pn != NULL)) {
    pthread_mutex_lock(&(pn->pkgl));
    pn->csp.remove(cn);
    if (DBGL >= 1) { printf("[svr_http_final] close conn ->  pool size = %d\n", pn->csp.size());  fflush(stdout); }
    pthread_mutex_unlock(&(pn->pkgl));
  }
  
      // // close all sub connections of cn (if closing) TODO: use another list for closing
      // POOL<Conn*>::L::iterator itr = svrC->csp.begin();
      // while (itr != svrC->csp.end()) {
      // 	if (DBGL >= 6) printf("[svr_start] iterate fd=%d\n",  (*itr)->cfd );
      // 	if ((*itr)->cst == CS_CLOSING) {
      // 	  if (DBGL >= 4) printf("[svr_start] close fd=%d\n",  (*itr)->cfd );
      // 	  svr_conn_close(*itr);
      // 	  svrC->csp.erase(itr++);
      // 	} else {
      // 	  ++itr;
      // 	}
      // }

    // POOL<Conn*>::L::iterator itr = csp.begin();
    // while (itr != csp.end()) {
    //   svr_conn_close(*(itr++));
    // }



  return ERRCONN_OK;

}
