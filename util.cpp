#include <stdio.h>

#include "util.h"

u8 u8_min = 0x00;
u8 u8_max = 0xff;
s8 s8_min = -128;
s8 s8_max = 127;
u16 u16_min = 0x0000;
u16 u16_max = 0xffff;
s16 s16_min = 0x8000;
s16 s16_max = 0x7fff;
u32 u32_min = 0x00000000;
u32 u32_max = 0xffffffff;
s32 s32_min = 0x80000000;
s32 s32_max = 0x7fffffff;
u64 u64_min = 0x0000000000000000UL;
u64 u64_max = 0xffffffffffffffffUL;
s64 s64_min = 0x8000000000000000UL;
s64 s64_max = 0x7fffffffffffffffUL;
u128 u128_min = 0;
u128 u128_max = ~((s128)1);
s128 s128_min = ((s128)1) << 127;
s128 s128_max = s128_min - 1;

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

void print_s128(s128 s, char end) {
  if(s<0) {
    printf("-");
    s = -s;
  }
  print_u128(s, end);
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
