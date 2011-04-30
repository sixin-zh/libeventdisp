#include <time.h>

#include <svr_common.h>
#include <svr_http.h>
#include <svr_tcp_ip.h>

// #define CTLR "\n"
// including '\0'
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
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R)) return svr_http_final(pk);

  pk->hst = HS_READING;

  // read header, assume no body, footer...
  charn & _tmp = pk->chead;
  if (_tmp.p == NULL) { _tmp.n = 0; _tmp.p = new char[MAXRH]; }
  memset(_tmp.p, 0, sizeof(char)*MAXRH);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_parse_aio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, cn->cfd); // PALL or RReadTaskID ?

  int iret = aio_read(cn->cfd, static_cast<void *> (_tmp.p), MAXRH, 0, ioCB); // end with '\0' 
  if (iret != 0) return svr_http_final(pk);

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
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R)) return svr_http_final(pk);

  pk->hst = HS_PARSING;
  char * ph = (char *)  pk->chead.p; // head
  char * pe = ph + pk->chead.n - 1;  // end
  while (ph <= pe) {
    // parse line by line
    if (DBGL >= 6) printf("[svr_http_parse] line (%s)\n", ph);
    char * pt = strchr(ph, '\n');
    if (pt==NULL) pt = pe;

    // GET
    if (strncmp(ph,"GET ",4)==0) { // "GET " + URI + ' ' + "HTTP/1.*"

      // create new hpkg
      HPKG * gk = new HPKG(cn);

      // met
      if (DBGL >= 6) printf("[svr_http_parse] met (%s)\n", ph);
      gk->met.n = 3;
      gk->met.p = new char[gk->met.n+1]; // end with '\0'
      strcpy(gk->met.p, "GET");

      // uri
      ph = ph + 4;  // URI + ' ' + "HTTP/1.*"
      if (DBGL >= 6) printf("[svr_http_parse] uri (%s)\n", ph);
      char * ps = strchr(ph, ' ');
      if (*ph == '/') ++ph; // skip /
      if ((ps != NULL) && (ps < pt)) {
	gk->uri.n = ps-ph;
	gk->uri.p = new char[gk->uri.n+1]; // end with '\0'
	memcpy(gk->uri.p, ph, gk->uri.n); 
	gk->uri.p[gk->uri.n] = '\0';
      
	// ver
	ph = ps + 1;
	if (DBGL >= 6) printf("[svr_http_parse] ver (%s)\n", ph);
	if (ph <= pt) {
	  gk->ver.n = pt-ph;
	  gk->ver.p = new char[gk->ver.n+1]; // end with '\0'
	  memcpy(gk->ver.p, ph, gk->ver.n);
	  gk->ver.p[gk->ver.n] = '\0';
	}
      } // else bad GET request (no uri or no ver)

      if (DBGL >= 6) printf("[svr_http_parse] endline (met=%s, ver=%s, uri=%s)\n", pk->met.p, pk->ver.p, pk->uri.p);

      // fetch GET
      bool bret = Dispatcher::instance()->enqueue(new UnitTask(BIND(svr_http_fetch, gk), CacheTaskID)); //// IN SERIAL !
      if (bret == false) svr_http_final(gk);

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
  if (DBGL >= 4) printf("[svr_http_parse_aio] new request header (hpkg=%x, fd=%d, nbytes=%d):\n%s\n", pk, fd, nbytes, pk->chead.p);

  Conn * &cn = *(pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R)) return svr_http_final(pk);

  // HS_PARSING;
  pk->chead.p = (char *) buf;
  pk->chead.n = nbytes-1; // '\0 not counted

  // continue reading new request if not EOF
  if (nbytes == 0) // EOF
    return svr_http_final_aio(pk, fd, 0);
  else {
    bool bret = Dispatcher::instance()->enqueue( new UnitTask(BIND(svr_http_read, pk), cn->cfd));  // TODO: RReadTaskID
    if (bret == false) return svr_http_final(pk);
  }
  
  return svr_http_parse(pk);
}


// FETCH; PUT; CANCEL [CacheTaskID]
ErrHTTP svr_http_header_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  { // open local file
    pk->hfd = open(pk->uri.p, O_RDONLY); // ! blocking

    if (pk->hfd < 0) { // 404

      pk->hsn = N_404;

      // fetch
      char * _tmp = (char*) malloc (24);
      strncpy(_tmp, _sline_404[pk->vern], 23);
      pk->chead.p = _tmp; pk->chead.n = 23; pk->chead.cached = true;
      // put
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);

    } // end 404
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
      strncpy(_tmp.p, _sline_200[pk->vern], 16);
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
      strcat(_tmp.p, "\n"); // _tmp.n = strlen(_tmp.p);
      // end of head

      if (DBGL >= 5) printf("[svr_http_fetch] hpkg=%x, head[%d] = \n%s\n", pk, pk->chead.n, pk->chead.p);

      // PUT HEAD
      bool isput = _cache.put(pk->headkey.p, pk->chead.p, pk->chead.n);
      if (DBGL >= 3) assert(isput == true);

    } // end 200
    
  } // end open

  return EHTTP_OK;
}


// [CacheTaskID]
ErrHTTP svr_http_header_to_body_aio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  pk->hst = HS_UNKNOWN;
  pk->wst = WS_HEAD;

  //  if (pk->headkey.p != NULL){  _cache.doneWith(pk->headkey.p); delete[] pk->headkey.p; pk->headkey.p = NULL; }

  // body
  if (pk->hsn == N_200)
    return svr_http_body(pk);
  
  return svr_http_final(pk);
    
}


// Got Head [CacheTaskID]
ErrHTTP svr_http_header_wio(HPKG * &pk, const CacheData &cd) {

  if (DBGL >= 4) printf("[svr_http_header_wio] (hpkg=%x, nbytes=%d)\n", pk, cd.size);
  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  pk->hst = HS_WRITING;
  pk->wst = WS_HEAD;

  // hsn
  const char * loc = cd.data+9;
  if (strncmp(loc, "200", 3))
    pk->hsn = N_200;
  else if (strncmp(loc, "404", 3))
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
  
  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  pk->hst = HS_FETCHING;
  pk->wst = WS_HEAD;

  CacheCallback * cCB = new CacheCallback(BIND(svr_http_header_wio, pk, _1));
  
  if (pk->headkey.p == NULL) { pk->headkey.p = new char[MAXKEYS]; }
  strncpy(pk->headkey.p, pk->uri.p, MAXKEYS-1);
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
  if (DBGL >= 4) printf("[svr_http_fetch] hpkg (met=%s, ver=%s, uri=%s)\n", pk->met.p, pk->ver.p, pk->uri.p);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

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


// GOT BODY [CacheTaskID]
ErrHTTP svr_http_body_wio(HPKG * &pk,  const CacheData &cd) {
  if (DBGL >= 4) printf("[svr_http_fetch_aio] (hpkg=%x, nbytes=%d)\n", pk, cd.size);
  if (DBGL >= 3) assert(pk != NULL);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

  IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_rio, pk, _1, _2, _3));
  IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
  IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
  int nret = aio_write(cn->cfd, const_cast<char *> (cd.data), cd.size, 0, ioCB); 
  if (nret != 0) return svr_http_final(pk);

  return EHTTP_OK;
}

// GOT BODY
// ErrHTTP svr_http_fetch_aio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {
//   if (DBGL >= 4) printf("[svr_http_fetch_aio] (hpkg=%x, nbytes=%d)\n", pk, nbytes);
//   if (DBGL >= 3) assert(pk != NULL);

//   Conn * &cn = (*pk->cpn);
//   if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) return svr_http_final(pk);

//   IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_write_aio, pk, _1, _2, _3));
//   IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
//   IOCallback    * ioCB  = new IOCallback(okCB, errCB, cn->cfd);
//   int nret = aio_write(cn->cfd, buf, nbytes, 0, ioCB);
//   if (nret != 0) return svr_http_final(pk);

//   return EHTTP_OK;
// }


// WRITE DONE [CacheTaskID]
ErrHTTP svr_http_body_rio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  if (DBGL >= 4) printf("[svr_http_write_aio] (hpkg=%x, wst=%d)\n", pk, pk->wst);

  if (pk->wst == WS_BODY) {
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

ErrHTTP svr_http_body_pio(HPKG * &pk, int &fd, void * buf, const size_t &nbytes) {

  bool iscac = _cache.put(pk->bodykey.p, static_cast<const char *> (buf), nbytes); 
  if (DBGL >= 2) assert(iscac == true); 
  pk->chead.cached = iscac;

  return EHTTP_OK;
}


// FETCH BODY [CacheTaskID]
ErrHTTP svr_http_body(HPKG * &pk) {
  
  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 4) printf("[svr_http_write] (hpkg=%x, wst=%d)\n", pk, pk->wst);

  Conn * &cn = (*pk->cpn);
  if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) { printf("[svr_http_write] conn error\n"); return svr_http_final(pk); }

  pk->hst = HS_FETCHING;
  pk->wst = WS_BODY;
  
  if ((pk->hsn = N_200) && (pk->enc == TE_IDE)) {
      // new key
      if (pk->bodykey.p == NULL) { pk->bodykey.p = new char[MAXKEYS]; pk->bodykey.cached = false; }
      pk->bodykey.n = snprintf(pk->bodykey.p, MAXKEYS, "%s#%d", pk->uri.p, pk->tr_offset);
      if (DBGL >= 3) printf("[200 bodykey]: %s [%d]\n", pk->bodykey.p, pk->bodykey.n);

      // get
      CacheCallback * cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
      if (!_cache.get(pk->bodykey.p, cCB)) {
	bool rsvp = _cache.reserve(pk->bodykey.p);
	if (DBGL >= 2) assert(rsvp == true);
	cCB = new CacheCallback(BIND(svr_http_body_wio, pk, _1));
	bool iscac = _cache.get(pk->bodykey.p, cCB);
	if (DBGL >= 2) assert(iscac == true); // return svr_http_final(pk);
	
	if (pk->cbody.p == NULL) { pk->cbody.p = new char[MAXWB]; }

	IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_body_pio, pk, _1, _2, _3));
	IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
	IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
	int nret = aio_read(pk->hfd, static_cast<void *> (pk->cbody.p), MAXWB, pk->tr_offset, ioCB);
	if (nret != 0) return svr_http_final(pk);
      }
  } else {
    return svr_http_final(pk);
  }

  return EHTTP_OK;
}


// // [CacheTaskID]
// ErrHTTP svr_http_write(HPKG * &pk) {

//   if (DBGL >= 3) assert(pk != NULL);
//   if (DBGL >= 4) printf("[svr_http_write] (hpkg=%x, wst=%d)\n", pk, pk->wst);

//   Conn * &cn = (*pk->cpn);
//   if ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_W)) {
//     printf("[svr_http_write] i/o error\n");
//     return svr_http_final(pk);
//   }

//   pk->hst = HS_WRITING;

//   if (pk->hsn == N_400) { // BEGIN 400
//     if (DBGL >= 5) printf("[svr_http_write] hsn=400, wst = %d\n", pk->wst);
//     if (pk->wst == WS_HEAD) {
//       // HTTP/1.* 400 Bad Request
//       IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_write_aio, pk, _1, _2, _3));
//       IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
//       IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
//       int nret = aio_write(cn->cfd, static_cast<void *> (pk->chead.p), pk->chead.n+1, 0, ioCB);
//       if (nret != 0) return svr_http_final(pk);
//     } else {
//       return svr_http_final(pk);
//     }
//   } // END 400
//   else if (pk->hsn == N_404) { // BEGIN 404
//     if (DBGL >= 5) printf("[svr_http_write] hsn=404, wst = %d\n", pk->wst);
//     if (pk->wst == WS_HEAD) {
//       // HTTP/1.* 404 Not Found
//       IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_write_aio, pk, _1, _2, _3));
//       IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
//       IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
//       if (DBGL >= 5) printf("[svr_http_write] aio_write: %s", _sline_404[pk->vern]);
//       int nret = aio_write(cn->cfd, static_cast<void *> (pk->chead.p), pk->chead.n+1, 0, ioCB);
//       if (nret != 0) return svr_http_final(pk);
//     } else {
//       return svr_http_final(pk);
//     }
//   } // END 404
//   else if (pk->hsn == N_200) { // BEGIN 200

//     if (DBGL >= 5) printf("[svr_http_write] hsn=200, vern = %d, wst = %d, hfd=%d, offs=%d \n", pk->vern, pk->wst, pk->hfd, pk->tr_offset);
    
//     if (pk->wst == WS_HEAD) {
//       IOOkCallback  * okCB  = new IOOkCallback(BIND(svr_http_write_aio, pk, _1, _2, _3));
//       IOErrCallback * errCB = new IOErrCallback(BIND(svr_http_final_aio, pk, _1, _2));
//       IOCallback    * ioCB  = new IOCallback(okCB, errCB, CacheTaskID); // [CacheTaskID]
//       if (DBGL >= 5) printf("[svr_http_write] aio_write[%d]:\n%s", pk->chead.n, (pk->chead).p);
//       int nret = aio_write(cn->cfd, static_cast<void *> (pk->chead.p), pk->chead.n+1, 0, ioCB);
//       if (nret != 0) return svr_http_final(pk);
//     }
//     else if (pk->wst == WS_BODY) {
//       if (pk->enc == TE_IDE) { // TODO: check in fetch_aio
// 	// new key
// 	if (pk->bodykey.p == NULL) { pk->bodykey.p = new char[MAXKEYS]; pk->bodykey.cached = false; }
// 	pk->bodykey.n = snprintf(pk->bodykey.p, MAXKEYS, "%s#%d", pk->uri.p, pk->tr_offset);
// 	printf("[200 bodykey]: %s [%d]\n", pk->bodykey.p, pk->bodykey.n);
	
// 	// get
// 	CacheCallback * cCB = new CacheCallback(BIND(svr_http_write_wio, pk, _1));
// 	if (!_cache.get(pk->bodykey.p, cCB)) {
// 	  bool rsvp = _cache.reserve(pk->bodykey.p);
// 	  if (DBGL >= 2) assert(rsvp == true);
// 	  cCB = new CacheCallback(BIND(svr_http_write_wio, pk, _1));
// 	  bool iscac = _cache.get(pk->bodykey.p, cCB);
// 	  if (DBGL >= 2) assert(iscac == true); // return svr_http_final(pk);
	  
// 	}
//       } else {
// 	return svr_http_final(pk);
//       }
//     }
//     else {
//       return svr_http_final(pk);
//     }
//   } // END 200
//   else {
//     printf("[svr_http_write] final called UNO\n");
//     return svr_http_final(pk);
//   } // END HSN

//   return EHTTP_OK;
// }


// [CacheTaskID]
ErrHTTP svr_http_final_aio(HPKG * &pk, int & fd, const int &errcode) {

  if (DBGL >= 4) printf("[svr_http_final_aio] hpkg=%x, fd=%d, error code=%d\n", pk, fd, errcode);
  if (DBGL >= 3) assert(pk != NULL);
  if (DBGL >= 3) assert(pk->cpn != NULL);

  Conn * & cn = *(pk->cpn); 

  if (cn->cst != CS_CLOSING) {
    if (pk->hst == HS_READING) {
      pthread_mutex_lock(&(cn->pkgl));
      if (cn->cst == CS_CLOSING_W)
  	cn->cst = CS_CLOSING;
      else
  	cn->cst = CS_CLOSING_R; // TODO: shutdown READ
      pthread_mutex_unlock(&(cn->pkgl));
    } 
    else if (pk->hst == HS_WRITING) {
      pthread_mutex_lock(&(cn->pkgl));
      if (cn->cst == CS_CLOSING_R)
  	cn->cst = CS_CLOSING;
      else
  	cn->cst = CS_CLOSING_W; // TODO: shutdown WRITE
      pthread_mutex_unlock(&(cn->pkgl));
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
  if (DBGL >= 3) printf("[svr_http_final] hpkg->c = %d (before --)\n", cn->pkgc);
  --cn->pkgc;
  if ((cn->pkgc == 0) && ((cn->cst == CS_CLOSING) || (cn->cst == CS_CLOSING_R) || (cn->cst == CS_CLOSING_W))) 
    // TODO: lock svr's conn pool
    svr_conn_close(cn); // TODO: delete the lock (no need for unlock ?)
  else
    pthread_mutex_unlock(&(cn->pkgl));

  return EHTTP_FINAL;
}
