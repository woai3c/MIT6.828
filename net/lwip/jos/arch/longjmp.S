#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 16, 0x90
#endif

#define ENTRY(x) \
        .text; _ALIGN_TEXT; .globl x; .type x,@function; x:


ENTRY(jos_setjmp)
	movl	4(%esp), %ecx	// jos_jmp_buf

	movl	0(%esp), %edx	// %eip as pushed by call
	movl	%edx,  0(%ecx)

	leal	4(%esp), %edx	// where %esp will point when we return
	movl	%edx,  4(%ecx)

	movl	%ebp,  8(%ecx)
	movl	%ebx, 12(%ecx)
	movl	%esi, 16(%ecx)
	movl	%edi, 20(%ecx)

	movl	$0, %eax
	ret

ENTRY(jos_longjmp)
	// %eax is the jos_jmp_buf*
	// %edx is the return value

	movl	 0(%eax), %ecx	// %eip
	movl	 4(%eax), %esp
	movl	 8(%eax), %ebp
	movl	12(%eax), %ebx
	movl	16(%eax), %esi
	movl	20(%eax), %edi

	movl	%edx, %eax
	jmp	*%ecx

