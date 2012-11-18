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

void test_handleInterrupt(uint32_t UNUSED(interrupt), void *owner, void *UNUSED(context))
{
	uint8_t scancode = inb(0x60);
	printf("scancode: %i\n", (int)scancode);

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
