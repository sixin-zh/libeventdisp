#ifndef ED_SVR_COMMON_H_
#define ED_SVR_COMMON_H_

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


// server debug (verbal) level 0,1,2,3,4,...
#define DBGL 5
#include <assert.h>
#include <errno.h>


// Pool for general resource management
#include <list>
template<class T>
struct POOL {
  typedef std::list<T> L;
};


// Parameters
#define MaxCSL  511   // max backlog: SOMAXCONN [socket dependent]

#define MaxKeepAlive 150
#define ReadTimeoutUSEC 0
#define ReadTimeoutSEC 30

#define MaxACCEPT 600
#define ACSLEEPTIME_U 500000

#define MAXCSIZE 65536 // cache size
#define CacheTaskID 0  

#define MAXKEYS 256    // cache key length

#define MAXRH   8192
#define MAXWH   8192
#define MAXWHL  256     // max 32 lines for head
#define MAXWB   1048576 // 1024K, TODO 65536  // hex: 10000



#endif



//#include <arpa/inet.h>
//#include <sys/time.h>
//#include <time.h>
//#include <unistd.h>
//#include <sys/wait.h>
