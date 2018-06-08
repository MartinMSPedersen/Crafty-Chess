        .text
        .align  16, 0x90
.globl PopCnt
PopCnt:
        movl    4(%esp), %ecx
        xorl    %eax, %eax
        testl   %ecx, %ecx
        jz      l1
l0:
        leal    -1(%ecx), %edx
        incl    %eax
        andl    %edx, %ecx
        jnz     l0
l1:
        movl    8(%esp), %ecx
        testl   %ecx, %ecx
        jz      l3
l2:
        leal    -1(%ecx), %edx
        incl    %eax
        andl    %edx, %ecx
        jnz     l2
l3:
        ret

/*----------------------------------------------------------------------------*/

        .align  16, 0x90
        .globl  FirstOne
FirstOne:
        cmpl    $1, 8(%esp)
        sbbl    %eax, %eax
        movl    8(%esp,%eax,4), %edx
        bsr     %edx, %ecx
        jz      l4
        andl    $32, %eax
        subl    $31, %ecx
        subl    %ecx, %eax
        ret
l4:     movl    $64, %eax
        ret

        .align  16, 0x90
        .globl  LastOne
LastOne:
        bsf     4(%esp),%edx
        jz      l5
        movl    $63, %eax
        subl    %edx, %eax
        ret
l5:     bsf     8(%esp), %edx
        jz      l6
        movl    $31, %eax
        subl    %edx, %eax
        ret
l6:     mov     $64, %eax
        ret

/*----------------------------------------------------------------------------*/

        .align  16, 0x90

/*----------------------------------------------------------------------------*/

        .comm   first_ones_8bit, 256
        .comm   last_ones_8bit, 256

/*----------------------------------------------------------------------------*/

        .align  16, 0x90
        .end
