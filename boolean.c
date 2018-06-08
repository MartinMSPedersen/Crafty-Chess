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
int PopCnt(register BITBOARD a) {
  register int c=0;

  while(a) {
    c++;
    a &= a - 1;
  }
  return(c);
}

int FirstOne(BITBOARD arg1) {
    if (arg1>>48)
      return (first_ones[arg1>>48]);
    if ((arg1>>32)&65535)
      return (first_ones[(arg1>>32)&65535]+16);
    if ((arg1>>16)&65535)
      return (first_ones[(arg1>>16)&65535]+32);
    return (first_ones[arg1&65535]+48);
}
  
int LastOne(BITBOARD arg1) {
    if (arg1&65535)
      return (last_ones[arg1&65535]+48);
    if ((arg1>>16)&65535)
      return (last_ones[(arg1>>16)&65535]+32);
    if ((arg1>>32)&65535)
      return (last_ones[(arg1>>32)&65535]+16);
    return (last_ones[arg1>>48]);
}
#endif
#endif
#endif
