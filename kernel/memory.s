	.file	"memory.c"
	.comm	kernel_vaddr,12,4
	.comm	kernel_pool,16,4
	.comm	user_pool,16,4
	.text
	.type	vaddr_pool_apply, @function
vaddr_pool_apply:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$24, %esp
	movl	$0, -20(%ebp)
	movl	$-1, -12(%ebp)
	movl	$0, -16(%ebp)
	cmpl	$1, 8(%ebp)
	jne	.L2
	subl	$8, %esp
	pushl	12(%ebp)
	pushl	$kernel_vaddr
	call	bitmap_scan
	addl	$16, %esp
	movl	%eax, -12(%ebp)
	cmpl	$-1, -12(%ebp)
	jne	.L5
	movl	$0, %eax
	jmp	.L4
.L6:
	movl	-12(%ebp), %edx
	movl	-16(%ebp), %eax
	addl	%edx, %eax
	subl	$4, %esp
	pushl	$1
	pushl	%eax
	pushl	$kernel_vaddr
	call	bitmap_set
	addl	$16, %esp
	addl	$1, -16(%ebp)
.L5:
	movl	-16(%ebp), %eax
	cmpl	12(%ebp), %eax
	jb	.L6
	movl	kernel_vaddr+8, %eax
	movl	-12(%ebp), %edx
	sall	$12, %edx
	addl	%edx, %eax
	movl	%eax, -20(%ebp)
.L2:
	movl	-20(%ebp), %eax
.L4:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	vaddr_pool_apply, .-vaddr_pool_apply
	.globl	pte_ptr
	.type	pte_ptr, @function
pte_ptr:
.LFB1:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	8(%ebp), %eax
	andl	$-4194304, %eax
	shrl	$10, %eax
	movl	%eax, %edx
	movl	8(%ebp), %eax
	andl	$4190208, %eax
	shrl	$12, %eax
	sall	$2, %eax
	addl	%edx, %eax
	subl	$4194304, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	pte_ptr, .-pte_ptr
	.globl	pde_ptr
	.type	pde_ptr, @function
pde_ptr:
.LFB2:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	8(%ebp), %eax
	shrl	$22, %eax
	addl	$1073740800, %eax
	sall	$2, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE2:
	.size	pde_ptr, .-pde_ptr
	.type	phy_page_alloc, @function
phy_page_alloc:
.LFB3:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$24, %esp
	movl	8(%ebp), %eax
	subl	$8, %esp
	pushl	$1
	pushl	%eax
	call	bitmap_scan
	addl	$16, %esp
	movl	%eax, -16(%ebp)
	cmpl	$-1, -16(%ebp)
	jne	.L12
	movl	$0, %eax
	jmp	.L13
.L12:
	movl	-16(%ebp), %edx
	movl	8(%ebp), %eax
	subl	$4, %esp
	pushl	$1
	pushl	%edx
	pushl	%eax
	call	bitmap_set
	addl	$16, %esp
	movl	-16(%ebp), %eax
	sall	$12, %eax
	movl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%edx, %eax
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
.L13:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE3:
	.size	phy_page_alloc, .-phy_page_alloc
	.section	.rodata
.LC0:
	.string	"!(*pte & 0x00000001)"
.LC1:
	.string	"memory.c"
	.text
	.type	page_table_add, @function
page_table_add:
.LFB4:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$40, %esp
	movl	8(%ebp), %eax
	movl	%eax, -28(%ebp)
	movl	12(%ebp), %eax
	movl	%eax, -24(%ebp)
	pushl	-28(%ebp)
	call	pde_ptr
	addl	$4, %esp
	movl	%eax, -20(%ebp)
	pushl	-28(%ebp)
	call	pte_ptr
	addl	$4, %esp
	movl	%eax, -16(%ebp)
	movl	-20(%ebp), %eax
	movl	(%eax), %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L15
	movl	-16(%ebp), %eax
	movl	(%eax), %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L16
	pushl	$.LC0
	pushl	$__func__.1760
	pushl	$77
	pushl	$.LC1
	call	panic_spin
	addl	$16, %esp
.L16:
	movl	-16(%ebp), %eax
	movl	(%eax), %eax
	andl	$1, %eax
	testl	%eax, %eax
	jne	.L19
	movl	-24(%ebp), %eax
	orl	$7, %eax
	movl	%eax, %edx
	movl	-16(%ebp), %eax
	movl	%edx, (%eax)
	jmp	.L19
.L15:
	subl	$12, %esp
	pushl	$kernel_pool
	call	phy_page_alloc
	addl	$16, %esp
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	orl	$7, %eax
	movl	%eax, %edx
	movl	-20(%ebp), %eax
	movl	%edx, (%eax)
	movl	-16(%ebp), %eax
	andl	$-4096, %eax
	subl	$4, %esp
	pushl	$4096
	pushl	$0
	pushl	%eax
	call	memset
	addl	$16, %esp
	movl	-16(%ebp), %eax
	movl	(%eax), %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L18
	pushl	$.LC0
	pushl	$__func__.1760
	pushl	$90
	pushl	$.LC1
	call	panic_spin
	addl	$16, %esp
.L18:
	movl	-24(%ebp), %eax
	orl	$7, %eax
	movl	%eax, %edx
	movl	-16(%ebp), %eax
	movl	%edx, (%eax)
.L19:
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE4:
	.size	page_table_add, .-page_table_add
	.section	.rodata
.LC2:
	.string	"pg_cnt > 0 && pg_cnt < 3840"
	.text
	.globl	malloc_pages
	.type	malloc_pages, @function
malloc_pages:
.LFB5:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$40, %esp
	cmpl	$0, 12(%ebp)
	je	.L21
	cmpl	$3839, 12(%ebp)
	jbe	.L22
.L21:
	pushl	$.LC2
	pushl	$__func__.1766
	pushl	$98
	pushl	$.LC1
	call	panic_spin
	addl	$16, %esp
.L22:
	subl	$8, %esp
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	vaddr_pool_apply
	addl	$16, %esp
	movl	%eax, -20(%ebp)
	cmpl	$0, -20(%ebp)
	jne	.L23
	movl	$0, %eax
	jmp	.L24
.L23:
	movl	-20(%ebp), %eax
	movl	%eax, -28(%ebp)
	movl	12(%ebp), %eax
	movl	%eax, -24(%ebp)
	movl	8(%ebp), %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L25
	movl	$kernel_pool, %eax
	jmp	.L26
.L25:
	movl	$user_pool, %eax
.L26:
	movl	%eax, -16(%ebp)
	jmp	.L27
.L28:
	subl	$12, %esp
	pushl	-16(%ebp)
	call	phy_page_alloc
	addl	$16, %esp
	movl	%eax, -12(%ebp)
	movl	-28(%ebp), %eax
	subl	$8, %esp
	pushl	-12(%ebp)
	pushl	%eax
	call	page_table_add
	addl	$16, %esp
	addl	$4096, -28(%ebp)
.L27:
	movl	-24(%ebp), %eax
	leal	-1(%eax), %edx
	movl	%edx, -24(%ebp)
	testl	%eax, %eax
	jne	.L28
	movl	-20(%ebp), %eax
.L24:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE5:
	.size	malloc_pages, .-malloc_pages
	.globl	get_kernel_pages
	.type	get_kernel_pages, @function
get_kernel_pages:
.LFB6:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$24, %esp
	subl	$8, %esp
	pushl	8(%ebp)
	pushl	$1
	call	malloc_pages
	addl	$16, %esp
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	je	.L30
	subl	$4, %esp
	pushl	$16777216
	pushl	$0
	pushl	-12(%ebp)
	call	memset
	addl	$16, %esp
.L30:
	movl	-12(%ebp), %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE6:
	.size	get_kernel_pages, .-get_kernel_pages
	.section	.rodata
.LC3:
	.string	" mem_pool_init start!\n"
.LC4:
	.string	"   kernel_pool_bitmap_start: "
	.align 4
.LC5:
	.string	"  kernel_pool_phy_addr_start: "
.LC6:
	.string	"\n"
.LC7:
	.string	"   user_pool_bitmap_start: "
.LC8:
	.string	"  user_pool_phy_addr_start: "
.LC9:
	.string	"mem_pool_init done\n"
	.text
	.type	mem_pool_init, @function
mem_pool_init:
.LFB7:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$56, %esp
	subl	$12, %esp
	pushl	$.LC3
	call	put_str
	addl	$16, %esp
	movl	$1048576, -36(%ebp)
	movl	-36(%ebp), %eax
	addl	$1048576, %eax
	movl	%eax, -32(%ebp)
	movl	8(%ebp), %eax
	subl	-32(%ebp), %eax
	movl	%eax, -28(%ebp)
	movl	-28(%ebp), %eax
	shrl	$12, %eax
	movw	%ax, -42(%ebp)
	movzwl	-42(%ebp), %eax
	shrw	%ax
	movw	%ax, -40(%ebp)
	movzwl	-42(%ebp), %eax
	subw	-40(%ebp), %ax
	movw	%ax, -38(%ebp)
	movzwl	-40(%ebp), %eax
	shrw	$3, %ax
	movzwl	%ax, %eax
	movl	%eax, -24(%ebp)
	movzwl	-38(%ebp), %eax
	shrw	$3, %ax
	movzwl	%ax, %eax
	movl	%eax, -20(%ebp)
	movl	-32(%ebp), %eax
	movl	%eax, -16(%ebp)
	movzwl	-40(%ebp), %eax
	sall	$12, %eax
	movl	%eax, %edx
	movl	-16(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, -12(%ebp)
	movl	-16(%ebp), %eax
	movl	%eax, kernel_pool+8
	movl	-12(%ebp), %eax
	movl	%eax, user_pool+8
	movzwl	-40(%ebp), %eax
	sall	$12, %eax
	movl	%eax, kernel_pool+12
	movzwl	-38(%ebp), %eax
	sall	$12, %eax
	movl	%eax, user_pool+12
	movl	-24(%ebp), %eax
	movl	%eax, kernel_pool
	movl	-20(%ebp), %eax
	movl	%eax, user_pool
	movl	$-1073111040, kernel_pool+4
	movl	-24(%ebp), %eax
	subl	$1073111040, %eax
	movl	%eax, user_pool+4
	subl	$12, %esp
	pushl	$.LC4
	call	put_str
	addl	$16, %esp
	movl	kernel_pool+4, %eax
	subl	$12, %esp
	pushl	%eax
	call	put_int
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC5
	call	put_str
	addl	$16, %esp
	movl	kernel_pool+8, %eax
	subl	$12, %esp
	pushl	%eax
	call	put_int
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC6
	call	put_str
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC7
	call	put_str
	addl	$16, %esp
	movl	user_pool+4, %eax
	subl	$12, %esp
	pushl	%eax
	call	put_int
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC8
	call	put_str
	addl	$16, %esp
	movl	user_pool+8, %eax
	subl	$12, %esp
	pushl	%eax
	call	put_int
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC6
	call	put_str
	addl	$16, %esp
	subl	$12, %esp
	pushl	$kernel_pool
	call	bitmap_init
	addl	$16, %esp
	subl	$12, %esp
	pushl	$user_pool
	call	bitmap_init
	addl	$16, %esp
	movl	-24(%ebp), %eax
	movl	%eax, kernel_vaddr
	movl	-24(%ebp), %edx
	movl	-20(%ebp), %eax
	addl	%edx, %eax
	subl	$1073111040, %eax
	movl	%eax, kernel_vaddr+4
	movl	$-1072693248, kernel_vaddr+8
	subl	$12, %esp
	pushl	$kernel_vaddr
	call	bitmap_init
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC9
	call	put_str
	addl	$16, %esp
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE7:
	.size	mem_pool_init, .-mem_pool_init
	.section	.rodata
.LC10:
	.string	"mem_init start\n"
.LC11:
	.string	"mem_init done!\n"
	.text
	.globl	mem_init
	.type	mem_init, @function
mem_init:
.LFB8:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$24, %esp
	subl	$12, %esp
	pushl	$.LC10
	call	put_str
	addl	$16, %esp
	movl	$2816, %eax
	movl	(%eax), %eax
	movl	%eax, -12(%ebp)
	subl	$12, %esp
	pushl	-12(%ebp)
	call	mem_pool_init
	addl	$16, %esp
	subl	$12, %esp
	pushl	$.LC11
	call	put_str
	addl	$16, %esp
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE8:
	.size	mem_init, .-mem_init
	.section	.rodata
	.align 4
	.type	__func__.1760, @object
	.size	__func__.1760, 15
__func__.1760:
	.string	"page_table_add"
	.align 4
	.type	__func__.1766, @object
	.size	__func__.1766, 13
__func__.1766:
	.string	"malloc_pages"
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.9) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
