#ifndef ED_SVR_HTTP_H_
#define ED_SVR_HTTP_H_

#include <svr_tcp_ip.h>

#define MAXRH   8192
#define MAXWH   8192
#define MAXWHL  256    // max 32 lines for head
#define MAXWB   65536  // hex: 10000
#define MAXKEYS 256

// state
enum HttpSN {
  N_200,
  N_404,
  N_400,
  N_UNO,
};

enum HttpST { 
  HS_READING,
  HS_PARSING,
  HS_FETCHING,
  HS_WRITING,
  HS_UNKNOWN,
};

enum HttpWST {
  WS_HEAD,        
  WS_BODY,
  WS_UNO,
};

// transfer-encoding
enum TransEnc { 
  TE_IDE, // IDENTITY
  TE_CHU, // CHUNCKED
  TE_UNO, // UNKNOWN
};

enum ErrHTTP {
  EHTTP_OK,    
  EHTTP_READ,  EHTTP_READ_AIO,
  EHTTP_PARSE, EHTTP_PARSE_AIO,
  EHTTP_FETCH, EHTTP_FETCH_AIO,
  EHTTP_CACHE, EHTTP_CACHE_AIO,
  EHTTP_WRITE, EHTTP_WRITE_AIO,
  EHTTP_FINAL, EHTTP_FINAL_AIO,
};

struct charn {
  char * p; size_t n; bool cached;
  charn() { p = NULL; n = 0; cached = false; }
  ~charn() { if ((p!=NULL)&&(!cached)) delete[] p; }
};

// Http Package
struct HPKG {

  HttpSN   hsn;
  HttpST   hst;
  HttpWST  wst;
  TransEnc enc;

  ssize_t         vern; // rep version (1 -> HTTP/1.1, 0 -> HTTP/1.0)
  struct charn    ver;  // req version
  struct charn    met;  // method
  struct charn    uri;  // uri
  struct charn    host; // TODO

  struct charn    chead;  charn headkey;
  struct charn    cbody;  charn bodykey;

  int hfd; 

  off_t    tr_offset;           // nbytes read/writtern
  size_t   tr_nbytes;           // total size
  time_t   tr_current_time;
  time_t   tr_last_modify_time; // TOTO: MIME TYPE

  Conn **  cpn;                 // hpkg -> conn

  HPKG() { cpn = NULL; hst = HS_UNKNOWN; enc = TE_UNO; vern = 1; hfd = -1;}

  HPKG(Conn * & cn) {
    cpn = &cn;
    pthread_mutex_lock(&(cn->pkgl));
    ++cn->pkgc;
    pthread_mutex_unlock(&(cn->pkgl));
    hst = HS_UNKNOWN; enc = TE_UNO; 
    vern = 1; hfd = -1;
  }

  ~HPKG() {
    if (hfd != -1)  close(hfd);
  }
  
};

// call-forward
ErrHTTP svr_http_read  (HPKG * &);
ErrHTTP svr_http_parse (HPKG * &);
ErrHTTP svr_http_fetch (HPKG * &);
ErrHTTP svr_http_header(HPKG * &);
ErrHTTP svr_http_body  (HPKG * &);
ErrHTTP svr_http_final (HPKG * &);

#include "cache.h"
using nyu_libedisp_webserver::Cache;
using nyu_libedisp_webserver::CacheData;
using nyu_libedisp_webserver::CacheCallback;

// call-back
ErrHTTP svr_http_read_aio  (HPKG * &, int &, void *, const size_t &);
ErrHTTP svr_http_parse_aio (HPKG * &, int &, void *, const size_t &);
ErrHTTP svr_http_header_rio(HPKG * &, int &, void *, const size_t &);
ErrHTTP svr_http_header_wio(HPKG * &, const CacheData &);
ErrHTTP svr_http_body_rio  (HPKG * &, int &, void *, const size_t &);
ErrHTTP svr_http_body_wio  (HPKG * &, const CacheData &);
ErrHTTP svr_http_final_aio (HPKG * &, int &, const int &);


#endif
