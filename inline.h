/*
     X86-64 inline functions for MSB(), LSB() and PopCnt().  Note
     that these are 64 bit functions and they use 64 bit (quad-word)
     X86-64 instructions.
*/
/* *INDENT-OFF* */
#if defined(UNIX)
static __inline__ int MSB(uint64_t word) {
  uint64_t dummy, dummy2;

  asm(" 	 bsrq	 %1, %0     " "\n\t"
      :   "=&r"(dummy), "=&r" (dummy2)
      :   "1"((uint64_t) (word))
      :   "cc");
  return (dummy);
}
static __inline__ int LSB(uint64_t word) {
  uint64_t dummy, dummy2;

  asm(" 	 bsfq	 %1, %0     " "\n\t"
      :   "=&r"(dummy), "=&r" (dummy2)
      :   "1"((uint64_t) (word))
      :   "cc");
  return (dummy);
}
#if defined(POPCNT)
static __inline__ int PopCnt(uint64_t word) {
  uint64_t dummy, dummy2;

  asm(" 	 popcnt  %1, %0     " "\n\t"
      :   "=&r"(dummy), "=&r" (dummy2)
      :   "1"((uint64_t) (word))
      :   "cc");
  return (dummy);
}
#else
static __inline__ int PopCnt(uint64_t word) {
  uint64_t dummy, dummy2, dummy3;

asm("	       xorq    %0, %0	 " "\n\t"
    "	       testq   %1, %1	 " "\n\t"
    "	       jz      2f	 " "\n\t"
    "1:        leaq    -1(%1),%2 " "\n\t"
    "	       incq    %0	 " "\n\t"
    "	       andq    %2, %1	 " "\n\t"
    "	       jnz     1b	 " "\n\t"
    "2: 			 " "\n\t"
:   "=&r"(dummy), "=&r"(dummy2), "=&r"(dummy3)
:   "1"((uint64_t) (word))
:   "cc");
  return (dummy);
}
#endif
#else
#if defined(POPCNT)
#include <nmmintrin.h>
__forceinline int PopCnt(uint64_t a) {
  return _mm_popcnt_u64(a);
}
#else

__forceinline int PopCnt(uint64_t a) {
  int c = 1;

  if (!a)
    return 0;
  while (a &= a - 1)
    c++;
  return c;
}
#endif

__forceinline int MSB(uint64_t a) {
  int v;

  _BitScanReverse64(&v, a);
  return v;
}

__forceinline int LSB(uint64_t a) {
  int v;

  _BitScanForward64(&v, a);
  return v;
}
#endif
