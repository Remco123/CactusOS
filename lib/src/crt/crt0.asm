BITS 32

extern libMain

global _start
_start:
	call libMain 	; Defined in lib/src/main.cpp
_wait: 					; enter infinite loop in the case we don't exit for some reason
	hlt
	jmp    _wait