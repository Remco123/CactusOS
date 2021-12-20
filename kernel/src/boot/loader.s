.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

.section .multiboot
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

.set KERNEL_VIRTUAL_BASE, 0xC0000000
.set KERNEL_PAGE_NUMBER, (KERNEL_VIRTUAL_BASE >> 22)

.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
.global stack_top
stack_top:

.section .data
.align 0x1000
.global BootPageDirectory
BootPageDirectory:
	# identity map the first 4 MB
	.long 0x00000083
	
	# pages before kernel
	.rept (KERNEL_PAGE_NUMBER - 1)
	.long 0
	.endr
	
	# this page contains the kernel
	.long 0x00000083

	# pages after kernel
	.rept (1024 - KERNEL_PAGE_NUMBER - 1)
	.long 0
	.endr

.global _kernel_virtual_base
_kernel_virtual_base:
	.long KERNEL_VIRTUAL_BASE

.section .text
.align 4
.global _entrypoint
.type _entrypoint, @function

_entrypoint:
	mov $(BootPageDirectory - KERNEL_VIRTUAL_BASE), %ecx
	mov %ecx, %cr3

	# enable 4 mb pages
	mov %cr4, %ecx
	or $0x00000010, %ecx
	mov %ecx, %cr4

	# enable paging
	mov %cr0, %ecx
	or $0x80000001, %ecx
	mov %ecx, %cr0

	# jump to higher half code
	lea 4f, %ecx
	jmp *%ecx

4:
	# Unmap the identity-mapped pages
	movl $0, BootPageDirectory
	invlpg 0

	movl $stack_top, %esp
	# Mark end of call stack for unwinding
	movl $0, %ebp

	add $KERNEL_VIRTUAL_BASE, %ebx

    call callConstructors

	push $_kernel_base
	push $_kernel_end
    push %eax
    push %ebx
    call kernelMain

_stop:
    cli
    hlt
    jmp _stop
