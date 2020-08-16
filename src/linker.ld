/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */

ENTRY(_start)

SECTIONS
{
	. = 1M;

	/*
	    First put the multiboot header, as it is required to be put very early
	    early in the image or the bootloader won't recognize the file format.
	    Next we'll put the .text section.
	*/

	.text 0x100000 :
	{
        *(.multiboot)
        *(.bootstrap)
		*(.text)
	}

	.rodata ALIGN (4K) : AT (ADDR (.rodata))
	{
		*(.rodata)
	}

	.data ALIGN (4K) : AT (ADDR (.data))
	{
		*(.data)
	}

	.bss ALIGN (4K) : AT (ADDR (.bss))
	{
		*(COMMON)
		*(.bss)
	}
}