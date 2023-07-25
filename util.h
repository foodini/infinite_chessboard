#ifndef _UTIL_H
#define _UTIL_H
#include <sys/time.h>

typedef unsigned char u8;
extern const u8 u8_min;
extern const u8 u8_max;
typedef char s8;
extern const s8 s8_min;
extern const s8 s8_max;
typedef unsigned short u16;
extern const u16 u16_min;
extern const u16 u16_max;
typedef short s16;
extern const s16 s16_min;
extern const s16 s16_max;
typedef unsigned int u32;
extern const u32 u32_min;
extern const u32 u32_max;
typedef int s32;
extern const s32 s32_min;
extern const s32 s32_max;
typedef unsigned long u64;
extern const u64 u64_min;
extern const u64 u64_max;
typedef long s64;
extern const s64 s64_min;
extern const s64 s64_max;
typedef __uint128_t u128;
extern const u128 u128_min;
extern const u128 u128_max;
typedef __int128_t s128;
extern const s128 s128_min;
extern const s128 s128_max;

void print_u128(u128 u, char end='\0');
void print_x128(u128 u, char end='\0');
void print_s128(s128 u, char end='\0');
double now();
bool progress_timer();

#endif // __UTIL_H
