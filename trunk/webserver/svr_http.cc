#include <time.h>

#include <svr_common.h>
#include <svr_http.h>
#include <svr_tcp_ip.h>

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


// #define MAXCSIZE 65536
static Cache _cache(MAXCSIZE);

// #define CacheTaskID 0

static const char * _websvrd      = "Rain/1.x";

// Status line
#define CRLF "\r\n"
static const char * _sline_200[2] = {"HTTP/1.0 200 OK"         , "HTTP/1.1 200 OK"};          //
static const char * _sline_400[2] = {"HTTP/1.0 400 Bad Request", "HTTP/1.1 400 Bad Request"}; //
static const char * _sline_404[2] = {"HTTP/1.0 404 Not Found"  , "HTTP/1.1 404 Not Found"};   //

// Read HTTP request [ACCETP, PARSE, FINAL]
ErrHTTP svr_http_read(HPKG * &pk) {

  if (DBGL >= 5) { printf("[svr_http] read \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  Conn * &cn = pk->cpn;

  if (DBGL >= 4) printf("[svr_http_read] hpkg=%p, cn=%p\n",pk, cn);

  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R)) {
    if (DBGL >= 1) printf("[svr_http_read] conn error, hpkg=%p, cn=%p\n",pk, cn);    
    return svr_http_final(pk); }

  pk->hst = HS_READING;

  if (DBGL >= 5) { printf("[svr_http_read] chead.p"); fflush(stdout); }
  // read header, assume no body, footer...
  if (pk->chead.p == NULL) { pk->chead.n = 0; pk->chead.p = (char *) malloc(MAXRH); }
  memset(pk->chead.p, '\0', sizeof(char)*MAXRH);

  if (DBGL >= 5) { printf("[svr_http_read] ioCB"); fflush(stdout); }
  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_parse_aio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, cn->cfd);  // PALL: SIDEEFFECT ON LINE 172,  // TOTO: RReadTaskID

  if (DBGL >= 5) { printf("[svr_http_read] call aio_read, pk=%p, cn=%p, buf=%p\n", pk, cn, pk->chead.p); fflush(stdout); }
  int nret = aio_read(cn->cfd, static_cast<void *> (pk->chead.p), MAXRH-1, 0, ioCB); // end with '\0' 
  if (DBGL >= 0) assert(nret == 0);

  return EHTTP_OK;
}


// Parse HTTP header [PARSE_AIO]
//
// GET /path/file.html HTTP/1.1
// Host: www.host1.com:80
// [blank line here]
//
// TOTO: proxy, GET http://www.somehost.com/path/file.html HTTP/1.0
ErrHTTP svr_http_parse(HPKG * &pk) {

  if (DBGL >= 5) { printf("[svr_http] parse \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  if (DBGL >= 5) { printf("[svr_http] parse cn="); fflush(stdout); printf("%p\n", pk->cpn); fflush(stdout); }
  Conn * &cn = pk->cpn;

  if (DBGL >= 4) printf("[svr_http_parse] hpkg=%p, cn=%p\n", pk, cn);

  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 1)  printf("[svr_http_parse] conn error, hpkg=%p\n", pk); 
    return svr_http_final(pk); } 

  pk->hst = HS_PARSING;

  char * ph = pk->chead.p;       // head
  char * pe = ph + pk->chead.n;  // '\0'
  
  HPKG * fget = NULL;   // first GET HPKG

  while (ph < pe) {
    // parse line by line
    char * pt = strstr(ph, CRLF);
    if (pt==NULL) pt = pe;
    // if (DBGL >= 6) { char _bk = *pt; *pt = '\0'; printf("[svr_http_parse] line [%s]\n", ph); *pt = _bk; }

    // GET
    if (strncmp(ph,"GET ",4) == 0) { // "GET " + URI + ' ' + "HTTP/1.*"
      // create new hpkg
      HPKG * gk = new HPKG(cn);

      // met
      gk->met.n = 3;
      gk->met.p = (char *) malloc(gk->met.n+1); // end with '\0'
      strcpy(gk->met.p, "GET");
      // if (DBGL >= 6) printf("[svr_http_parse] met: %s [%s]\n", gk->met.p, ph);

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

      if (DBGL >= 5) printf("[svr_http_parse] new hpkg=%p, cn=%p, met=%s, ver=%s, uri=%s\n", gk, cn, gk->met.p, gk->ver.p, gk->uri.p);

      // ordering
      gk->ppg = pk;   // parent
      if (pk->lcg == NULL) {
	fget = gk;    // first GET
	pk->lcg = gk; // last child [LOCK]
      }
      else {
	(pk->lcg)->nsg = gk; // next sibling [LOCK]
	pk->lcg = gk;        // last child
      }

    } // end GET
    else { // skip whole line (HEAD, POST, PUT, DELETE, OPTIONS, TRACE, ...)
      ph = pt + strlen(CRLF);  // if (DBGL >= 6) printf("[svr_http_parse_aio] skipping (%s)\n", ph);
    }
  } // end parsing

  
  if (fget != NULL) { // fetch the first GET
    bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, fget), CacheTaskID)); // [CacheTaskID] FETCH IN SERIAL !
    if (DBGL >= 0) assert(bret == true); // svr_http_final(fget);
  }
  else { // NO GET TASK !
    // continue "reading"
    if ( (cn->cpp)->nc <= MaxKeepAlive ) {
      if (DBGL >= 4) printf("[svr_http_parse] keepalive, current pool size = %zu\n", (cn->cpp)->nc );
      bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_read, pk), cn->cfd));      // TOTO: RReadTaskID
      if (DBGL >= 0) assert(bret == true); // { 
	// if (DBGL >= 1) printf("[svr_http_parse] dispatcher error, hpkg=%p\n", pk); 
	// return svr_http_final_aio(pk, cn->cfd, 0); }
    } else {
      if (DBGL >= 2) printf("[svr_http_parse] max pooled, cn=%p, hpkg=%p\n", cn, pk); 
      return svr_http_final_aio(pk, cn->cfd, 0);
    }
  }

  return EHTTP_OK;
} 



// Parse AIO
ErrHTTP svr_http_parse_aio(HPKG * &pk, int & fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 5) { printf("[svr_http] parse aio \n"); fflush(stdout); }
  
  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 0) assert(pk->chead.p == buf);

  Conn * &cn = pk->cpn;

  if (DBGL >= 6) { printf("[svr_http_parse_aio] new request (hpkg=%p, cn=%p, fd=%d, nbytes=%zu):\n%s\n", pk, cn, fd, nbytes, pk->chead.p);  fflush(stdout); }
  else if (DBGL >= 5) { printf("[svr_http_parse_aio] new request (hpkg=%p, cn=%p, fd=%d, nbytes=%zu)\n", pk, cn, fd, nbytes);  fflush(stdout);  }
  
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 1) printf("[svr_http_parse_aio] conn error, hpkg=%p\n", pk); 
    return svr_http_final(pk); }

  pk->hst = HS_PARSING;

  // HS_PARSING;
  pk->chead.p = (char *) buf;
  pk->chead.n = nbytes-1; // '\0 not counted

  // continue reading new request if not EOF
  if (nbytes == 0) { // EOF
    if (DBGL >= 1) { printf("[svr_http_parse_aio] zero bytes read, cn=%p, hpkg=%p\n", cn, pk);  fflush(stdout); }
    return svr_http_final_aio(pk, fd, 0);
  }
  
  return svr_http_parse(pk);
}


// FETCH; PUT; (CANCEL) [CacheTaskID]
ErrHTTP svr_http_header_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 5) { printf("[svr_http] header rio \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_header_rio] hpkg=%p, cn=%p, uri=%s, hfd=%d\n", pk, pk->cpn, pk->uri.p, pk->hfd);

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  { // open local file
    if (pk->hfd == -1) pk->hfd = open(pk->uri.p, O_RDONLY); // ! blocking

    if (pk->hfd < 0) { // 404

      if (DBGL >= 4) printf("[svr_http_header_rio] file open 404, pk=%p, hfd=%d\n", pk, pk->hfd);
      pk->hsn = N_404;

      // fetch
      size_t _ss = strlen(_sline_404[pk->vern])+2*strlen(CRLF)+1;
      pk->chead.p = (char*) malloc(_ss);
      pk->chead.n = snprintf(pk->chead.p, _ss, "%s%s%s", _sline_404[pk->vern], CRLF, CRLF);

      // put
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 0) assert(isput == true);
      pk->chead.cached = isput;

    }
    else {             // 200

      if (DBGL >= 4) printf("[svr_http_header_rio] file open 200, pk=%p, hfd=%d\n", pk, pk->hfd);
      pk->hsn = N_200;

      // FETCH
      struct stat stathfd; 
      fstat(pk->hfd, &stathfd);
      pk->tr_nbytes = stathfd.st_size;            // Content-Length
      pk->tr_last_modify_time = stathfd.st_mtime; // Last-Modified
      time(&(pk->tr_current_time));               // Date
      pk->tr_offset = 0;                          

      // MALLOC
      if (DBGL >= 0) assert (pk->chead.p == NULL);


      pk->chead.p = (char*) malloc (MAXWH); 

      // Date
      tm * ptm;
      char datestr[MAXWHL];
      ptm = gmtime(&(pk->tr_current_time));
      strftime(datestr, MAXWHL-1, "%a, %zu %b %Y %p GMT", ptm);

      char lastmod[MAXWHL];
      ptm = gmtime(&(pk->tr_last_modify_time));
      strftime(lastmod, MAXWHL-1, "%a, %zu %b %Y %p GMT", ptm);      

      pk->chead.n = snprintf(pk->chead.p, MAXWH-1, "%s%sDate: %s%sServer: %s%sLast-Modified: %s%sContent-Type: %s%sContent-Length: %zu%s%s", 
			     _sline_200[pk->vern],       // Status line
			     CRLF, 
			     datestr,                    // Date
			     CRLF,
			     _websvrd,                   // Server
			     CRLF,
			     lastmod,                    // Last-Modified
			     CRLF,
			     "text/plain",               // Content-Type
			     CRLF,
			     pk->tr_nbytes,              // Content-Length
			     CRLF, 
			     CRLF);                      // EOH

      if (DBGL >= 5) printf("[svr_http_header_rio] hpkg=%p, head[%zu] = \n%s\n", pk, pk->chead.n, pk->chead.p);

      // PUT HEAD
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 0) assert(isput == true);
      pk->chead.cached = isput;


    } // end 200
    
  } // end open

  return EHTTP_OK;
}


// [CacheTaskID]
ErrHTTP svr_http_header_to_body_aio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 5) { printf("[svr_http] header to body aio\n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_header_to_body_aio] hpkg=%p, cn=%p, hsn=%d\n", pk, pk->cpn,  pk->hsn);

  pk->hst = HS_UNKNOWN;
  pk->wst = WS_HEAD;

  // body
  if (pk->hsn == N_200)
    return svr_http_body(pk);
  
  return svr_http_final(pk);
}


// Got TODO: parallize the cache callback
ErrHTTP svr_http_header_wio(HPKG * &pk, const CacheData &cd) {

  if (DBGL >= 5) { printf("[svr_http] header wio \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  Conn * &cn = pk->cpn;

  if (DBGL >= 6) printf("[svr_http_header_wio] hpkg=%p, cn=%p, data=%s\n", pk, cn, cd.data);
  else if (DBGL >= 4) printf("[svr_http_header_wio] hpkg=%p, cn=%p, nbytes=%zu\n", pk, cn, cd.size);

  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 1) printf("[svr_http_header_wio] conn error, hpkg=%p\n", pk); 
    return svr_http_final(pk); }

  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;

  // hsn
  const char * loc = cd.data+9;
  if (strncmp(loc, "200", 3) == 0)
    pk->hsn = N_200;
  else if (strncmp(loc, "404", 3) == 0)
    pk->hsn = N_404;
  else
    pk->hsn = N_UNO;

  // write header
  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_header_to_body_aio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]

  int nret = aio_write(cn->cfd, const_cast<char *> (cd.data), cd.size, 0, ioCB); 
  if (DBGL >= 0) assert(nret == 0); //  return svr_http_final(pk);

  return EHTTP_OK;
}


// Get Head [CacheTaskID]
ErrHTTP svr_http_header(HPKG * &pk) {

  if (DBGL >= 5) { printf("[svr_http] header \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  Conn * &cn = pk->cpn;

  if (DBGL >= 4) printf("[svr_http_header] hpkg=%p, cn=%p\n", pk, cn);

  if (DBGL >= 5) { printf("[svr_http] check closing \n"); fflush(stdout); }
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 1) printf("[svr_http_header] conn error, hpkg=%p \n", pk); 
    return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  CacheCallback * cCB = new CacheCallback(BIND(svr_http_header_wio, pk, _1));

  if (DBGL >= 5) { printf("[svr_http] get key value \n"); fflush(stdout); }  
  if (pk->headkey.p == NULL) { pk->headkey.p = (char *) malloc(MAXKEYS); pk->headkey.cached = false; pk->headkey.p[MAXKEYS-1] = '\0'; }
  strncpy(pk->headkey.p, pk->uri.p, MAXKEYS-1); 
  pk->headkey.n = strlen(pk->headkey.p); // pk->uri.n;
  if (!_cache.get(pk->headkey.p, cCB)) {
    bool rsvp = _cache.reserve(pk->headkey.p);
    if (DBGL >= 0) assert(rsvp == true);
    cCB = new CacheCallback(BIND(svr_http_header_wio, pk, _1));
    bool iscac = _cache.get(pk->headkey.p, cCB); 
    if (DBGL >= 0) assert(iscac == true);
    return svr_http_header_rio(pk, pk->hfd, pk->chead.p, pk->chead.n);
  }

  return EHTTP_OK;
}


// 400 ERROR [CacheTaskID]
ErrHTTP svr_http_write_400(HPKG * &pk) {

  if (DBGL >= 5) { printf("[svr_http] write 400 \n"); fflush(stdout); }
 
  if (DBGL >= 1) assert(pk != NULL);
  
  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;
  pk->hsn = N_400;

  Conn * &cn = pk->cpn;
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  if (DBGL >= 2) printf("[svr_http_write_400] hpkg=%p, cn=%p\n", pk, cn);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]

  size_t _ss = strlen(_sline_400[1])+2*strlen(CRLF)+1;
  pk->chead.p = (char*) malloc(_ss);
  pk->chead.n = snprintf(pk->chead.p, _ss, "%s%s%s", _sline_400[1], CRLF, CRLF);
  int nret = aio_write(cn->cfd, (void *) pk->chead.p, pk->chead.n, 0, ioCB);
  if (DBGL >= 0) assert(nret == 0); //  return svr_http_final(pk);
  
  return EHTTP_OK;
}


// FETCH URI [CacheTaskID]
ErrHTTP svr_http_fetch(HPKG * &pk) { 

  if (DBGL >= 5) { printf("[svr_http] fetch\n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  Conn * &cn = pk->cpn;

  if (DBGL >= 2) printf("[svr_http_fetch] hpkg=%p, cn=%p, met=%s, ver=%s, uri=%s\n", pk, cn, pk->met.p, pk->ver.p, pk->uri.p);

  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 1) printf("[svr_http_fetch] pk=%p, cn=%p, conn error\n", pk, cn); 
    return svr_http_final(pk); }

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


// GOT, TODO: parallel this callback by cache
ErrHTTP svr_http_body_wio(HPKG * &pk,  const CacheData &cd) {

  if (DBGL >= 5) { printf("[svr_http] body wio\n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);

  if (DBGL >= 1)  printf("[svr_http_body_wio] hpkg=%p, cn=%p\n", pk, pk->cpn); 

  // cancel(ed)
  if (cd.data == NULL) {
    return svr_http_final(pk);
  }

  // put(ed)
  Conn * &cn = pk->cpn;
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W))  { 
    if (DBGL >= 1)  printf("[svr_http_body_wio] hpkg=%p, cn=%p, conn error\n", pk, pk->cpn); 
    return svr_http_final_aio(pk, cn->cfd, 0); }

  if (DBGL >= 6) printf("[svr_http_body_wio] hpkg=%p, cn=%p, nbytes=%zu, data=%s\n", pk, cn, cd.size, cd.data);
  else if (DBGL >= 3) printf("[svr_http_body_wio] hpkg=%p, cn=%p, nbytes=%zu\n", pk, cn, cd.size);

  pk->hst = HS_WRITING;
  pk->wst = WS_BODY;

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
  int nret = aio_write(cn->cfd, const_cast<char *> (cd.data), cd.size, 0, ioCB); 
  if (DBGL >= 0) assert(nret == 0); //  return svr_http_final(pk);

  return EHTTP_OK;
}


// WRITTEN [CacheTaskID]
ErrHTTP svr_http_body_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 5) { printf("[svr_http] body rio\n"); fflush(stdout); }

  if (DBGL >= 4) printf("[svr_http_body_rio] hpkg=%p, wst=%d\n", pk, pk->wst);

  if (pk->wst == WS_BODY) {
    if (DBGL >= 2) printf("[svr_http_body_rio] done with hpkg %p, body key =%s\n", pk, pk->bodykey.p); 
    if (pk->bodykey.p != NULL) _cache.doneWith(pk->bodykey.p);
    if (nbytes > 0) {    // ! EOF     
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

// PUT [CacheTaskID]
ErrHTTP svr_http_body_pio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 5) { printf("[svr_http] body pio\n"); fflush(stdout); }

  if (DBGL >= 4) printf("[svr_http_body_pio] hpkg=%p, put key=%s, size=%zu, data=%s\n", pk, pk->bodykey.p, nbytes, (char *) buf);

  bool iscac = _cache.put(pk->bodykey.p, static_cast<const char *> (buf), nbytes); 
  if (DBGL >= 0) assert(iscac == true);
  pk->cbody.cached = iscac;

  return EHTTP_OK;
}


// FETCH BODY [CacheTaskID]
ErrHTTP svr_http_body(HPKG * &pk) {
  
  if (DBGL >= 5) { printf("[svr_http] body \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_body] hpkg=%p, wst=%d\n", pk, pk->wst);

  Conn * &cn = pk->cpn;
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) {
    if (DBGL >= 1) printf("[svr_http_body] conn error, hpkg=%p\n", pk);
    return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_BODY;   //  if (pk->hsn == N_200) { // pk->enc == TE_IDE
  
  // new key
  if (pk->bodykey.p == NULL) { pk->bodykey.p = (char *) malloc(MAXKEYS); pk->bodykey.cached = false; pk->bodykey.p[MAXKEYS-1] = '\0'; }
  pk->bodykey.n = snprintf(pk->bodykey.p, MAXKEYS, "%s#%zu", pk->uri.p, pk->tr_offset);
  if (DBGL >= 3) printf("[svr_http_body] use body key: %s [%zu]\n", pk->bodykey.p, pk->bodykey.n);

  // get
  CacheCallback * cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
  if (!_cache.get(pk->bodykey.p, cCB)) {
    bool rsvp = _cache.reserve(pk->bodykey.p);
    if (DBGL >= 0) assert(rsvp == true);
    cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
    bool iscac = _cache.get(pk->bodykey.p, cCB);
    if (DBGL >= 0) assert(iscac == true); // return svr_http_final(pk);
	
    pk->cbody.p = (char *) malloc(MAXWB);
    if (pk->hfd == -1) { printf("[svr_http_body] to open file\n"); pk->hfd = open(pk->uri.p, O_RDONLY); } // ! blocking
    
    IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_pio, pk, _1, _2, _3));
    IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2)); // to cancel Reservation
    IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
    int nret = aio_read(pk->hfd, static_cast<void *> (pk->cbody.p), MAXWB, pk->tr_offset, ioCB);
    if (DBGL >=0) assert(nret == 0); // cancelReseration,  return svr_http_final(pk);
  }

  return EHTTP_OK;
}


// [CacheTaskID]
ErrHTTP svr_http_final_aio(HPKG * &pk, int & fd, const int &errcode) {

  if (DBGL >= 5) { printf("[svr_http] final aio\n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 0) assert(pk->cpn != NULL);

  Conn * & cn = pk->cpn; 

  if (DBGL >= 2) printf("[svr_http_final_aio] hpkg=%p, cn=%p, cn->cst=%d, error code=%d\n", pk, cn, cn->cst, errcode);

  if (DBGL >= 0) assert(cn->cst != CS_CLOSING);

  if ((pk->hst == HS_READING) || (pk->hst == HS_PARSING)) {
    // TODO: shutdown READ
    pthread_mutex_lock(&(cn->lock));
    if (cn->cst == CS_CLOSING_W)
      cn->cst = CS_CLOSING;
    else
      cn->cst = CS_CLOSING_R;
    pthread_mutex_unlock(&(cn->lock));
  }
  else if ((pk->hst == HS_WRITING) || (pk->hst == HS_FETCHING)) {
    // TODO: shutdown WRITE
    pthread_mutex_lock(&(cn->lock));
    if (cn->cst == CS_CLOSING_R)
      cn->cst = CS_CLOSING;
    else
      cn->cst = CS_CLOSING_W;
    pthread_mutex_unlock(&(cn->lock));

    // cancelReservation
    if ((pk->hst == HS_FETCHING) && (pk->wst == WS_BODY)) { // _cache.cancelReservation(pk->headkey.p);
      printf("[svr_http_final_aio] fetching (body) error, cancel key = %s\n", pk->bodykey.p);
      _cache.cancelReservation(pk->bodykey.p);
      return EHTTP_FINAL;
    }

    // doneWith
    if ((pk->hst == HS_WRITING) && (pk->wst == WS_HEAD))  {
      if (DBGL >= 0) assert(pk->headkey.p != NULL);
      if (DBGL >= 1) printf("[svr_http_final_aio] done with hpkg %p, head key =%s\n", pk, pk->headkey.p); 
      _cache.doneWith(pk->headkey.p);
    }

    if ((pk->hst == HS_WRITING) && (pk->wst == WS_BODY))  {
      if (DBGL >= 0) assert(pk->bodykey.p != NULL);
      if (DBGL >= 1) printf("[svr_http_final_aio] done with hpkg %p, body key =%s\n", pk, pk->bodykey.p); 
      _cache.doneWith(pk->bodykey.p);
    }
  }


  return svr_http_final(pk);
}

// TODO: cfd
ErrHTTP svr_http_final(HPKG * &pk) {

  if (DBGL >= 5) { printf("[svr_http] final \n"); fflush(stdout); }

  if (DBGL >= 0) assert(pk != NULL);
  if (DBGL >= 0) assert(pk->cpn != NULL);  

  Conn * & cn  = pk->cpn;
  HPKG *  park = pk->ppg;
  HPKG *  fget = pk->nsg;

  if (DBGL >= 3) { printf("[svr_http_final] hpkg=%p, fget=%p, park=%p, cn=%p\n", pk, fget, park, cn); fflush(stdout); }

  delete pk;
  if (DBGL >= 5) { printf("[svr_http_final] pk deleted \n"); fflush(stdout); }

  if (park == NULL) { // EOF, close connection
    if (DBGL >= 5) { printf("[svr_http_final] park \n"); fflush(stdout); }
    svr_conn_close(cn);
    if (DBGL >= 5) { printf("[svr_http_final] conn closed\n"); fflush(stdout); }
  }
  else if (fget == NULL) { // last GET, continue "reading"
    if (DBGL >= 5) { printf("[svr_http_final] fget\n"); fflush(stdout); }
    park->lcg = NULL;
    if (DBGL >= 5) { printf("[svr_http_final] keepalive\n"); fflush(stdout); }
    if ( (cn->cpp)->nc <= MaxKeepAlive) {
      if (DBGL >= 2) { 
	printf("[svr_http_final] keepalive cn=%p, hpkg=%p, current pool size = %zu\n", cn, park, (cn->cpp)->nc);
	fflush(stdout); }
      bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_read, park), cn->cfd));  // TOTO: RReadTaskID
      if (DBGL >= 0) assert(bret == true);
    } else {
      if (DBGL >= 2) { printf("[svr_http_final] max pooled, cn=%p, hpkg=%p\n", cn, park); fflush(stdout); }
      svr_http_final(park);
    }
  }
  else { // continue fetching
    if (DBGL >= 5) { printf("[svr_http_final] fetch\n"); fflush(stdout); }
    bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, fget), CacheTaskID)); // [CacheTaskID] IN SERIAL !
    if (bret == false) svr_http_final(fget);
  }
 
  return EHTTP_FINAL;
}
