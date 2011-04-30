#include <time.h>

#include <svr_common.h>
#include <svr_http.h>
#include <svr_tcp_ip.h>

#define CRLF "\r\n"
//#define MAXWBC 5      // max char for encoding hex length (not including '\n')

#include <dispatcher.h>
#include <aio_wrapper.h>

#define BIND std::tr1::bind

using nyu_libeventdisp::Dispatcher;
using nyu_libeventdisp::UnitTask;
using nyu_libeventdisp::aio_read;
using nyu_libeventdisp::aio_write;
using nyu_libeventdisp::IOOkCallback;
using nyu_libeventdisp::IOErrCallback;
using nyu_libeventdisp::IOCallback;
using namespace std::tr1::placeholders;


#define MAXCSIZE 1024
static Cache _cache(MAXCSIZE);

#define CacheTaskID 0

// Status line
static const char * _sline_200[2] = {"HTTP/1.0 200 OK\n"         , "HTTP/1.1 200 OK\n"};          // 16+1
static const char * _sline_400[2] = {"HTTP/1.0 400 Bad Request\n", "HTTP/1.1 400 Bad Request\n"}; // 25+1
static const char * _sline_404[2] = {"HTTP/1.0 404 Not Found\n"  , "HTTP/1.1 404 Not Found\n"};   // 23+1

// Server name
static const char * _websvrd      = "websvrd/1.1"; // 11+1


// Read HTTP request
ErrHTTP svr_http_read(HPKG * &pk) {

  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R)) {
    printf("[svr_http_read] conn error, hpkg=%x\n",pk); 
    return svr_http_final(pk); }

  pk->hst = HS_READING;

  // read header, assume no body, footer...
  charn & _tmp = pk->chead;
  if (_tmp.p == NULL) { _tmp.n = 0; _tmp.p = (char *) malloc(MAXRH); }
  memset(_tmp.p, 0, sizeof(char)*MAXRH);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_parse_aio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, cn->cfd); // PALL,, or RReadTaskID ? ! SIDEEFFECT ON LINE 172

  int iret = aio_read(cn->cfd, static_cast<void *> (_tmp.p), MAXRH, 0, ioCB); // end with '\0' 
  if (iret != 0) { 
      printf("[svr_http_read] dispatcher error, hpkg=%x\n", pk); 
      return svr_http_final(pk); }

  return EHTTP_OK;
}


// Parse HTTP header
//
// GET /path/file.html HTTP/1.1
// Host: www.host1.com:80
// [blank line here]
//
// TOTO: proxy, GET http://www.somehost.com/path/file.html HTTP/1.0
ErrHTTP svr_http_parse(HPKG * &pk) {

  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = *(pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 5)  printf("[svr_http_parse] conn error, hpkg=%x\n", pk); 
    return svr_http_final(pk); } 

  pk->hst = HS_PARSING;

  char * ph = pk->chead.p; // head
  char * pe = ph + pk->chead.n - 1;  // end
  
  while (ph <= pe) {
    // parse line by line
    char * pt = strstr(ph, CRLF);
    if (pt==NULL) pt = pe;
    if (DBGL >= 6) { char _bk = *pt; *pt = '\0'; printf("[svr_http_parse] line [%s]\n", ph); *pt = _bk; }

    // GET
    if (strncmp(ph,"GET ",4) == 0) { // "GET " + URI + ' ' + "HTTP/1.*"

      // create new hpkg
      HPKG * gk = new HPKG(cn);

      // met
      gk->met.n = 3;
      gk->met.p = (char *) malloc(gk->met.n+1); // end with '\0'
      strcpy(gk->met.p, "GET");
      //      if (DBGL >= 6) printf("[svr_http_parse] met: %s [%s]\n", gk->met.p, ph);

      // uri
      ph = ph + 4;  // URI + ' ' + "HTTP/1.*"
      char * ps = strchr(ph, ' ');
      if (*ph == '/') ++ph; // skip /
      if ((ps != NULL) && (ps < pt)) {
	gk->uri.n = ps-ph;
	gk->uri.p = (char *) malloc(gk->uri.n+1); // end with '\0'
	memcpy(gk->uri.p, ph, gk->uri.n); 
	gk->uri.p[gk->uri.n] = '\0';
	//if (DBGL >= 6) printf("[svr_http_parse] uri: %s [%s]\n", gk->uri.p, ph);
      
	// ver
	ph = ps + 1;
	if (ph <= pt) {
	  gk->ver.n = pt-ph;
	  gk->ver.p = (char *) malloc(gk->ver.n+1); // end with '\0'
	  memcpy(gk->ver.p, ph, gk->ver.n);
	  gk->ver.p[gk->ver.n] = '\0';
	}
	//if (DBGL >= 6) printf("[svr_http_parse] ver: %s [%s]\n", gk->ver.p, ph);
      } // else bad GET request (no uri or no ver)

      if (DBGL >= 5) printf("[svr_http_parse] hpkg=%x, met=%s, ver=%s, uri=%s\n", gk, gk->met.p, gk->ver.p, gk->uri.p);

      // fetch GET
      bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, gk), CacheTaskID)); //// IN SERIAL !
      if (bret == false) { 
	if (DBGL >= 5) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", gk); 
	return svr_http_final(gk); }

    } // end GET
    else { // skip whole line (HEAD, POST, PUT, DELETE, OPTIONS, TRACE, ...)
      if (DBGL >= 6) printf("[svr_http_parse_aio] skipping (%s)\n", ph);
      ph = pt + 1;   // TODO parse the body, footer
    }
  } // end parsing

  return EHTTP_OK;
} // end svr_http_parse



// Parse AIO
ErrHTTP svr_http_parse_aio(HPKG * &pk, int & fd, void * buf, const size_t &nbytes) {
  
  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 3) assert (pk->chead.p == buf);
  if (DBGL >= 4) printf("[svr_http_parse_aio] new request (hpkg=%x, fd=%d, nbytes=%d):\n%s\n", pk, fd, nbytes, pk->chead.p);

  Conn * &cn = *(pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 4) printf("[svr_http_parse_aio] conn error, hpkg=%x\n", pk); 
    return svr_http_final(pk); }

  // HS_PARSING;
  pk->chead.p = (char *) buf;
  pk->chead.n = nbytes-1; // '\0 not counted

  // continue reading new request if not EOF
  if (nbytes == 0) { // EOF
    if (DBGL >= 4) printf("[svr_http_parse_aio] 0 bytes error, hpkg=%x\n", pk); 
    return svr_http_final_aio(pk, fd, 0);
  }
  else {
    bool bret = Dispatcher::instance()->enqueue( new UnitTask(BIND(svr_http_read, pk), cn->cfd));  // TODO: RReadTaskID
    if (bret == false) { 
      if (DBGL >= 4) printf("[svr_http_parse_aio] dispatcher error, hpkg=%x\n", pk); 
      return svr_http_final(pk); }
  }
  
  return svr_http_parse(pk);
}


// FETCH; PUT; CANCEL [CacheTaskID]
ErrHTTP svr_http_header_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 4) printf("[svr_http_header_rio] hpkg=%x\n", pk);
  if (DBGL >= 3) assert(pk != NULL);

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  { // open local file
    if (pk->hfd == -1) pk->hfd = open(pk->uri.p, O_RDONLY); // ! blocking

    if (pk->hfd < 0) { // 404

      pk->hsn = N_404;

      // fetch
      char * _tmp = (char*) malloc(24);
      strncpy(_tmp, _sline_404[pk->vern], 24);
      pk->chead.p = _tmp; pk->chead.n = 23; pk->chead.cached = true;
      // put
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);

    }
    else {             // 200

      pk->hsn = N_200;

      // FETCH
      struct stat stathfd; 
      fstat(pk->hfd, &stathfd);
      pk->tr_nbytes = stathfd.st_size;            // Content-Length
      pk->tr_last_modify_time = stathfd.st_mtime; // Last-Modified
      time(&(pk->tr_current_time));               // Date
      pk->tr_offset = 0;                          

      // MALLOC
      charn & _tmp = pk->chead;
      if (DBGL >= 3) assert (_tmp.p == NULL);
      _tmp.p = (char*) malloc (MAXWH); _tmp.n = MAXWH-1; _tmp.cached = true;
      memset(_tmp.p, 0, sizeof(char)*MAXWH);
      // status line
      strncpy(_tmp.p, _sline_200[pk->vern], 17);
      // head line
      char hline[MAXWHL];
      memset(hline, 0, sizeof(char)*MAXWHL);
      // time display
      tm * ptm;
      // Date
      ptm = gmtime(&(pk->tr_current_time));
      strftime(hline, MAXWHL-1, "Date: %a, %d %b %Y %X GMT\n", ptm);
      strncat(_tmp.p, hline, MAXWHL-1); // TOTO: delete ptm;     
      // Server
      snprintf(hline, MAXWHL-1, "Server: %s\n", _websvrd);
      strncat(_tmp.p, hline, MAXWHL-1);
      // Last-Modified
      ptm = gmtime(&(pk->tr_last_modify_time));
      strftime(hline, MAXWHL-1, "Last-Modified: %a, %d %b %Y %X GMT\n", ptm);
      strncat(_tmp.p, hline, MAXWHL-1);
      // TODO: Content-Type
      snprintf(hline, MAXWHL-1, "Content-Type: text/html\n");
      strncat(_tmp.p, hline, MAXWHL-1);
      // To chuck or not
      if (false) { // ? size unknown, use chucked
	snprintf(hline, MAXWHL-1, "Transfer-Encoding: chunked\n");
	strncat(_tmp.p, hline, MAXWHL-1);
	pk->enc = TE_UNO; // TOTO: TE_CHU; 
      }
      else { // identity transfer encoding
	snprintf(hline, MAXWHL-1, "Content-Length: %d\n", pk->tr_nbytes);
	strncat(_tmp.p, hline, MAXWHL-1);
	pk->enc = TE_IDE;
      }
      strcat(_tmp.p, "\n"); _tmp.n = strlen(_tmp.p);
      // end of head

      if (DBGL >= 5) printf("[svr_http_header_rio] hpkg=%x, head[%d] = \n%s\n", pk, pk->chead.n, pk->chead.p);

      // PUT HEAD
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);

    } // end 200
    
  } // end open

  return EHTTP_OK;
}


// [CacheTaskID]
ErrHTTP svr_http_header_to_body_aio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 4) printf("[svr_http_header_to_body_aio] hpkg=%x, hsn=%d\n", pk,  pk->hsn);
  if (DBGL >= 3) assert(pk != NULL);

  pk->hst = HS_UNKNOWN;
  pk->wst = WS_HEAD;

  //  if (pk->headkey.p != NULL){  _cache.doneWith(pk->headkey.p); free(pk->headkey.p); pk->headkey.p = NULL; }

  // body
  if (pk->hsn == N_200)
    return svr_http_body(pk);
  
  return svr_http_final(pk);
    
}


// Got Head [CacheTaskID]
ErrHTTP svr_http_header_wio(HPKG * &pk, const CacheData &cd) {

  if (DBGL >= 4) printf("[svr_http_header_wio] hpkg=%x, nbytes=%d\n", pk, cd.size);
  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    printf("[svr_http_header_wio] conn error, hpkg=%x\n", pk); 
    return svr_http_final(pk); }

  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;

  // hsn
  const char * loc = cd.data+9;
  if (strncmp(loc, "200", 3)==0)
    pk->hsn = N_200;
  else if (strncmp(loc, "404", 3)==0)
    pk->hsn = N_404;
  else
    pk->hsn = N_UNO;

  // write header
  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_header_to_body_aio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
  int nret = aio_write(cn->cfd, const_cast<char *> (cd.data), cd.size, 0, ioCB); 
  if (nret != 0) return svr_http_final(pk);

  return EHTTP_OK;
}


// Get Head [CacheTaskID]
ErrHTTP svr_http_header(HPKG * &pk) {
  
  if (DBGL >= 4) printf("[svr_http_header] hpkg=%x\n", pk);
  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_header] conn error, hpkg=%x \n", pk); return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  CacheCallback * cCB = new CacheCallback(BIND(svr_http_header_wio, pk, _1));
  
  if (pk->headkey.p == NULL) { pk->headkey.p = (char *) malloc(MAXKEYS); }
  strncpy(pk->headkey.p, pk->uri.p, MAXKEYS);
  pk->headkey.n = pk->uri.n; // strlen(pk->headkey.p);
  if (!_cache.get(pk->headkey.p, cCB)) {
    bool rsvp = _cache.reserve(pk->headkey.p);
    if (DBGL >= 2) assert(rsvp == true);
    cCB = new CacheCallback(BIND(svr_http_header_wio, pk, _1));
    bool iscac = _cache.get(pk->headkey.p, cCB); 
    if (DBGL >= 2) assert(iscac == true);
    return svr_http_header_rio(pk, pk->hfd, pk->chead.p, pk->chead.n);
  }

  return EHTTP_OK;
}


// 400 ERROR [CacheTaskID]
ErrHTTP svr_http_write_400(HPKG * &pk) {

  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_write_400] hpkg=%x\n", pk);
  
  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;
  pk->hsn = N_400;

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
  int nret = aio_write(cn->cfd, (void *) (const_cast<char *> (_sline_400[1])), 26, 0, ioCB);
  if (nret != 0) return svr_http_final(pk);
  
  return EHTTP_OK;
}


// FETCH URI [CacheTaskID]
ErrHTTP svr_http_fetch(HPKG * &pk) { 

  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_fetch] hpkg=%x, met=%s, ver=%s, uri=%s\n", pk, pk->met.p, pk->ver.p, pk->uri.p);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_fetch] conn error\n"); return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;
  pk->hsn = N_UNO;

  // check 400
  if ((pk->met.p == NULL) || (pk->ver.p == NULL) || (pk->uri.p == NULL)) {
    return svr_http_write_400(pk);
  }
  if (strcmp(pk->ver.p, "HTTP/1.0") == 0) {
    pk->vern = 0;
  } else if (strcmp(pk->ver.p, "HTTP/1.1") == 0) {
    pk->vern = 1;
  } else {
    return svr_http_write_400(pk);
  }

  // header
  return svr_http_header(pk);
}


// BODY GOT [CacheTaskID]
ErrHTTP svr_http_body_wio(HPKG * &pk,  const CacheData &cd) {

  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_body_wio] hpkg=%x, nbytes=%d, data=%s\n", pk, cd.size, cd.data);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W))  { printf("[svr_http_body_wio] conn error\n"); return svr_http_final(pk); }

  pk->hst = HS_WRITING;
  pk->wst = WS_BODY;

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
  int nret = aio_write(cn->cfd, const_cast<char *> (cd.data), cd.size, 0, ioCB); 
  if (nret != 0) return svr_http_final(pk);

  return EHTTP_OK;
}


// WRITTEN [CacheTaskID]
ErrHTTP svr_http_body_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 4) printf("[svr_http_body_rio] (hpkg=%x, wst=%d)\n", pk, pk->wst);

  if (pk->wst == WS_BODY) {
    if (nbytes > 0) {    // ! EOF
      if (pk->bodykey.p != NULL) _cache.doneWith(pk->bodykey.p);
      pk->tr_offset += nbytes;
      return svr_http_body(pk); // continue writing
    } else {
      pk->wst = WS_UNO;  // EOF
      return svr_http_final(pk);
    }
  } else {
    return svr_http_final(pk);
  }
  
  return EHTTP_OK;
}

ErrHTTP svr_http_body_pio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 4) printf("[svr_http_body_pio] hpkg=%x, put key=%s, data=%s\n", pk, pk->bodykey.p, buf);

  bool iscac = _cache.put(pk->bodykey.p, static_cast<const char *> (buf), nbytes); 
  if (DBGL >= 2) assert(iscac == true); 
  pk->cbody.cached = iscac;

  return EHTTP_OK;
}


// FETCH BODY [CacheTaskID]
ErrHTTP svr_http_body(HPKG * &pk) {
  
  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_body] hpkg=%x, wst=%d\n", pk, pk->wst);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_body] conn error, hpkg=%x\n", pk); return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_BODY;
  
  //  if (pk->hsn == N_200) { // pk->enc == TE_IDE
  // new key
  if (pk->bodykey.p == NULL) { pk->bodykey.p = (char *) malloc(MAXKEYS); pk->bodykey.cached = false; }
  pk->bodykey.n = snprintf(pk->bodykey.p, MAXKEYS, "%s#%d", pk->uri.p, pk->tr_offset);
  if (DBGL >= 3) printf("[200 new body key]: %s [%d]\n", pk->bodykey.p, pk->bodykey.n);

  // get
  CacheCallback * cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
  if (!_cache.get(pk->bodykey.p, cCB)) {
    bool rsvp = _cache.reserve(pk->bodykey.p);
    if (DBGL >= 2) assert(rsvp == true);
    cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
    bool iscac = _cache.get(pk->bodykey.p, cCB);
    if (DBGL >= 2) assert(iscac == true); // return svr_http_final(pk);
	
    pk->cbody.p = (char *) malloc(MAXWB);
    if (pk->hfd == -1) pk->hfd = open(pk->uri.p, O_RDONLY); // ! blocking
    
    IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_pio, pk, _1, _2, _3));
    IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
    IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
    int nret = aio_read(pk->hfd, static_cast<void *> (pk->cbody.p), MAXWB, pk->tr_offset, ioCB);
    if (nret != 0) return svr_http_final(pk);
  }
    //} // else {
  //   return svr_http_final(pk);
  // }

  return EHTTP_OK;
}


// [CacheTaskID]
ErrHTTP svr_http_final_aio(HPKG * &pk, int & fd, const int &errcode) {

  if (DBGL >= 4) printf("[svr_http_final_aio] hpkg=%x, fd=%d, error code=%d\n", pk, fd, errcode);
  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 3) assert(pk->cpn != NULL);

  Conn * & cn = *(pk->cpn); 

  if (cn->cst != CS_CLOSING) {
    if (pk->hst == HS_READING) {
      //      pthread_mutex_lock(&(cn->pkgl));
      if (cn->cst == CS_CLOSING_W)
  	cn->cst = CS_CLOSING;
      else
  	cn->cst = CS_CLOSING_R; // TODO: shutdown READ
      //pthread_mutex_unlock(&(cn->pkgl));
    } 
    else if ((pk->hst == HS_WRITING) || (pk->hst == HS_FETCHING)) {
      //      pthread_mutex_lock(&(cn->pkgl));
      if (cn->cst == CS_CLOSING_R)
  	cn->cst = CS_CLOSING;
      else
  	cn->cst = CS_CLOSING_W; // TODO: shutdown WRITE
      //pthread_mutex_unlock(&(cn->pkgl));
    }
  }

  return svr_http_final(pk);
}

// [CacheTaskID]
ErrHTTP svr_http_final(HPKG * &pk) {

  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 3) assert(pk->cpn != NULL);

  if (pk->headkey.p != NULL) _cache.doneWith(pk->headkey.p);
  if (pk->bodykey.p != NULL) _cache.doneWith(pk->bodykey.p);
  
  Conn * & cn = *(pk->cpn);
  if (DBGL >= 5) printf("[svr_http_final] hpkg = %x, cn = %x, cn->cst = %d\n", pk, cn, cn->cst);
  delete pk; 

  pthread_mutex_lock(&(cn->pkgl));
  if (DBGL >= 3) { printf("[svr_http_final] hpkg->c = %d (before --)\n", cn->pkgc); fflush(stdout); }
  --cn->pkgc;
  if ((cn->pkgc == 0) && ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R) || (cn->cst == CS_CLOSING_W))) {
    pthread_mutex_unlock(&(cn->pkgl));
    if (DBGL >= 3) { printf("[svr_http_final] cn=%x, pool size = %d\n", cn, (*(cn->cpp))->csp.size() ); fflush(stdout); }
    svr_conn_close(cn);
  }
  else {
    pthread_mutex_unlock(&(cn->pkgl));
  }

  return EHTTP_FINAL;
}
