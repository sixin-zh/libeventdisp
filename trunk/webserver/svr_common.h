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
#define DBGL 2
#include <assert.h>
#include <errno.h>


// Pool for general resource management
//#include <queue>
//#include <set>
#include <list>
template<class T>
struct POOL {
  //  typedef std::queue<T> Q;
  //  typedef std::set<T>   S;
  typedef std::list<T> L;
};


// server log
void err_sys(const char * format , ...);
void err_std(const char * format , ...);


#endif



//#include <arpa/inet.h>
//#include <sys/time.h>
//#include <time.h>
//#include <unistd.h>
//#include <sys/wait.h>
