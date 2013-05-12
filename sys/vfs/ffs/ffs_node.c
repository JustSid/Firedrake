//
//  ffs_node.h
//  Firedrake
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

#include <memory/memory.h>
#include <libc/string.h>
#include <libc/math.h>
#include <system/syslog.h>
#include "ffs_node.h"

#define kFFSNodeMaxChunkPages 10
#define kFFSNodeMaxChunkSize (kFFSNodeMaxChunkPages * VM_PAGE_SIZE)

ffs_node_data_t *ffs_node_dataCreate()
{
	ffs_node_data_t *data = halloc(NULL, sizeof(ffs_node_data_t));
	data->data = NULL;
	data->size = 0;
	data->pages = 0;

	return data;
}

void ffs_node_dataDestroy(ffs_node_data_t *data)
{
	if(data->data)
		mm_free(data->data, vm_getKernelDirectory(), data->pages);

	hfree(NULL, data);
}

void ffs_node_allocateMemory(ffs_node_data_t *data, size_t minSize)
{
	if(!data->data)
	{
		size_t pages = pageCount(minSize);
		data->data  = mm_alloc(vm_getKernelDirectory(), pages, VM_FLAGS_KERNEL);
		data->pages = pages;

		return;
	}

	size_t requiredPages = pageCount(minSize);
	if(requiredPages > data->pages)
	{
		void *temp = mm_alloc(vm_getKernelDirectory(), requiredPages, VM_FLAGS_KERNEL);
		memcpy(temp, data->data, data->size);
		mm_free(data->data, vm_getKernelDirectory(), data->pages);

		data->data  = temp;
		data->pages = requiredPages;
	}
}

size_t ffs_node_writeData(ffs_node_data_t *data, vfs_context_t *context, size_t offset, const void *ptr, size_t size, int *errno)
{
	size = MIN(size, kFFSNodeMaxChunkSize);
	ffs_node_allocateMemory(data, data->size + size);

	bool result = vfs_contextCopyDataOut(context, ptr, size, data->data + offset, errno);
	if(!result)
		return -1;

	if(offset + size > data->size)
	{
		data->size += (offset + size) - data->size;
	}

	return size;
}

size_t ffs_node_readData(ffs_node_data_t *data, vfs_context_t *context, size_t offset, void *ptr, size_t size, int *errno)
{
	uint8_t *end = data->data + data->size;
	uint8_t *preferredEnd = data->data + offset + size;

	end = MIN(end, preferredEnd);

	size = end - (data->data + offset);
	size = MIN(size, kFFSNodeMaxChunkSize);

	if(size > 0)
	{
		bool result = vfs_contextCopyDataIn(context, data->data + offset, size, ptr, errno);
		if(result)
		{
			data->size += size;
			return size;
		}

		return -1;
	}

	return 0;
}
