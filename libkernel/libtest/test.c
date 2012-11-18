//
//  test.c
//  libtest
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <libkernel/libkernel.h>

static bool test_shifKey = false;

char test_translateScancode(uint8_t scancode)
{
	bool keyDown = !(scancode & 0x80);
	scancode = keyDown ? scancode : scancode & ~0x80;

	if(keyDown)
	{
		if(!test_shifKey)
			test_shifKey = (scancode == 0x2A || scancode == 0x36);

		if(test_shifKey)
		{
			switch(scancode)
			{
				case 2:
					return '!';
				case 3:
					return '@';
				case 4:
					return '#';
				case 5:
					return '$';
				case 6:
					return '%';
				case 7:
					return '^';
				case 8:
					return '&';
				case 9:
					return '*';
				case 10:
					return '(';
				case 11:
					return ')';
				case 12:
					return '_';
				case 13:
					return '+';

				default:
					break;
			}
		}

		switch(scancode)
		{
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				return (scancode - 2) + '1';
			case 11:
				return '0';
			case 12:
				return '-';
			case 13:
				return '=';

			case 28:
				return '\n';
			case 57:
				return ' ';
			case 42:
			case 54:
				return '\0';

			case 51:
				return ',';
			case 52:
				return '.';

			case 16:
				return 'q';
			case 17:
				return 'w';
			case 18:
				return 'e';
			case 19:
				return 'r';
			case 20:
				return 't';
			case 21:
				return 'y';
			case 22:
				return 'u';
			case 23:
				return 'i';
			case 24:
				return 'o';
			case 25:
				return 'p';

			case 30:
				return 'a';
			case 31:
				return 's';
			case 32:
				return 'd';
			case 33:
				return 'f';
			case 34:
				return 'g';
			case 35:
				return 'h';
			case 36:
				return 'j';
			case 37:
				return 'k';
			case 38:
				return 'l';

			case 44:
				return 'z';
			case 45:
				return 'x';
			case 46:
				return 'c';
			case 47:
				return 'v';
			case 48:
				return 'b';
			case 49:
				return 'n';
			case 50:
				return 'm';

			default:
				printf("(%i)", (int)scancode);
				return '\0';
		}
	}
	else
	{
		if(test_shifKey)
			test_shifKey = !(scancode == 0x2A || scancode == 0x36);
	}

	return '\0';
}

void test_handleInterrupt(uint32_t UNUSED(interrupt), void *owner, void *UNUSED(context))
{
	uint8_t scancode = inb(0x60);
	char character = test_translateScancode(scancode);

	printf("%c", (test_shifKey) ? toupper(character) : character);

	if(scancode == 1) // ESC key
	{
		kern_setInterruptHandler(0x21, owner, NULL, NULL);
		kern_moduleRelease((kern_module_t *)owner);
	}
}



bool test_start(kern_module_t *module)
{	
	kern_return_t result = kern_setInterruptHandler(0x21, module, NULL, test_handleInterrupt);
	if(result != kReturnSuccess)
		return false;

	printf("test_start() %s\n", module->name);

	// Clear the keyboard buffer
	while(inb(0x64) & 0x1)
		inb(0x60);

	while((inb(0x64) & 0x2)) {} // Wait until the keyboard is ready
	outb(0x60, 0xF4); // Activate the keyboard

	return true;
}

bool test_stop(kern_module_t *module)
{
	printf("test_stop() %s\n", module->name);
	return true;
}

kern_start_stub(test_start)
kern_stop_stub(test_stop)
