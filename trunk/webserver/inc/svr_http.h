#ifndef ED_SVR_HTTP_H_
#define ED_SVR_HTTP_H_

#include <svr_tcp_ip.h>

#define MAXLINEB 1024
#define MAXHEADB 4096
#define MAXBODYB 8912
#define MAXFILEB 65536

typedef enum HttpST { // state
  HS_OK,
  HS_BAD
};

typedef enum TransEnc { // transfer-encoding
  TE_IDENTITY,
  TE_CHUNCKED
};


// Each HPKG has these two pools (free in svr_http_final)
#include <set>
typedef std::set<char *> PoolCSTR;    // cstring
typedef std::set<int>    PoolUXFD;    // unix fd 

struct HPKG { // Http Package
  Conn * conn;  
  HPKG * next; // next package
  
  char * ver;
  char * method;
  char * uri;
  char * head;
  int    headBytes;
  char * body;
  int    bodyBytes;

  HttpST   tSt;
  TransEnc tEnc;

  PoolCSTR pStr;
  PoolUXFD pFd;
};

int svr_http_init(Conn *);
int svr_http_read(HPKG *);
int svr_http_parse(HPKG *);
int svr_http_inter(HPKG *);
int svr_http_write(HPKG *);
int svr_http_final(HPKG *);


#endif


// #define CTLR "\n"
