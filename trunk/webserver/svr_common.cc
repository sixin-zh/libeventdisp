#include <svr_common.h>
//#include <syslog.h>

void print_times(const void * conn, const char * src, const char * dest, const timeval &prev, const timeval &curr, const timeval &prev_u, const timeval &curr_u, const timeval &prev_s, const timeval &curr_s, const size_t &qlen) {
  printf("%p %s %s %.6lf %.6lf %.6lf %zu\n", conn, src, dest, qlen,
	 ((double) (curr.tv_sec - prev.tv_sec)     + (double) (curr.tv_usec - prev.tv_usec) / 1000000.0) * 1000,
	 ((double) (curr_u.tv_sec - prev_u.tv_sec) + (double) (curr_u.tv_usec - prev_u.tv_usec) / 1000000.0) * 1000, 
	 ((double) (curr_s.tv_sec - prev_s.tv_sec) + (double) (curr_s.tv_usec - prev_s.tv_usec) / 1000000.0) * 1000);
}



void do_stat(const char * fpath) {
  printf("do_stat: %s\n", fpath);
  
  FILE * fstream = fopen(fpath, "r");
  
  void * conn; char src[MAXCLOC]; char dest[MAXCLOC]; size_t qlen; double time; double utime; double stime;
  
  while ( fscanf(fstream, "%p%s%s%lf%lf%lf%zu", &conn, src, dest, &qlen, &time, &utime, &stime) != EOF ) {
    printf("%s\n", src);
  }
    
  
  if (fstream != NULL) fclose(fstream);

}

