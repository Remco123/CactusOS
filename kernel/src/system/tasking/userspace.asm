GLOBAL enter_usermode
enter_usermode:
   ;push ebp
   mov ebp, esp
   cli

   mov ax, 0x20 | 3
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax

   push 0x20 | 3   ; push ss3

   mov ecx, [ebp+8]
   push ecx ; push esp3

   pushf  ; push flags onto stack
   pop eax ; pop into eax
   or eax, [ebp+12] ; Copy EFLAGS from arg 3
   push eax ; push eflags
   push 0x18 | 3 ; push CS, requested priv. level = 3

   xor eax, eax  ; Clear eax
   mov eax, [ebp+4] ; Load new IP into eax
   push eax ; Push EIP onto stack

   iret