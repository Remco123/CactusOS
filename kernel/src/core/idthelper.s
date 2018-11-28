.macro HandleException num
.global HandleException\num\()
HandleException\num\():
    movb $\num, (interruptnumber)
    jmp interrupthandler
.endm


.macro HandleInterruptRequest num
.global HandleInterruptRequest\num\()
HandleInterruptRequest\num\():
    movb $\num + 0x20, (interruptnumber)
    pushl $0
    jmp interrupthandler
.endm

HandleException 0x00
HandleException 0x01
HandleException 0x02
HandleException 0x03
HandleException 0x04
HandleException 0x05
HandleException 0x06
HandleException 0x07
HandleException 0x08
HandleException 0x09
HandleException 0x0A
HandleException 0x0B
HandleException 0x0C
HandleException 0x0D
HandleException 0x0E
HandleException 0x0F
HandleException 0x10
HandleException 0x11
HandleException 0x12
HandleException 0x13

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

interrupthandler:    
    pushl %ebp
    pushl %edi
    pushl %esi

    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax

    # call C++ Handler
    pushl %esp
    push (interruptnumber)
    call _ZN8CactusOS4core16InterruptManager15HandleInterruptEhj

    mov %eax, %esp # switch the stack that is returned from the function

    # restore registers
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx

    popl %esi
    popl %edi
    popl %ebp
    
    add $4, %esp

.global IgnoreInterrupt
IgnoreInterrupt:
    iret

.data
interruptnumber: .byte 0