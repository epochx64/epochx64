ENTRY(_epochstart)

SECTIONS
{
	.text ALIGN (4K) : AT (ADDR (.text))
	{
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