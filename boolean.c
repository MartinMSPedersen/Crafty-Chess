#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

#if !defined(HAS_64BITS)

BITBOARD Mask(int arg1)
{
  unsigned long long i;
  i=-1;
  if (arg1 == 128)
    return(0);
  else if (arg1 > 64)
    return(i>>(arg1-64));
  else
    return(i<<(64-arg1));
}

int Popcnt(register BITBOARD a)
{
  register int c;
  c = 0;
  while(a) {
    c++;
    a = a &~ -a;
  }
  return(c);
}

int Popcntl(BITBOARD arg1)
{
  union doub {
    unsigned short i[4];
    BITBOARD d;
  };
  union doub x;
  if (arg1) {
    x.d=arg1;
    return (population_count[x.i[0]]+
      population_count[x.i[1]]+
      population_count[x.i[2]]+
      population_count[x.i[3]]);
  }
  else
    return(0);
}

int First_One(BITBOARD arg1)
{
  union doub {
    unsigned short i[4];
    BITBOARD d;
  };
  union doub x;
  x.d=arg1;
#if defined(LITTLE_ENDIAN)
  if (x.i[3])
    return (first_ones[x.i[3]]);
  if (x.i[2])
    return (first_ones[x.i[2]]+16);
  if (x.i[1])
    return (first_ones[x.i[1]]+32);
  if (x.i[0]) 
    return (first_ones[x.i[0]]+48);
#endif
#if !defined(LITTLE_ENDIAN)
  if (x.i[0])
    return (first_ones[x.i[0]]);
  if (x.i[1])
    return (first_ones[x.i[1]]+16);
  if (x.i[2])
    return (first_ones[x.i[2]]+32);
  if (x.i[3]) 
    return (first_ones[x.i[3]]+48);
#endif
  return(64);
}

int Last_One(BITBOARD arg1)
{
  union doub {
    unsigned short i[4];
    BITBOARD d;
  };
  union doub x;
  x.d=arg1;
#if defined(LITTLE_ENDIAN)
  if (x.i[0]) 
    return (last_ones[x.i[0]]+48);
  if (x.i[1])
    return (last_ones[x.i[1]]+32);
  if (x.i[2])
    return (last_ones[x.i[2]]+16);
  if (x.i[3])
    return (last_ones[x.i[3]]);
#endif
#if !defined(LITTLE_ENDIAN)
  if (x.i[3]) 
    return (last_ones[x.i[3]]+48);
  if (x.i[2])
    return (last_ones[x.i[2]]+32);
  if (x.i[1])
    return (last_ones[x.i[1]]+16);
  if (x.i[0])
    return (last_ones[x.i[0]]);
#endif
  return(64);
}

#endif
