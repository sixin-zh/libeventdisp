#include <websvrd.h>

int main() {

  register int svr_fd;
  struct sockaddr_in svr_sa;


  if ((svr_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    err_std("[client] socket error\n");
    return -1;
  }

  bzero(&svr_sa, sizeof svr_sa);

  svr_sa.sin_family = AF_INET;
  svr_sa.sin_port = SVR_PORT;
  svr_sa.sin_addr.s_addr = SVR_IPV4;


  if (connect(svr_fd, (SA *) &svr_sa, sizeof svr_sa) < 0) {
    err_std("[client] connect error\n");
    close(svr_fd);
    return -2;
  }
  
  FILE *svr_io;
  if ((svr_io = fdopen(svr_fd, "wb")) == NULL) {
    err_std("\t[client] open server failed.\n");
    close(svr_fd);
    return -3;
  }

  fprintf(svr_io, "HTTP/1.1 200 OK\n");

  
  fclose(svr_io);
  close(svr_fd);

  return 0;
}
