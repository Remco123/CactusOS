.macro HandleExceptionWithError num
.global HandleException\num\()
HandleException\num\():
    pushl $\num
    jmp interrupthandler
.endm

.macro HandleExceptionNoError num
.global HandleException\num\()
HandleException\num\():
    pushl $0 # Add empty error code
    pushl $\num
    jmp interrupthandler
.endm


.macro HandleInterruptRequest num
.global HandleInterruptRequest\num\()
HandleInterruptRequest\num\():
    pushl $0
    pushl $\num + 0x20
    jmp interrupthandler
.endm

HandleExceptionNoError 0x00
HandleExceptionNoError 0x01
HandleExceptionNoError 0x02
HandleExceptionNoError 0x03
HandleExceptionNoError 0x04
HandleExceptionNoError 0x05
HandleExceptionNoError 0x06
HandleExceptionNoError 0x07
HandleExceptionWithError 0x08
HandleExceptionNoError 0x09
HandleExceptionWithError 0x0A
HandleExceptionWithError 0x0B
HandleExceptionWithError 0x0C
HandleExceptionWithError 0x0D
HandleExceptionWithError 0x0E
HandleExceptionNoError 0x0F
HandleExceptionNoError 0x10
HandleExceptionNoError 0x11
HandleExceptionNoError 0x12
HandleExceptionNoError 0x13

HandleInterruptRequest 0x00
HandleInterruptRequest 0x01
HandleInterruptRequest 0x02
HandleInterruptRequest 0x03
HandleInterruptRequest 0x04
HandleInterruptRequest 0x05
HandleInterruptRequest 0x06
HandleInterruptRequest 0x07
HandleInterruptRequest 0x08
HandleInterruptRequest 0x09
HandleInterruptRequest 0x0A
HandleInterruptRequest 0x0B
HandleInterruptRequest 0x0C
HandleInterruptRequest 0x0D
HandleInterruptRequest 0x0E
HandleInterruptRequest 0x0F
HandleInterruptRequest 0x31

HandleInterruptRequest 0xDD
HandleInterruptRequest 0x60

interrupthandler:    
	cli # Stop Interrupts
	
    # Save Registers
    pusha
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

    # load the kernel data segment descriptor
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl %esp
	# Call the kernel IRQ handler
	call _ZN8CactusOS4core24InterruptDescriptorTable15HandleInterruptEPNS0_8CPUStateE
	mov %eax, %esp

    # Restore Registers
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

    # Clean up
	add $8, %esp

	sti # Restart Interrupts
    iret

.global IgnoreInterrupt
IgnoreInterrupt:
    iret