#ifndef ED_SVR_CONN_H_
#define ED_SVR_CONN_H_

#include <svr_common.h>
#define MAXCLOC 1024

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
  CS_CLOSING_R, // read error
  CS_CLOSING_W, // write error
  CS_CLOSING,
  CS_CLOSED
};

// Conn can be understood as Session Layer in OSI model 
struct Conn {
  ConnST            cst;   // state
  int               cfd;   // fd [socket]
  sockaddr_in       csa;   // socket address (TODO: cast to sockaddr)
  Conn *            cpp;   // parent connection

  pthread_mutex_t   lock;  // connection lock
  size_t            nc;    // number of connections

  struct timeval  lifetime;    // for accept polling
  struct timeval  tim_accept;  // to estimate the lifetime

  // debug information
  char            curr_name[MAXCLOC];
  struct timeval  curr_time;
  struct timeval  curr_utime;
  struct timeval  curr_stime;
  
  Conn() {
    Conn((Conn *) NULL);
    /* pthread_mutex_init(&lock, NULL); */
    /* cpp = NULL; nc = 0; */

    /* // debug information */
    /* snprintf(curr_name, MAXCLOC, "svr_conn_begin"); */
    /* gettimeofday(&curr_time, NULL); */
    /* gettimeofday(&tim_accept, NULL); */
    /* struct rusage ru; getrusage(RUSAGE_SELF, &ru); */
    /* curr_utime = ru.ru_utime; */
    /* curr_stime = ru.ru_stime; */
  }

  Conn(Conn * cn) {
    pthread_mutex_init(&lock, NULL);
    cpp = NULL; nc = 0;

    if (cn != NULL) { // parent
      cpp = cn;
      pthread_mutex_lock(&(cpp->lock));
      ++cpp->nc;
      pthread_mutex_unlock(&(cpp->lock));
    }

    // debug information
    snprintf(curr_name, MAXCLOC, "svr_conn_begin");
    gettimeofday(&curr_time, NULL);
    gettimeofday(&tim_accept, NULL);
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    curr_utime = ru.ru_utime;
    curr_stime = ru.ru_stime;
  }


  ~Conn() {  

    struct timeval tim; struct rusage ru;
    gettimeofday(&tim,NULL);
    getrusage(RUSAGE_SELF, &ru);
    lifetime.tv_sec  = tim.tv_sec  - tim_accept.tv_sec;
    lifetime.tv_usec = tim.tv_usec - tim_accept.tv_usec;

    if (cpp != NULL) {
      pthread_mutex_lock(&(cpp->lock));
      --cpp->nc;
      cpp->lifetime.tv_sec = cpp->lifetime.tv_sec + LearningRATE * (lifetime.tv_sec - cpp->lifetime.tv_sec);
      cpp->lifetime.tv_usec = cpp->lifetime.tv_usec + LearningRATE * (lifetime.tv_usec - cpp->lifetime.tv_usec);
      pthread_mutex_unlock(&(cpp->lock));
    }

    if (DBGL == -1) printf("%.9lf\n",convert_tim_sec(lifetime)); // pirnt out lifetime

    // debug
    if (DBGL >= 1) {
      char * cname =  (char *) "svr_conn_end";
      assert(cpp != NULL);
      pthread_mutex_lock(&(cpp->lock));
      print_times((void *) this, curr_name, cname, curr_time, tim, curr_utime, ru.ru_utime, curr_stime, ru.ru_stime, (size_t) 0); 
      fflush(stdout);
      pthread_mutex_unlock(&(cpp->lock));
    }

    pthread_mutex_destroy(&lock);
  }

};



// Basis
ErrConn svr_conn_listen (Conn * &);            // listen (socket, bind, listen)
ErrConn svr_conn_accept (Conn * &, Conn * &);  // accept new connection from client
ErrConn svr_conn_connect(Conn * &);            // connect to another server
ErrConn svr_conn_close  (Conn * &);            // close connection

/* // TODO: KeepAlive */
/* #define MaxCSL  512   // max backlog: SOMAXCONN [socket dependent] */
/* #define MaxKeepAlive 150 */
/* #define ReadTimeoutUSEC 0 */
/* #define ReadTimeoutSEC 30 */
/* #define MaxACCEPT 500 */

// Notice: MaxKeepAlive,MaxCSL <= Max#FD

#endif
