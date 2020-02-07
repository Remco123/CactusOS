[bits 16]

global VM86CodeStart
VM86CodeStart:

; FIXME: uses self modifying code
global Int86
Int86:
    push cs
    pop ds

    ; modify the int n instruction
    mov byte [Int86.doInt - VM86CodeStart + 1], al

    ; load registers
    mov ax, word [0x8000]
    mov bx, word [0x8002]
    mov cx, word [0x8004]
    mov dx, word [0x8006]

    mov di, word [0x8008]
    mov es, word [0x8010]
    ; do the int n instruction
.doInt:
    int 0x0	; dummy int instruction
    		; will be overwritten
    ; store registers
    push ds
    push cs
    pop ds
    
    mov word [0x8000], ax
    mov word [0x8002], bx
    mov word [0x8004], cx
    mov word [0x8006], dx

    mov word [0x8008], di
    mov word [0x8010], es

    ; return to kernel
    int 0xFE

global VM86CodeEnd
VM86CodeEnd:
