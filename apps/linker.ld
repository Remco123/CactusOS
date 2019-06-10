ENTRY(_start)
SECTIONS
{
	. = 8M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		start_ctors = .;
        *(SORT(.ctors*))  /* Note the "SORT" */
        end_ctors = .;

        start_dtors = .;
        *(SORT(.dtors*))
        end_dtors = .;

		*(.rodata)
	}
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
} 