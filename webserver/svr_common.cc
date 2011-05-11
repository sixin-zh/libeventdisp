#include <svr_common.h>
//#include <syslog.h>

double convert_tim_sec(const timeval &tim) { // double [sec]
  return (double) (tim.tv_sec + tim.tv_usec / 1000000.0);
}


void print_times(const void * conn, const char * src, const char * dest, const timeval &prev, const timeval &curr, const timeval &prev_u, const timeval &curr_u, const timeval &prev_s, const timeval &curr_s, const size_t &qlen) {
  printf("%zu, %p, %s, %s, %.6lf, %.6lf, %.6lf, ", qlen, conn, src, dest,
	 convert_tim_sec(curr) - convert_tim_sec(prev),
	 convert_tim_sec(curr_u) - convert_tim_sec(prev_u),
	 convert_tim_sec(curr_s) - convert_tim_sec(prev_s));
}



void do_stat(const char * fpath) {
  printf("do_stat: %s\n", fpath);
  
  FILE * fstream = fopen(fpath, "r");
  
  // void * conn; char src[MAXCLOC]; char dest[MAXCLOC]; size_t qlen; double time; double utime; double stime;
  
  // while ( fscanf(fstream, "%p%s%s%lf%lf%lf%zu", &conn, src, dest, &qlen, &time, &utime, &stime) != EOF ) {
  //   // printf("%s\n", src);
  // }
  
  if (fstream != NULL) fclose(fstream);

}

