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

int main()
{
	void *blob = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	cheapstrcpy((char *)blob, "Hello world\n");

	pid_t pid = fork();
	if(pid == 0)
	{
		puts("Child process!\n");

		cheapstrcpy((char *)blob, "Hello child process\n");
		puts((const char *)blob);
	}
	else
	{
		sleep(); // Todo: How about waitpid()?

		puts("Parent process!\n");
		puts((const char *)blob);
	}

	return 0;
}
