#include <pthread.h>
#include <dispatcher.h>
using nyu_libeventdisp::Dispatcher;

#include <websvrd.h>

Conn * svrC; // server conn

// Start the server
void svr_start() {
  svrC = new Conn(NULL);
  Conn * peerC = NULL;   

  // listen
  if ( (svr_conn_listen(svrC) < 0) ) {
    if (DBGL >= 2) printf("[srv_start] server listen failed.\n");
    svr_stop(); 
    return;
  }

  if (DBGL >= 4) printf("svrC=%p\n", svrC);
  // loop
  while (svrC!=NULL) {
    svr_conn_accept(svrC,peerC); /* assume no concurrent accept */
  }
} 



// Stop the server
void svr_stop() {
  if (svrC != NULL) svr_conn_close(svrC); // TODO: del all the tasks in Dispatcher
}

/* Entrance */
int main(int argc, char* argv[]) {
  Dispatcher::init(NDISPATCHER,true);
  svrC = NULL; 
  svr_start(); 

  return 0;
}

