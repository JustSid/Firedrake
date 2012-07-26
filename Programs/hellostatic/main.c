//
//  main.c
//  helloworld
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

#include <libtest/print.h>
#include <libtest/thread.h>
#include <libtest/mmap.h>

void cheapstrcpy(char *dst, const char *src)
{
	while(*src != '\0')
	{
		*dst = *src;

		dst ++;
		src ++;
	}
}

void threadEntry(void *arg)
{
	puts("Thread 1\n");
}

int main()
{
	puts("Main thread: Entry\n");

	uint32_t tid = thread_create((void *)threadEntry, NULL);
	thread_join(tid);

	void *region = mmap(NULL, 4096 * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	uintptr_t address = (uintptr_t)region;

	munmap((void *)(address + 4096), 4096);
	munmap(region, 4096);

	// Segfault, here we go...
	cheapstrcpy((char *)region, "Hello world!\n");
	puts((const char *)region);

	puts("Main thread: Exit\n");

	return 0;
}
