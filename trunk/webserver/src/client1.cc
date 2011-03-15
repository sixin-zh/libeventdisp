#include <websvrd.h>

int main() {
  
  // Init socket, address
  int svr_fd; // register
  struct sockaddr_in svr_sa;

  if ((svr_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    err_std("[fail] client init.\n");
    return -1;
  }

  bzero(&svr_sa, sizeof svr_sa);
  svr_sa.sin_family      = AF_INET;
  svr_sa.sin_port        = SVR_PORT;
  svr_sa.sin_addr.s_addr = SVR_IPV4;

  // Connect to server
  if (connect(svr_fd, (SA *) &svr_sa, sizeof svr_sa) < 0) {
    err_std("[fail] connect to server.\n");
    close(svr_fd);
    return -2;
  }
  
  // Simple HTTP request
  const char reqstr[] = "GET / HTTP/1.1\n";
  // printf("%s: %lu\n", reqstr, sizeof reqstr);

  write(svr_fd, reqstr, strlen(reqstr)+1);
  
  char replybuff[1024];
  read(svr_fd, replybuff, 1024);
  printf("[server reply]\n%s\n[the end]\n", replybuff);

  close(svr_fd);

  return 0;
}
