#ifndef ED_SVR_TCP_IP_H_
#define ED_SVR_TCP_IP_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SVR_SA    sockaddr
#define SVR_PORT  htons(8080)
#define SVR_IPV4  inet_addr("127.0.0.1")

#define BackLog  512   // TODO: max # of connections : ECONNREFUSED
#define MaxKeepAlive 100
#define KeepAliveTimeout 15

typedef enum ConnST {
  CS_CONNECTED,
  CS_IDLE,
  CS_BUDY,
  CS_CLOSING
};

struct Conn {
  ConnST connst; // state
  int    connfd; // fd
  // timeout
};


#include <set>
// pool of all connections
typedef std::set<Conn *> PoolCONN;
// TODO: init pool size  


int svr_conn_listen(Conn *);  // socket, bind, listen
Conn * svr_conn_accept(Conn *);    // accept new connection
int svr_conn_close(Conn *);     // active close connection


#endif


