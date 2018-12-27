global cpuGetEIP
cpuGetEIP:
  pop eax
  jmp eax

global cpuGetESP
cpuGetESP:
  mov eax, esp
  add eax, 4
  ret

# VM86 Code
global cpuEnterV86Int
cpuEnterV86Int:
  push ebp
  mov ebp, esp
  push ebx
  push esi
  push edi
  mov eax, [ebp + 8]
  mov ebx, [ebp + 12]
  mov ecx, [ebp + 16]
  mov edx, [ebp + 20]
  mov esi, [ebp + 24]
  mov edi, [ebp + 28]
  int 0xFD ; MUST match interrupt number registered in vm86.c
  pop edi
  pop esi
  pop ebx
  mov esp, ebp
  pop ebp
  ret

global cpuEnterV86
cpuEnterV86:
  mov ebp, esp
  push 0                ; GS
  push 0                ; FS
  push 0                ; DS
  push 0                ; ES
  push dword [ebp + 4]  ; SS
  push dword [ebp + 8]  ; ESP
  pushfd                ; EFLAGS
  or dword [esp], (1 << 17)
  push dword [ebp + 12] ; CS
  push dword [ebp + 16] ; EIP
  mov eax, dword [ebp + 20]
;  hlt
  iretd