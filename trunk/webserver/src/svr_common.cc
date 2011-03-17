#include <svr_common.h>
#include <syslog.h>
// #include <cstdio>

#define MAXLEN 127

void err_sys(const char * format, ...) {
  fprintf(stderr, format); //  syslog(LOG_ERR, str);
}

void err_std(const char * format , ...) { 
  printf(format);
}
