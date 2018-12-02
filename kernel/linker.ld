ENTRY(_entrypoint)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
  . = 0xC0100000;

	.text BLOCK(4K) : AT(ADDR(.text) - 0xC0000000) ALIGN(4K)
	{
		*(.multiboot)
		*(.text*)
  }

  .rodata BLOCK(4K) : AT(ADDR(.rodata) - 0xC0000000) ALIGN(4K)
	{
		*(.rodata)
  }

  .data BLOCK(4K) : AT(ADDR(.data) - 0xC0000000) ALIGN(4K)
  {
    start_ctors = .;
    KEEP(*( .init_array ));
    KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
    end_ctors = .;

    *(.data*)
  }

  .bss BLOCK(4K) : AT(ADDR(.bss) - 0xC0000000) ALIGN(4K)
  {
    *(COMMON)
    *(.bss*)
    *(.bootstrap_stack)
  }

  /DISCARD/ : { *(.fini_array*) *(.comment) }

  _kernel_end = .;
}