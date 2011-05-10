#include <svr_common.h>
//#include <syslog.h>


void print_times(char * src, char * dest, timeval &prev, timeval &curr, timeval &prev_u, timeval &curr_u, timeval &prev_s, timeval &curr_s) {

  printf("[%s -> %s] ", src, dest); 
  
  printf("time=%lf, ", (double) (curr.tv_sec - prev.tv_sec) + (double) (curr.tv_usec - prev.tv_usec) / 1000000.0);
  printf("utime=%lf, stime=%lf\n", (double) (curr_u.tv_sec - prev_u.tv_sec) + (double) (curr_u.tv_usec - prev_u.tv_usec) / 1000000.0, 
	                           (double) (curr_s.tv_sec - prev_s.tv_sec) + (double) (curr_s.tv_usec - prev_s.tv_usec) / 1000000.0);

}
