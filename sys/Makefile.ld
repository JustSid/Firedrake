ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
	. = 0x100000;
	kernel_begin = .;

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
	ctors_begin = .;

	.ctors :
	{
		*(.ctors)
	}

	ctors_end = .;

	. = ALIGN(4096);
	kernel_end = .;
}

