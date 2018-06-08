/*
     Intel X86oinline functions for FirstOne(), LastOne() and
     PopCnt().  Note that these are 64 bit functions and they use
     32 bit (double-word) X86 instructions.
*/
int static __inline__ PopCnt(BITBOARD word)
{
/*  r0=result, %1=tmp, %2=first input, %3=second input */
  long      dummy, dummy2;

asm("        xorl    %0, %0"                    "\n\t"
    "        testl   %2, %2"                    "\n\t"
    "        jz      2f"                        "\n\t"
    "1:      leal    -1(%2), %1"                "\n\t"
    "        incl    %0"                        "\n\t"
    "        andl    %1, %2"                    "\n\t"
    "        jnz     1b"                        "\n\t"
    "2:      testl   %3, %3"                    "\n\t"
    "        jz      4f"                        "\n\t"
    "3:      leal    -1(%3), %1"                "\n\t"
    "        incl    %0"                        "\n\t"
    "        andl    %1, %3"                    "\n\t"
    "        jnz     3b"                        "\n\t"
    "4:"                                        "\n\t"
  : "=&q" (dummy), "=&q" (dummy2)
  : "q" ((int) (word>>32)), "q" ((int) word)
  : "cc");
  return (dummy);
}

int static __inline__ FirstOne(BITBOARD word) {
  int dummy, dummy2;
       asm ("movl    $-1, %1"             "\n\t"
            "bsr     %3, %0"              "\n\t"
            "cmovz   %1, %0"              "\n\t"
            "bsr     %2, %1"              "\n\t"
	    "setnz   %b3"                 "\n\t"
	    "addl    $32,%1"              "\n\t"
	    "testb   %b3, %b3"            "\n\t"
            "cmovz   %0, %1"              "\n\t"
            "movl    $63, %0"             "\n\t"
            "subl    %1, %0"              "\n\t"
  : "=&q" (dummy), "=&q" (dummy2)
  : "q" ((int) (word>>32)), "q" ((int) word)
  : "cc");
  return (dummy);
}
int static __inline__ LastOne(BITBOARD word) {
  int dummy, dummy2;
       asm ("movl    $-1, %1"             "\n\t"
            "bsf     %2, %0"              "\n\t"
            "setnz   %b2"                 "\n\t"
            "addl    $32,%0"              "\n\t"
            "testb   %b2, %b2"            "\n\t"
            "cmovz   %1, %0"              "\n\t"
            "bsf     %3, %1"              "\n\t"
            "cmovz   %0, %1"              "\n\t"
            "movl    $63, %0"             "\n\t"
            "subl    %1, %0"              "\n\t"
  : "=&q" (dummy), "=&q" (dummy2)
  : "q" ((int) (word>>32)), "q" ((int) word)
  : "cc");
  return (dummy);
}
