#include <sys/time.h>

typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;
typedef unsigned long u64;
typedef long s64;
typedef __uint128_t u128;
typedef __int128_t s128;

void print_u128(u128 u, char end='\0');
void print_x128(u128 u, char end='\0');
double now();
bool progress_timer();
