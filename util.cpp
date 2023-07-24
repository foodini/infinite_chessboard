#include <stdio.h>

#include "util.h"

void print_u128(u128 u, char end) {
  char buf[80];
  char * cur = buf + 79;
  *cur-- = '\0';
  *cur-- = end;
  do {
    *cur-- = '0' + (u % 10);
    u /= 10;
  } while(u);
  printf("%s", cur + 1);
}

void print_x128(u128 u, char end) {
  u64 mask = -1ULL;
  u64 least = u & mask;
  u64 most = u >> 64;
  printf("0x%08lx%08lx", most, least);
  printf("%c", end);
}

double now() {
  timeval tv;
  gettimeofday(&tv, NULL);

  double retval = tv.tv_sec;
  retval += tv.tv_usec/1000000.0;
  return retval;
}

bool progress_timer() {
  static double next_print_time = 0.0;
  double current_time = now();
  if(current_time > next_print_time) {
    next_print_time = current_time + 1.0;
    return true;
  }
  return false;
}
