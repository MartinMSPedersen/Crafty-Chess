	.file	"tst.c"
.globl ans
	.data
	.align 4
	.type	ans,@object
	.size	ans,4
ans:
	.long	-1
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"val? "
.LC1:
	.string	"%d"
.LC2:
	.string	"ans=%d\n"
	.text
.globl main
	.type	main,@function
main:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$12, %esp
	andl	$-16, %esp
	subl	$12, %esp
	pushl	$.LC0
	call	printf
	addl	$8, %esp
	leal	-24(%ebp), %eax
	pushl	%eax
	pushl	$.LC1
	call	scanf
	addl	$8, %esp
ZZ: Command not found.
	movl	%ebx, %edx
#APP
	        xorl    %ebx, %ebx
	        testl   %edx, %edx
	        jz      2f
	1:      leal    -1(%edx), %eax
	        incl    %ebx
	        andl    %eax, %edx
	        jnz     1b
	2:      testl   %ecx, %ecx
	        jz      4f
	3:      leal    -1(%ecx), %eax
	        incl    %ebx
	        andl    %eax, %ecx
	        jnz     3b
	4:      movl    %ebx,ans
	
#NO_APP
	pushl	%ebx
	pushl	$.LC2
	call	printf
	addl	$8, %esp
	pushl	ans
	pushl	$.LC2
	call	printf
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	leave
	ret
.Lfe1:
	.size	main,.Lfe1-main
	.ident	"GCC: (GNU) 3.2.2 20030222 (Red Hat Linux 3.2.2-5)"
