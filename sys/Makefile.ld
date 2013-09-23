ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
	. = 0x100000;
	kernelBegin = .;

	.text : 
	{
		*(multiboot)
		*(.text)
	}
	.data ALIGN(4096) : 
	{
		*(.data)
	}
	.rodata ALIGN(4096) : 
	{
		*(.rodata)
	}
	.bss ALIGN(4096) : 
	{
		*(.bss)
	}

	. = ALIGN(4096);
	ctorsBegin = .;

	.ctors :
	{
		*(.ctors)
	}

	ctorsEnd = .;

	. = ALIGN(4096);
	kernelEnd = .;
}

