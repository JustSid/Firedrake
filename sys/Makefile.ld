ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
	. = 0x100000;
	__kernel_start__ = .;

	.text : 
	{
		*(.multiboot)
		*(.text.boot)
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
		__bss_start__ = .;
		*(.bss)
		. = ALIGN(4096);
		__bss_end__ = .;
	}
	.ctors :
	{
		__ctors_start__ = .;
		*(.ctors)
		__ctors_end__ = .;
	}
	.init_array :
	{
		__init_array_start = .;
		*(.init_array)
		__init_array_end = .;
	}
	. = ALIGN(4096);
	__kernel_end__ = .;
}

