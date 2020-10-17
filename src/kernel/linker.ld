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

    /*
     * The bootstrap code for the application processors (APs)
     * needs to be in the lower 1MiB of memory and also has
     * to be 4KiB aligned
     */
    . = 0x1000;
    .apbootstrapdata ALIGN(4K) : AT (ADDR(.apbootstrapdata))
    {
        *(.apbootstrapdata)
    }

    . = 0x10000;

    .apbootstrap ALIGN (4K) : AT (ADDR(.apbootstrap))
    {
        *(.apbootstrap)
    }
}