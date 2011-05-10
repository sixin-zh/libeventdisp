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
  CS_CLOSING_R, // read error
  CS_CLOSING_W, // write error
  CS_CLOSING,
  CS_CLOSED
};

#define MAXCLOC 1024
// Conn is regarded as Session Layer in OSI model 
class Conn {

 public:
  ConnST            cst;   // state
  int               cfd;   // fd [socket]
  sockaddr_in       csa;   // socket address (TODO: cast to sockaddr)
  Conn *            cpp;   // parent connection

  pthread_mutex_t   lock;  // connection lock
  size_t            nc;    // number of connections


  // debug information
  char            curr_name[MAXCLOC];
  struct timeval  curr_time;
  struct timeval  curr_utime;
  struct timeval  curr_stime;

  struct timeval  life_time;   
  struct timeval  life_utime;
  struct timeval  life_stime;
  /* struct timeval  acceptime; */
  /* struct timeval  acceptimeu; */
  /* struct timeval  acceptimes; */
  size_t          curr_nc;    // current # of conns (including itself)

  Conn(Conn * cn) {
    pthread_mutex_init(&lock, NULL);
    cpp = NULL; nc = 0;

    if (cn != NULL) { // parent
      cpp = cn;
      pthread_mutex_lock(&(cpp->lock));
      ++cpp->nc;
      pthread_mutex_unlock(&(cpp->lock));
    }

    if (DBGL == -1) {
      gettimeofday(&life_time, NULL);
      struct rusage ru; getrusage(RUSAGE_SELF, &ru);
      life_utime = ru.ru_utime;
      life_stime = ru.ru_stime;
      if (cpp != NULL) curr_nc = cpp->nc;
    }

    // debug information
    if (DBGL >= 1) {
      snprintf(curr_name, MAXCLOC, "svr_conn_begin");
      gettimeofday(&curr_time, NULL);
      struct rusage ru; getrusage(RUSAGE_SELF, &ru);
      curr_utime = ru.ru_utime;
      curr_stime = ru.ru_stime;
    }
  }

  ~Conn() {  

    // debug
    if (DBGL >= 1) {
      char * cname =  (char *) "svr_conn_end";
      struct timeval tim; struct rusage ru;
      gettimeofday(&tim,NULL);
      getrusage(RUSAGE_SELF, &ru);
      assert(cpp != NULL);
      pthread_mutex_lock(&(cpp->lock));
      print_times((void *) this, curr_name, cname, curr_time, tim, curr_utime, ru.ru_utime, curr_stime, ru.ru_stime, (size_t) 0); 
      fflush(stdout);
      pthread_mutex_unlock(&(cpp->lock));
    }

    if (DBGL == -1) { 
      struct timeval tim; struct rusage ru;
      gettimeofday(&tim,NULL);
      getrusage(RUSAGE_SELF, &ru);
      life_time.tv_sec  = tim.tv_sec  - life_time.tv_sec;
      life_time.tv_usec = tim.tv_usec - life_time.tv_usec;
      life_utime.tv_sec = ru.ru_utime.tv_sec -  life_utime.tv_sec;
      life_utime.tv_usec = ru.ru_utime.tv_usec - life_utime.tv_usec;
      life_stime.tv_sec = ru.ru_stime.tv_sec -  life_stime.tv_sec;
      life_stime.tv_usec = ru.ru_stime.tv_usec - life_stime.tv_usec;
      assert(cpp != NULL);
      pthread_mutex_lock(&(cpp->lock));
      printf("%zu->%zu: %.9lf, %.9lf, %.9lf\n", curr_nc, cpp->nc, convert_tim_sec(life_time), convert_tim_sec(life_utime), convert_tim_sec(life_stime)); 
      fflush(stdout);
      pthread_mutex_unlock(&(cpp->lock));
    }

    if (cpp != NULL) {
      pthread_mutex_lock(&(cpp->lock));
      --cpp->nc;
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

#endif
