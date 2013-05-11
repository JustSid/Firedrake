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
#include <libtest/errno.h>

#include <libtest/fcntl.h>
#include <libtest/unistd.h>

void dumpdir(const char *path)
{
	int fd = open(path, O_RDONLY);
	if(fd != -1)
	{
		printf("Content of %s\n", path);

		vfs_directory_entry_t buffer[5];
		off_t read;

		while((read = readdir(fd, buffer, 5)) > 0)
		{
			if(read == -1)
			{
				printf("Encountered error: %i", errno);
				break;
			}

			for(off_t i=0; i<read; i++)
			{
				printf("Entry: %s\n", buffer[i].name);
			}
		}

		printf("\n");
		close(fd);
	}
}

void dumpstat(const char *path)
{
	vfs_stat_t buf;
	int result = stat(path, &buf);
	if(result == 0)
	{
		printf("Stats for: %s\nName: %s, id: %i, size: %u\n", path, buf.name, buf.id, buf.size);
	}
}

void dumpfile(const char *path)
{
	int fd = open(path, O_RDONLY);
	if(fd >= 0)
	{
		char buffer[1024];
		read(fd, buffer, 1024);

		printf("Contents of '%s': \"%s\"\n\n", path, buffer);
		close(fd);
	}
	else
	{
		printf("No such file or directory %s\n", path);
	}
}

int main()
{
	dumpfile("/etc/about");
	dumpdir("/lib");
	dumpstat("/lib/libio.so");

	return 0;
}
