#ifndef ED_WEBSVRD_H_
#define ED_WEBSVRD_H_

#include <dispatcher.h>
#include <aio_wrapper.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define SA        sockaddr
#define SVR_PORT  htons(8080)
#define SVR_IPV4  htonl((((((127 << 8) | 0) << 8) | 0) << 8) | 1)

#define DEBUG_SVR_
#include <assert.h>


#define BIND std::tr1::bind

void err_std(const char *str) {
  printf("%s", str);
}

#include <syslog.h>
void err_sys(const char *str) {
  printf("%s", str);  //  syslog(LOG_ERR, str);
}

//#include <arpa/inet.h>
//#include <sys/time.h>
//#include <time.h>
//#include <unistd.h>
//#include <errno.h>	
//#include <sys/wait.h>


#endif
