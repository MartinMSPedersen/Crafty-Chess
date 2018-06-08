extern unsigned char  first_ones[65536];
extern unsigned char  last_ones[65536];

#if _MSC_VER >= 1200
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif


FORCEINLINE int PopCnt(BITBOARD a) {

/* Because Crafty bitboards are typically sparsely populated, we use a
   streamlined version of the boolean.c algorithm instead of the one in x86.s */

  __asm {
        mov     ecx, dword ptr a
        xor     eax, eax
        test    ecx, ecx
        jz      l1
    l0: lea     edx, [ecx-1]
        inc     eax
        and     ecx, edx
        jnz     l0
    l1: mov     ecx, dword ptr a+4
        test    ecx, ecx
        jz      l3
    l2: lea     edx, [ecx-1]
        inc     eax
        and     ecx, edx
        jnz     l2
    l3: 
  }
}


FORCEINLINE int FirstOne(BITBOARD a) {

#if _M_IX86 <= 500 /* on plain Pentiums, use boolean.c algorithm */  
  __asm {
        movzx   edx, word ptr a+6
        xor     eax, eax
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a+4
        mov     eax, 16
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a+2
        mov     eax, 32
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a
        mov     eax, 48
  l1:   add     al, byte ptr first_ones[edx]
  }
#else /* BSF and BSR are *fast* instructions on PPro/PII */
  __asm {
        bsr     edx, dword ptr a+4
        mov     eax, 31
        jnz     l1
        bsr     edx, dword ptr a
        mov     eax, 63
        jnz     l1
        mov     edx, -1
  l1:   sub     eax, edx
  }
#endif /* _M_IX86 > 500 */
}

FORCEINLINE int LastOne(BITBOARD a) {

#if _M_IX86 <= 500 /* on plain Pentiums, use boolean.c algorithm */  
  __asm {
        movzx   edx, word ptr a
        mov     eax, 48
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a+2
        mov     eax, 32
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a+4
        mov     eax, 16
        test    edx, edx
        jnz     l1
        mov     dx, word ptr a+6
        xor     eax, eax
        test    edx, edx
        jnz     l1
        mov     eax, 48
l1:     add     al, byte ptr last_ones[edx] 
  }
#else /* BSF and BSR are *fast* instructions on PPro/PII */
  __asm {
        bsf     edx, dword ptr a
        mov     eax, 63
        jnz     l1
        bsf     edx, dword ptr a+4
        mov     eax, 31
        jnz     l1
        mov     edx, -33
  l1:   sub     eax, edx
  }
#endif /* _M_IX386 > 500 */
}
