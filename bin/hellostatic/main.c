//
//  main.c
//  helloworld
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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
#include <libtest/errno.h>
#include <libtest/tls.h>

void threadEntry(void *arg)
{
	int slot = tls_allocate();
	tls_set(slot, arg);

	sleep(1000);

	printf("Got: %p\n", tls_get(slot));

	tls_free(slot);
	tls_free(slot); // Make sure that errno is != 0!
}

int main()
{
	uint32_t t1 = thread_create(threadEntry, 0x1024);
	uint32_t t2 = thread_create(threadEntry, 0x32);

	thread_join(t1);
	thread_join(t2);

	printf("errno: %i\n", errno);
	return 0;
}
