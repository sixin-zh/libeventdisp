#ifndef ED_SVR_COMMON_H_
#define ED_SVR_COMMON_H_

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>



// server debug verbal level
#define SDVL 1
#include <assert.h>
#include <errno.h>

// server log
void err_sys(const char * format , ...);
void err_std(const char * format , ...);


#endif



//#include <arpa/inet.h>
//#include <sys/time.h>
//#include <time.h>
//#include <unistd.h>
//#include <sys/wait.h>
