#ifndef ED_SVR_COMMON_H_
#define ED_SVR_COMMON_H_

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// Server Parameters
#define NDISPATCHER 1 // # of dispatchers

#define MaxCSL  512   // SOMAXCONN

#define ReadTimeoutSEC  5 // <- 30
#define ReadTimeoutUSEC 0

#define MaxACCEPT      1024 // hard limit
#define ExpectedLifeTIME  1 // sec

#define MAXCSIZE 1048576  // cache size: 1M
#define MAXKEYS 256       // cache key length
#define CacheTaskID 0  
#define ReadTaskID  (NDISPATCHER>1)?(cn->cfd%(NDISPATCHER-1))+1:0

#define MAXRH   8192    // read head size
#define MAXWH   8192    // write head size
#define MAXWHL  256     // max 32 lines for head
#define MAXWB   65536   // body block size 1024K

// DEBUG
#define DBGL 0 // Server Debug (verbal) Level -1,0,1,2,3,4,...

#include <assert.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

void print_times(const void * conn, const char * src, const char * dest, const timeval &prev, const timeval &curr, 
		 const timeval &prev_u, const timeval &curr_u, const timeval &prev_s, const timeval &curr_s, const size_t &qlen);
double convert_tim_sec(const timeval &tim);

#endif
