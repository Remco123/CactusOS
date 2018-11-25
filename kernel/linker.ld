ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
  . = 0xC0100000;

  _kernel_start = .;

  .text ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
  {
    *(.multiboot)
    *(.text*)
    *(.rodata)
  }

  .data ALIGN (0x1000) : AT(ADDR(.data)-0xC0000000)
  {
    start_ctors = .;
    KEEP(*( .init_array ));
    KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
    end_ctors = .;

    *(.data)
  }

  .bss ALIGN (0x1000) : AT(ADDR(.bss)-0xC0000000)
  {
    *(.bss)
  }

  /DISCARD/ : { *(.fini_array*) *(.comment) }

  _kernel_end = .;
}