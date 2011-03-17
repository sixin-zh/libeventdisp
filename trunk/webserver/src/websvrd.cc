#include <websvrd.h>
#include <dispatcher.h>

using nyu_libeventdisp::Dispatcher;

Conn * svrC; // server conn
PoolCONN pConns; // all peer connections

/* entrance */
int main(int argc, char **argv) { 
  // Dispatcher init
  Dispatcher::init();

  // Start the server
  svr_start();

  return 0;
}

void svr_start() {

  svrC = new Conn;
  
  // Listen
  if ( (svr_conn_listen(svrC) < 0) ) {
    err_sys("[fail] server init.\n");
    svr_end();
    return;
  }

  // Loop
  while (true) {
    // accept the connection
    Conn * peerC = svr_conn_accept(svrC);
    if (peerC != NULL) {
      pConns.insert(peerC);
      svr_http_init(peerC);
    }
  } // end loop

  svr_end();

} // end svr_start


void svr_end() {
  // stop (del) all the tasks in Dispatcher 

  // delete all conns in pool, and

  svr_conn_close(svrC); // the server
}
