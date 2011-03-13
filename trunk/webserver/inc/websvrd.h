#ifndef ED_WEBSVRD_H_
#define ED_WEBSVRD_H_


#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

//#include <sys/time.h>
//#include <time.h>

//#include <unistd.h>

//#include <errno.h>	

//#include <fcntl.h>
//#include <sys/wait.h>
//#include <sys/stat.h>

// #include "event.h"
#include <dispatcher.h>
#include <aio_wrapper.h>


#define SA        sockaddr
#define SVR_PORT  htons(8088)
#define SVR_IPV4  htonl((((((127 << 8) | 0) << 8) | 0) << 8) | 1)

#define DEBUG_SVR_

void err_std(char *str) {
  printf("%s", str); 
}

#include <syslog.h>
void err_sys(char *str) {
  printf("%s", str);  //  syslog(LOG_ERR, str);
}

void log_std(char * str) {
  printf("%s", str); 
}
#endif
