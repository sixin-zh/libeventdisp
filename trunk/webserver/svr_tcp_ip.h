#ifndef ED_SVR_CONN_H_
#define ED_SVR_CONN_H_

#include <svr_common.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Server Socket IP/PORT
#define SVR_SA    sockaddr
#define SVR_PORT  htons(8080)
#define SVR_IPV4  inet_addr("127.0.0.1")

// Error code of Conn
enum ErrConn {
  ERRCONN_OK,  // ok
  ERRCONN_CR,  // creation err
  ERRCONN_BI,  // bind err
  ERRCONN_LI,  // listen err
  ERRCONN_AC,  // accept err
  ERRCONN_CL,  // close err
  ERRCONN_CO,  // conn err
  ERRCONN_KA   // keepalive err 
};

// Connection status is shared around http packags
enum ConnST {
  CS_LISTENING,
  CS_CONNECTING,
  CS_CONNECTED,
  CS_CLOSING_R, // read 
  CS_CLOSING_W, // write
  CS_CLOSING,
  CS_CLOSED
};


// TODO: init pool size
// Conn can be understood as Session Layer in OSI model 
// [unix/socket dependent]
struct Conn {
  ConnST            cst;   // state
  int               cfd;   // fd (socket, file)
  sockaddr_in       csa;   // socket address (TODO: cast to sockaddr)
  POOL<Conn*>::L    csp;   // pool for child-connections
  Conn **           cpp;   // parent connection

  pthread_mutex_t   pkgl;  // pkgs lock
  size_t            pkgc;  // pkgs counter

  Conn() {
    cpp = NULL; pkgc = 0; 
    pthread_mutex_init(&pkgl, NULL); 
  }

  ~Conn() {
    pthread_mutex_destroy(&pkgl);
  }
};

// Basis
ErrConn svr_conn_listen (Conn * &);            // listen (socket, bind, listen)
ErrConn svr_conn_accept (Conn * &, Conn * &);  // accept new connection from client
ErrConn svr_conn_connect(Conn * &);            // connect to another server
ErrConn svr_conn_close  (Conn * &);            // close connection

// TODO: KeepAlive
#define MaxKeepAlive 100
#define KeepAliveTimeout 1  // 15

#define MaxCSL  512   // max backlog: SOMAXCONN [socket dependent]

// Notice: MaxKeepAlive,MaxCSL <= Max#FD

#endif
