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
#define MAXCLOC 1024
#include <assert.h>
#include <errno.h>


// Server Parameters
#define NDISPATCHER 1

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

#define CacheTaskID 0  
#define ReadTaskID  (NDISPATCHER>1)?(cn->cfd%(NDISPATCHER-1))+1:0

#endif


#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
void print_times(const void * conn, const char * src, const char * dest, const timeval &prev, const timeval &curr, const timeval &prev_u, const timeval &curr_u, const timeval &prev_s, const timeval &curr_s, const size_t &qlen);

void do_stat(const char * fpath);


//#include <arpa/inet.h>
//#include <unistd.h>
//#include <sys/wait.h>
