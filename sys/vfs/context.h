//
//  context.h
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

#ifndef _VFS_CONTEXT_H_
#define _VFS_CONTEXT_H_

#include <prefix.h>
#include <memory/memory.h>
#include "node.h"

typedef struct vfs_context_s
{
	vfs_node_t *chdir;
	vm_page_directory_t pdirectory;
} vfs_context_t;

vfs_context_t *vfs_getKernelContext();
vfs_context_t *vfs_getCurrentContext();

vfs_context_t *vfs_contextCreate(vm_page_directory_t directory);
void vfs_contextDelete(vfs_context_t *context);

void vfs_contextChangeDirectory(vfs_context_t *context, vfs_node_t *node);

// Copys data OUT of the contexts virtual address space into the kernels
bool vfs_contextCopyDataOut(vfs_context_t *context, const void *data, size_t size, void *target, int *errno);
// Copys data IN the contexts virtual address space
bool vfs_contextCopyDataIn(vfs_context_t *context, const void *data, size_t size, void *target, int *errno);

#endif
