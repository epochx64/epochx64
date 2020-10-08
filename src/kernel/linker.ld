ENTRY(KernelMain)

SECTIONS
{
	. = 1M;

	.text ALIGN (4K) : AT (ADDR (.text))
	{
		*(.text)
	}

	.rodata ALIGN (4K) : AT (ADDR (.rodata))
	{
		*(.rodata)
	}

	.bss ALIGN (4K) : AT (ADDR (.bss))
	{
		*(COMMON)
		*(.bss)
	}

	.data ALIGN (4K) : AT (ADDR (.data))
    {
    	*(.data)
    }
}