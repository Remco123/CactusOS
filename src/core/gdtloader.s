.global load_gdt
load_gdt:
    mov 4(%esp), %edx   # EDX is 1st argument - GDT record pointer
    mov 8(%esp), %eax   # EAX is 2nd argument - Data Selector
    lgdt (%edx)         # Load GDT Register with GDT record at pointer passed as 1st argument
    mov %eax, %ds       # Reload all the data descriptors with Data selector (2nd argument)
    mov %eax, %es
    mov %eax, %gs
    mov %eax, %fs
    mov %eax, %ss

    pushl 12(%esp)      # Create FAR pointer on stack using Code selector (3rd argument)
    pushl $.setcs       # Offset of FAR JMP will be setcs label below
    ljmp *(%esp)        # Do the FAR JMP to next instruction to set CS with Code selector, and
                        #    set the EIP (instruction pointer) to offset of setcs
.setcs:
    add $8, %esp        # Restore stack (remove 2 DWORD values we put on stack to create FAR Pointer)
ret