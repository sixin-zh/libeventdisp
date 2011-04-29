
#include <pthread.h>
#define KASLEEPTIME_U 10 // keepalive sleeptime (usec)

#include <dispatcher.h>
using nyu_libeventdisp::Dispatcher;

#include <websvrd.h>
Conn * svrC; // server conn

// Start the server
void svr_start() {

  svrC = new Conn;
  Conn * peerC = NULL;   

  // listen
  if ( (svr_conn_listen(svrC) < 0) ) {
    if (DBGL >= 2) err_sys("[fail] server init.\n");
    svr_stop(); return;
  }

  // loop
  while (svrC!=NULL) {
    if (svrC->csp.size() > MaxKeepAlive) {
      // keepalive (polling) // TODO: asyn.
      
      if (DBGL >= 2) printf("max keepalive connections exceeded .. close and sleep...\n");

      usleep(KASLEEPTIME_U);

    } else { 
      // accept new conn
      svr_conn_accept(svrC,peerC); /* assume no concurrent accept */
    }
  } // end loop

  svr_stop();

} // end svr_start



// Stop the server
void svr_stop() {
  svr_conn_close(svrC); // the server
  // TODO: del all the tasks in Dispatcher
}

/* Entrance */
int main(int argc, char **argv) {

  Dispatcher::init(2,false);

  svrC = NULL; 
  svr_start(); 
  return 0;

}

