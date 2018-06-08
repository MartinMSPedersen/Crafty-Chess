#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

#if !defined(CRAY1)

BITBOARD Mask(int arg1) {
  register BITBOARD i;
  i=(BITBOARD) -1;
  if (arg1 == 128)
    return(0);
  else if (arg1 > 64)
    return(i>>(arg1-64));
  else
    return(i<<(64-arg1));
}

#if defined(MACOS)

int FirstOne(register BITBOARD a) {
  register unsigned long i;
  
  if (i = a >> 32)
    return(__cntlzw(i));
  if (i = (unsigned int) a)
    return(__cntlzw(i) + 32);
  return(64);
}
  
int LastOne(register BITBOARD a) {
  register unsigned long i;

  if (i = (unsigned int) a)
    return(__cntlzw(i ^ (i - 1)) + 32);
  if (i = a >> 32)
    return(__cntlzw(i ^ (i - 1)));
  return(64);
}

int PopCnt(register BITBOARD a) {
  register int c=0;

  while(a) {
    c++;
    a &= a - 1;
  }
  return(c);
}

#else
#if (!defined(USE_ASSEMBLY_B) && !defined(ALPHA)) || (defined(ALPHA) && !defined(PopCnt))

#if defined (_M_IA64)

#ifdef __ICL
typedef unsigned long long __m64;
#elif _MSC_VER >= 1300
typedef union __declspec(intrin_type) __declspec(align(8)) __m64
{
    unsigned __int64    m64_u64;
    float               m64_f32[2];
    __int8              m64_i8[8];
    __int16             m64_i16[4];
    __int32             m64_i32[2];
    __int64             m64_i64;
    unsigned __int8     m64_u8[8];
    unsigned __int16    m64_u16[4];
    unsigned __int32    m64_u32[2];
} __m64;
#endif

__m64 __m64_popcnt(__m64);
#pragma intrinsic (__m64_popcnt)

int PopCnt(register BITBOARD a) {
#ifdef __ICL
  return (int) __m64_popcnt(a);
#else
  __m64 m;

  m.m64_u64 = a;
  m = __m64_popcnt(m);
  return (int) m.m64_u64;
#endif
}

#else

int PopCnt(register BITBOARD a) {
  register int c=0;

  while(a) {
    c++;
    a &= a - 1;
  }
  return(c);
}

#endif

int FirstOne(BITBOARD arg1) {
    if (arg1>>48)
      return (first_one[arg1>>48]);
    if ((arg1>>32)&65535)
      return (first_one[(arg1>>32)&65535]+16);
    if ((arg1>>16)&65535)
      return (first_one[(arg1>>16)&65535]+32);
    return (first_one[arg1&65535]+48);
}
  
int LastOne(BITBOARD arg1) {
    if (arg1&65535)
      return (last_one[arg1&65535]+48);
    if ((arg1>>16)&65535)
      return (last_one[(arg1>>16)&65535]+32);
    if ((arg1>>32)&65535)
      return (last_one[(arg1>>32)&65535]+16);
    return (last_one[arg1>>48]);
}
#endif
#endif
#endif
