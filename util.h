#ifndef _UTIL_H
#define _UTIL_H
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

extern const u8 u8_min;
extern const u8 u8_max;

extern const s8 s8_min;
extern const s8 s8_max;

extern const u16 u16_min;
extern const u16 u16_max;

extern const s16 s16_min;
extern const s16 s16_max;

extern const u32 u32_min;
extern const u32 u32_max;

extern const s32 s32_min;
extern const s32 s32_max;

extern const u64 u64_min;
extern const u64 u64_max;

extern const s64 s64_min;
extern const s64 s64_max;

extern const u128 u128_min;
extern const u128 u128_max;

extern const s128 s128_min;
extern const s128 s128_max;

void print_u128(u128 u, char end='\0');
void print_x128(u128 u, char end='\0');
void print_s128(s128 u, char end='\0');
double now();
bool progress_timer();

#endif // __UTIL_H
