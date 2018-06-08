/*
     X86-64 inline functions for MSB(), LSB() and PopCnt().  Note
     that these are 64 bit functions and they use 64 bit (quad-word)
     X86-64 instructions.
*/
/* *INDENT-OFF* */
static __inline__ int MSB(long word)
{
  long dummy, dummy2;

asm("          bsrq    %1, %0     " "\n\t"
    "          jnz     1f         " "\n\t"
    "          movq    $64, %0    " "\n\t"
    "1:                           " "\n\t"
:   "=&r"(dummy), "=&r" (dummy2)
:   "1"((long) (word))
:   "cc");
  return (dummy);
}
static __inline__ int LSB(long word)
{
  long dummy, dummy2;

asm("          bsfq    %1, %0     " "\n\t"
    "          jnz     1f         " "\n\t"
    "          movq    $64, %0    " "\n\t"
    "1:                           " "\n\t"
:   "=&r"(dummy), "=&r" (dummy2)
:   "1"((long) (word))
:   "cc");
  return (dummy);
}
#if defined(POPCNT)
static __inline__ int PopCnt(long word)
{
  long dummy, dummy2;

asm("          popcnt  %1, %0     " "\n\t"
:   "=&r"(dummy), "=&r" (dummy2)
:   "1"((long) (word))
:   "cc");
  return (dummy);
}
#else
static __inline__ int PopCnt(long word)
{
  long dummy, dummy2, dummy3;

asm("          xorq    %0, %0    " "\n\t"
    "          testq   %1, %1    " "\n\t"
    "          jz      2f        " "\n\t"
    "1:        leaq    -1(%1),%2 " "\n\t"
    "          incq    %0        " "\n\t"
    "          andq    %2, %1    " "\n\t"
    "          jnz     1b        " "\n\t"
    "2:                          " "\n\t"
:   "=&r"(dummy), "=&r"(dummy2), "=&r"(dummy3)
:   "1"((long) (word))
:   "cc");
  return (dummy);
}
#endif
/* *INDENT-ON* */
