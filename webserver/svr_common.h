#ifndef ED_SVR_COMMON_H_
#define ED_SVR_COMMON_H_

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


// Server Debug (verbal) Level -1,0,1,2,3,4,...
#define DBGL 1
#include <assert.h>
#include <errno.h>


// Server Parameters
#define NDISPATCHER 2

#define MaxCSL  512   // SOMAXCONN

#define ReadTimeoutUSEC 0
#define ReadTimeoutSEC  5 // <- 30

#define MaxACCEPT     1024
#define ACSLEEPTIME_U 50000

#define MAXCSIZE 65536 // cache size

#define MAXKEYS 256    // cache key length

#define MAXRH   8192    // read head size
#define MAXWH   8192    // write head size
#define MAXWHL  256     // max 32 lines for head

#define MAXWB   65536 // 1048576 // body block size 1024K


#endif


#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
void print_times(char * src, char * dest, timeval &prev, timeval &curr, timeval &prev_u, timeval &curr_u, timeval &prev_s, timeval &curr_s);


//#include <arpa/inet.h>
//#include <unistd.h>
//#include <sys/wait.h>
