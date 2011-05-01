#include <time.h>

#include <svr_common.h>
#include <svr_http.h>
#include <svr_tcp_ip.h>


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
#define CRLF "\r\n"
static const char * _sline_200[2] = {"HTTP/1.0 200 OK"         , "HTTP/1.1 200 OK"};          //
static const char * _sline_400[2] = {"HTTP/1.0 400 Bad Request", "HTTP/1.1 400 Bad Request"}; //
static const char * _sline_404[2] = {"HTTP/1.0 404 Not Found"  , "HTTP/1.1 404 Not Found"};   //
// Server name
static const char * _websvrd      = "websvrd/1.1"; // 


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
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, cn->cfd); // PALL: SIDEEFFECT ON LINE 172,  // TOTO: RReadTaskID

  int iret = aio_read(cn->cfd, static_cast<void *> (_tmp.p), MAXRH-1, 0, ioCB); // ? end with '\0' 
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

  char * ph = pk->chead.p;       // head
  char * pe = ph + pk->chead.n;  // '\0'
  
  HPKG ** call = NULL;

  while (ph < pe) {
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

      if (DBGL >= 5) printf("[svr_http_parse] new hpkg=%x, cn=%x, met=%s, ver=%s, uri=%s\n", gk, cn, gk->met.p, gk->ver.p, gk->uri.p);

      // set order
      gk->ppg = &pk;   // parent
      if (pk->lcg == NULL) {
	call = &gk;    // first GET call
	pk->lcg = &gk; // last child [LOCK]
      }
      else {
	(*(pk->lcg))->nsg = &gk; // next sibling [LOCK]
	pk->lcg = &gk;           // last child
      }

    } // end GET
    else { // skip whole line (HEAD, POST, PUT, DELETE, OPTIONS, TRACE, ...)
      ph = pt + strlen(CRLF);  // if (DBGL >= 6) printf("[svr_http_parse_aio] skipping (%s)\n", ph);
    }
  } // end parsing


  // HAS GET Task ?
  bool bret = false; // = NULL;
  while ((call != NULL) && (bret == false)) {
    if (DBGL >= 1) assert(*call != NULL);
    // process the first GET
    bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, *call), CacheTaskID)); // [CacheTaskID] IN SERIAL !
    // if not enqueed ... 
    if (bret == false) {
      if (*(pk->lcg) == (*call)) { pk->lcg = NULL; break; } // last GET
      HPKG ** next = (*call)->nsg;
      svr_http_final(*call);
      call = next;
      if (DBGL >= 1) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", *call); 
    }
  }
  
  // NO GET TASK !
  if (bret == false) {
    // continue "reading"
    if (((*(cn->cpp))->csp.size()) <= MaxKeepAlive) {
      printf("[svr_http_parse] pool size = %d\n", (*(cn->cpp))->csp.size());
      bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_read, pk), cn->cfd));  // TOTO: RReadTaskID
      if (bret == false) { 
	if (DBGL >= 4) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", pk); 
	return svr_http_final_aio(pk, cn->cfd, 0); }
    } else {
      if (DBGL >= 1) printf("[svr_http_parse] max pooled, cn=%x, hpkg=%x\n", cn, pk); 
      return svr_http_final_aio(pk, cn->cfd, 0);
    }
  }

  //  pthread_mutex_lock(&((*(cn->cpp))->pkgl));
  //  printf("svr_http_parse] conn pool size %d\n", (*(cn->cpp))->csp.size());
  //  pthread_mutex_unlock(&((*(cn->cpp))->pkgl));

  return EHTTP_OK;
} // end svr_http_parse



// Parse AIO
ErrHTTP svr_http_parse_aio(HPKG * &pk, int & fd, void * buf, const size_t &nbytes) {
  
  if (DBGL >= 1) assert(pk != NULL);
  if (DBGL >= 1) assert (pk->chead.p == buf);

  Conn * &cn = *(pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    if (DBGL >= 4) printf("[svr_http_parse_aio] conn error, hpkg=%x\n", pk); 
    return svr_http_final(pk); }

  if (DBGL >= 4) printf("[svr_http_parse_aio] new request (hpkg=%x, cn=%x, fd=%d, nbytes=%d):\n%s\n", pk, cn, fd, nbytes, pk->chead.p);

  pk->hst = HS_PARSING;

  // HS_PARSING;
  pk->chead.p = (char *) buf;
  pk->chead.n = nbytes-1; // '\0 not counted

  // continue reading new request if not EOF
  if (nbytes == 0) { // EOF
    if (DBGL >= 1) printf("[svr_http_parse_aio] zero bytes read error, cn=%x, hpkg=%x\n", cn, pk); 
    return svr_http_final_aio(pk, fd, 0);
  }
  else {

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
      size_t _ss = strlen(_sline_404[pk->vern])+2*strlen(CRLF)+1;
      pk->chead.p = (char*) malloc(_ss);
      pk->chead.n = snprintf(pk->chead.p, _ss, "%s%s%s", _sline_404[pk->vern], CRLF, CRLF);

      // put
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);
      pk->chead.cached = isput;

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
      if (DBGL >= 3) assert (pk->chead.p == NULL);


      pk->chead.p = (char*) malloc (MAXWH); 

      // Date
      tm * ptm;
      char datestr[MAXWHL];
      ptm = gmtime(&(pk->tr_current_time));
      strftime(datestr, MAXWHL-1, "%a, %d %b %Y %X GMT", ptm);

      char lastmod[MAXWHL];
      ptm = gmtime(&(pk->tr_last_modify_time));
      strftime(lastmod, MAXWHL-1, "%a, %d %b %Y %X GMT", ptm);      

      pk->chead.n = snprintf(pk->chead.p, MAXWH-1, "%s%sDate: %s%sServer: %s%sLast-Modified: %s%sContent-Type: %s%sContent-Length: %d%s%s", 
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
			     
      // // Server
      // snprintf(hline, MAXWHL-1, "Server: %s\n", _);
      // strncat(_tmp.p, hline, MAXWHL-1);
      // // Last-Modified
      // ptm = gmtime(&(pk->tr_last_modify_time));
      // strftime(hline, MAXWHL-1, "Last-Modified: %a, %d %b %Y %X GMT\n", ptm);
      // strncat(_tmp.p, hline, MAXWHL-1);
      // // TODO: Content-Type
      // snprintf(hline, MAXWHL-1, "Content-Type: text/html\n");
      // strncat(_tmp.p, hline, MAXWHL-1);
      // // To chuck or not
      // if (false) { // ? size unknown, use chucked
      // 	snprintf(hline, MAXWHL-1, "Transfer-Encoding: chunked\n");
      // 	strncat(_tmp.p, hline, MAXWHL-1);
      // 	pk->enc = TE_UNO; // TOTO: TE_CHU; 
      // }
      // else { // identity transfer encoding
      // 	snprintf(hline, MAXWHL-1, "Content-Length: %d\n", pk->tr_nbytes);
      // 	strncat(_tmp.p, hline, MAXWHL-1);
      // 	pk->enc = TE_IDE;
      // }
      // strcat(_tmp.p, "\n"); _tmp.n = strlen(_tmp.p);
      // // end of head

      if (DBGL >= 5) printf("[svr_http_header_rio] hpkg=%x, head[%d] = \n%s\n", pk, pk->chead.n, pk->chead.p);

      // PUT HEAD
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);
      pk->chead.cached = isput;


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

  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { 
    printf("[svr_http_header_wio] conn error, hpkg=%x\n", pk); 
    return svr_http_final(pk); }

  if (DBGL >= 4) printf("[svr_http_header_wio] hpkg=%x, cn=%x, nbytes=%d\n", pk, cn, cd.size);

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
  
  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_header] conn error, hpkg=%x \n", pk); return svr_http_final(pk); }

  if (DBGL >= 4) printf("[svr_http_header] hpkg=%x, cn=%x\n", pk, cn);

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
  
  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;
  pk->hsn = N_400;

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  if (DBGL >= 4) printf("[svr_http_write_400] hpkg=%x, cn=%x\n", pk, cn);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]

  size_t _ss = strlen(_sline_400[1])+2*strlen(CRLF)+1;
  pk->chead.p = (char*) malloc(_ss);
  pk->chead.n = snprintf(pk->chead.p, _ss, "%s%s%s", _sline_400[1], CRLF, CRLF);
  int nret = aio_write(cn->cfd, (void *) pk->chead.p, pk->chead.n, 0, ioCB);
  if (nret != 0) return svr_http_final(pk);
  
  return EHTTP_OK;
}


// FETCH URI [CacheTaskID]
ErrHTTP svr_http_fetch(HPKG * &pk) { 

  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_fetch] conn error\n"); return svr_http_final(pk); }

  if (DBGL >= 4) printf("[svr_http_fetch] hpkg=%x, cn=%x, met=%s, ver=%s, uri=%s\n", pk, cn, pk->met.p, pk->ver.p, pk->uri.p);

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

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W))  { printf("[svr_http_body_wio] conn error\n"); return svr_http_final(pk); }

  if (DBGL >= 4) printf("[svr_http_body_wio] hpkg=%x, cn=%x, nbytes=%d, data=%s\n", pk, cn, cd.size, cd.data);

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

  if (DBGL >= 4) printf("[svr_http_body_rio] hpkg=%x, wst=%d\n", pk, pk->wst);

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

  if (DBGL >= 4) printf("[svr_http_body_pio] hpkg=%x, put key=%s, size=%d, data=%s\n", pk, pk->bodykey.p, nbytes,  buf);

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

  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 3) assert(pk->cpn != NULL);

  Conn * & cn = *(pk->cpn); 

  if (DBGL >= 1) printf("[svr_http_final_aio] hpkg=%x, cn = %x,  cn->cst = %d, error code=%d\n", pk, cn, cn->cst, errcode);

  if (cn->cst != CS_CLOSING) {
    if ((pk->hst == HS_READING) || (pk->hst == HS_PARSING)) {
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

  if (DBGL >= 1) assert(pk != NULL);

  if (pk->headkey.p != NULL) _cache.doneWith(pk->headkey.p);
  if (pk->bodykey.p != NULL) _cache.doneWith(pk->bodykey.p);

  if (DBGL >= 1) assert(pk->cpn != NULL);  

  Conn * & cn = *(pk->cpn);

  bool bret = false; 
  HPKG ** call = pk->nsb; HPKG ** park = pk->ppg;

  delete pk;

  if (park != NULL) { // 
  
    // HAS More GET Task ?
    while ((call != NULL) && (bret == false)) {
      if (DBGL >= 1) assert(*call != NULL);
      // process the first GET
      bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, *call), CacheTaskID)); // [CacheTaskID] IN SERIAL !
      // if not enqueed ... 
      if (bret == false) {
	if (*((*park)->lcg) == (*call)) { pk->lcg = NULL; break; } // last GET
	HPKG ** next = (*call)->nsg;
	svr_http_final(*call);
	call = next;
	if (DBGL >= 1) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", *call); 
      }
    }
  
    // NO GET TASK !
    if (bret == false) {
      // continue "reading"
      if (((*(cn->cpp))->csp.size()) <= MaxKeepAlive) {
	printf("[svr_http_parse] pool size = %d\n", (*(cn->cpp))->csp.size());
	bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_read, pk), cn->cfd));  // TOTO: RReadTaskID
	if (bret == false) { 
	  if (DBGL >= 4) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", pk); 
	  return svr_http_final_aio(pk, cn->cfd, 0); }
      } else {
	if (DBGL >= 1) printf("[svr_http_parse] max pooled, cn=%x, hpkg=%x\n", cn, pk); 
	return svr_http_final_aio(pk, cn->cfd, 0);
      }
    }

  } 
  else { 
    // close the conn
    
    


  }


  if (DBGL >= 1) printf("[svr_http_final] hpkg = %x, cn = %x, cn->cst = %d, hpkg->c = %d (before --)\n", pk, cn, cn->cst, cn->pkgc);

  
 


  // pthread_mutex_lock(&(cn->pkgl));
  // --cn->pkgc;
  // pthread_mutex_unlock(&(cn->pkgl));

  if ((cn->pkgc == 0) && ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R) || (cn->cst == CS_CLOSING_W))) {
    if (DBGL >= 1) { printf("[svr_http_final] cn=%x, pool size = %d\n", cn, (*(cn->cpp))->csp.size() ); fflush(stdout); }
    svr_conn_close(cn);
  }

  //
  if (((*(cn->cpp))->csp.size()) <= MaxKeepAlive) { 
    printf("[svr_http_parse] pool size = %d\n", (*(cn->cpp))->csp.size());
    bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_read, pk), cn->cfd));  // TOTO: RReadTaskID
    if (bret == false) { 
      if (DBGL >= 4) printf("[svr_http_parse] dispatcher error, hpkg=%x\n", pk); 
      return svr_http_final_aio(pk, cn->cfd, 0); }
  } else {
    if (DBGL >= 1) printf("[svr_http_parse] max pooled, cn=%x, hpkg=%x\n", cn, pk); 
    return svr_http_final_aio(pk, cn->cfd, 0);
  }


  return EHTTP_FINAL;
}
