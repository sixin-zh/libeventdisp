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
#include <assert.h>
#include <errno.h>

#define DBGL -1

// Server Parameters
#define NDISPATCHER 1

#define MaxCSL  512   // SOMAXCONN

#define ReadTimeoutUSEC 0
#define ReadTimeoutSEC  5 // <- 30

#define MaxACCEPT     4096
#define ACSLEEPTIME_U 50000

#define MAXCSIZE 1048576  // 1M cache size
#define MAXKEYS 256       // cache key length
#define CacheTaskID 0  
#define ReadTaskID  (NDISPATCHER>1)?(cn->cfd%(NDISPATCHER-1))+1:0

#define MAXRH   8192    // read head size
#define MAXWH   8192    // write head size
#define MAXWHL  256     // max 32 lines for head
#define MAXWB   65536 //  // body block size 1024K

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

void print_times(const void * conn, const char * src, const char * dest, const timeval &prev, const timeval &curr, const timeval &prev_u, const timeval &curr_u, const timeval &prev_s, const timeval &curr_s, const size_t &qlen);
double convert_tim_sec(const timeval &tim);
void do_stat(const char * fpath);

#endif

//#include <arpa/inet.h>
//#include <unistd.h>
//#include <sys/wait.h>
