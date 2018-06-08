#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

#if !defined(CRAY1)

BITBOARD Mask(int arg1)
{
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

  int FirstOne(register BITBOARD a)
	{
		register unsigned long i;
		
		if (i = a >> 32)
			return(__cntlzw(i));
		if (i = a & 0xffffffff)
			return(__cntlzw(i) + 32);
		return(64);
	}
  
  int LastOne(register BITBOARD a)
	{
		register unsigned long i;
		
		if (i = a & 0xffffffff)
			return(__cntlzw(i ^ (i - 1)) + 32);
		if (i = a >> 32)
			return(__cntlzw(i ^ (i - 1)));
		return(64);
	}

  int PopCnt(register BITBOARD a)
  {
    register int c=0;

    while(a) {
      c++;
      a &= a - 1;
    }
    return(c);
  }


#else
#if !defined(USE_ASSEMBLY_B)
  int PopCnt(register BITBOARD a)
  {
    register int c=0;

    while(a) {
      c++;
      a &= a - 1;
    }
    return(c);
  }

  int FirstOne(BITBOARD arg1)
  {
    union doub {
      unsigned short i[4];
      BITBOARD d;
    };
    register union doub x;
    x.d=arg1;
#  if defined(LITTLE_ENDIAN_ARCH)
    if (x.i[3])
      return (first_ones[x.i[3]]);
    if (x.i[2])
      return (first_ones[x.i[2]]+16);
    if (x.i[1])
      return (first_ones[x.i[1]]+32);
    if (x.i[0]) 
      return (first_ones[x.i[0]]+48);
#  endif
#  if !defined(LITTLE_ENDIAN_ARCH)
    if (x.i[0])
      return (first_ones[x.i[0]]);
    if (x.i[1])
      return (first_ones[x.i[1]]+16);
    if (x.i[2])
      return (first_ones[x.i[2]]+32);
    if (x.i[3]) 
      return (first_ones[x.i[3]]+48);
#  endif
    return(64);
  }
  
  int LastOne(BITBOARD arg1)
  {
    union doub {
      unsigned short i[4];
      BITBOARD d;
    };
    register union doub x;
    x.d=arg1;
#  if defined(LITTLE_ENDIAN_ARCH)
    if (x.i[0]) 
      return (last_ones[x.i[0]]+48);
    if (x.i[1])
      return (last_ones[x.i[1]]+32);
    if (x.i[2])
      return (last_ones[x.i[2]]+16);
    if (x.i[3])
      return (last_ones[x.i[3]]);
#  endif
#  if !defined(LITTLE_ENDIAN_ARCH)
    if (x.i[3]) 
      return (last_ones[x.i[3]]+48);
    if (x.i[2])
      return (last_ones[x.i[2]]+32);
    if (x.i[1])
      return (last_ones[x.i[1]]+16);
    if (x.i[0])
      return (last_ones[x.i[0]]);
#  endif
    return(64);
  }
#  endif
#endif
#endif
