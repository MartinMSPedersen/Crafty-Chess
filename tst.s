	.file	"tst.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"v=%llx\n"
.LC1:
	.string	"first=%d\n"
.LC2:
	.string	"last=%d\n"
	.text
.globl main
	.type	main,@function
main:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%ebx
	andl	$-16, %esp
	subl	$4, %esp
	pushl	$-16711936
	pushl	$-16776961
	pushl	$.LC0
	call	printf
	addl	$8, %esp
	movl	$-16711936, %ebx
	movl	%ebx, %ecx
	movl	$-16776961, %edx
#APP
	        bsr     %edx, %eax
	        jnz     2f
	        bsr     %ecx, %eax
	        jnz     1f
	        movl    $64, %eax
	        jmp     2f
	1:      addl    $32,%eax
	2:                    
	
#NO_APP
	pushl	%eax
	pushl	$.LC1
	call	printf
	addl	$8, %esp
	movl	%ebx, %ecx
	movl	$-16776961, %edx
#APP
	        bsf     %ecx, %eax
	        jz      1f
	        addl    $32,%eax
	        jmp     2f
	1:      bsf     %edx, %eax
	        jnz     2f
	        movl    $64, %eax
	2:
#NO_APP
	pushl	%eax
	pushl	$.LC2
	call	printf
	leal	-8(%ebp), %esp
	popl	%ebx
	popl	%esi
	leave
	ret
.Lfe1:
	.size	main,.Lfe1-main
	.ident	"GCC: (GNU) 3.2.2 20030222 (Red Hat Linux 3.2.2-5)"
