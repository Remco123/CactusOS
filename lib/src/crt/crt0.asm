BITS 32

global _start
_start:
	extern libMain
	call   libMain 	; Defined in lib/src/main.cpp
	mov    ebx, eax
	mov    eax, 0x0
	int    0x80 		; Syscall 0 is exit, returncode is in %ebx
_wait: 					; enter infinite loop in the case we don't exit for some reason
	hlt
	jmp    _wait