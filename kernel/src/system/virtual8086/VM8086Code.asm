[bits 16]

global __VM86CodeStart
__VM86CodeStart:

; FIXME: uses self modifying code
global __Int86
__Int86:
    push cs
    pop ds

    ; modify the int n instruction
    mov byte [__Int86.doInt - __VM86CodeStart + 1], al

    ; load registers
    ; layout if these registers MUST match
    ; VM86Regs structure in vm86.h
    push dword [0x801C] ; ds
    mov eax, dword [0x8020]
    mov es, ax
    mov eax, dword [0x8024]
    mov fs, ax
    mov eax, dword [0x8028]
    mov gs, ax
    mov eax, dword [0x8000]
    mov ebx, dword [0x8004]
    mov ecx, dword [0x8008]
    mov edx, dword [0x800C]
    mov esi, dword [0x8010]
    mov edi, dword [0x8014]
    mov ebp, dword [0x8018]
    pop ds
    ; do the int n instruction
.doInt:
    int 0xFE	; dummy int instruction
    		; will be overwritten
    ; store registers
    push ds
    push cs
    pop ds
    mov ebp, dword [0x8018]
    mov edi, dword [0x8014]
    mov esi, dword [0x8010]
    mov edx, dword [0x800C]
    mov ecx, dword [0x8008]
    mov ebx, dword [0x8004]
    mov eax, dword [0x8000]
    xor eax, eax
    mov ax, gs
    mov dword [0x8028], eax
    mov ax, fs
    mov dword [0x8024], eax
    mov ax, es
    mov dword [0x8020], eax
    pop dword [0x801C] ; ds

    ; return to kernel
    int 0xFE

global __VM86CodeEnd
__VM86CodeEnd:
