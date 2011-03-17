#include <websvrd.h>

int main() {
  
  // Init socket, address
  int svr_fd; // register
  struct sockaddr_in svr_sa;

  if ((svr_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    err_std("[client] init fail.\n");
    return -1;
  }

  bzero(&svr_sa, sizeof svr_sa);
  svr_sa.sin_family      = AF_INET;
  svr_sa.sin_port        = SVR_PORT;
  svr_sa.sin_addr.s_addr = SVR_IPV4;

  // Connect to server
  if (connect(svr_fd, (SVR_SA *) &svr_sa, sizeof svr_sa) < 0) {
    err_std("[client] fail to connect server: %d\n", svr_fd);    
    close(svr_fd);
    return -2;
  }
  
  // Simple HTTP request
  const char reqstr[] = "GET /paul.ape HTTP/1.1\nHost: localhost\n\n";
  printf("[sent]\n%s\n", reqstr);

  write(svr_fd, reqstr, strlen(reqstr)+1);
  
  char replybuff[1024];
  memset(replybuff, 0, sizeof replybuff);
  int n = read(svr_fd, replybuff, 1024);
  printf("[server reply %d bytes]\n%s[the end]\n", n, replybuff);

  close(svr_fd);

  return 0;
}
