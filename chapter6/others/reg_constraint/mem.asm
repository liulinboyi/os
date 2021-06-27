	.file	"mem.c"
	.section	.rodata
.LC0:
	.string	"in_b is %d\n"
.LC1:
	.string	"in_b now is %d\n"
	.text
.globl main
	.type	main, @function
main:
	pushl	%ebp ; 堆栈框架
	movl	%esp, %ebp
	andl	$-16, %esp
	subl	$32, %esp ; 预留局部变量空间
	movl	$1, 28(%esp) ; 变量in_a在栈中位置
	movl	$2, 24(%esp) ; 变量in_b在栈中位置
	movl	24(%esp), %edx
	movl	$.LC0, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	28(%esp), %eax ; 内联汇编的input,将in_a赋值给eax

#APP
# 10 "./mem.c" 1
	movb %al, 24(%esp); ;内联汇编,直接将al写入in_b的内存
# 0 "" 2
#NO_APP
	movl	24(%esp), %edx
	movl	$.LC1, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.4.7-8ubuntu1) 4.4.7"
	.section	.note.GNU-stack,"",@progbits
