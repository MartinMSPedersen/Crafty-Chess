/*
     AMD Opteron inline functions for FirstOne(), LastOne() and
     PopCnt().  Note that these are 64 bit functions and they use
     64 bit (quad-word) X86-64 instructions.
*/
int static __inline__ FirstOne(long word)
{
  long      dummy, dummy2;

asm("          bsrq    %0, %1"                       "\n\t"
    "          jnz     1f"                           "\n\t"
    "          movq    $-1, %1"                      "\n\t"
    "1:        movq    $63, %0"                      "\n\t"
    "          subq    %1, %0"                       "\n\t"
    :"=&r"(dummy), "=&r" (dummy2)
    :"0"((long) (word))
    :"cc");
  return (dummy);
}

int static __inline__ LastOne(long word)
{
  long      dummy, dummy2;

asm("          bsfq    %0, %1"                       "\n\t"
    "          jnz     1f"                           "\n\t"
    "          movq    $-1, %1"                      "\n\t"
    "1:        movq    $63, %0"                      "\n\t"
    "          subq    %1, %0"                       "\n\t"
    :"=&r"(dummy), "=&r" (dummy2)
    :"0"((long) (word))
    :"cc");
  return (dummy);
}

int static __inline__ PopCnt(long word)
{
  long      dummy, dummy2, dummy3;

asm("          xorq    %0, %0"                      "\n\t"
    "          testq   %1, %1"                      "\n\t"
    "          jz      2f"                          "\n\t"
    "1:        leaq    -1(%1),%2"                   "\n\t"
    "          incq    %0"                          "\n\t"
    "          andq    %2, %1"                      "\n\t"
    "          jnz     1b"                          "\n\t"
    "2:                      "                      "\n\t"
    :"=&r"(dummy), "=&r"(dummy2), "=&r" (dummy3)
    :"1"((long) (word))
    :"cc");
  return (dummy);
}
