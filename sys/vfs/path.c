//
//  path.c
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

#include <errno.h>
#include <libc/string.h>
#include <memory/memory.h>
#include <system/syslog.h>
#include "filesystem.h"
#include "path.h"

extern vfs_node_t *vfs_rootNode;

vfs_node_t *vfs_resolvePath(const char *path, vfs_context_t *context, int *errno)
{
	vfs_path_t *request = vfs_pathCreate(path, context);

	while(vfs_pathNextElement(request, errno))
	{}

	vfs_node_t *node = vfs_pathCurrent(request);
	vfs_pathDestroy(request);

	return node;
}

vfs_node_t *vfs_resolvePathToParent(const char *path, vfs_context_t *context, char *name, int *errno)
{
	vfs_path_t *request = vfs_pathCreate(path, context);
	vfs_node_t *node = NULL;

	while(!vfs_pathIsAtEnd(request))
	{
		node = vfs_pathCurrent(request);
		bool result = vfs_pathNextElement(request, errno);

		if(!result && !vfs_pathIsAtEnd(request))
		{
			*errno = ENOENT;

			vfs_pathDestroy(request);
			return NULL;
		}

		if(!result && vfs_pathIsAtEnd(request))
		{
			if(name)
				strcpy(name, vfs_pathCurrentName(request));

			vfs_pathDestroy(request);
			return node;
		}
	}

	if(name)
		strcpy(name, vfs_pathCurrentName(request));

	vfs_pathDestroy(request);
	return node ? (vfs_node_t *)node->parent : NULL;
}

vfs_path_t *vfs_pathCreate(const char *path, vfs_context_t *context)
{
	assert(path);
	assert(context);

	vfs_path_t *request = halloc(NULL, sizeof(vfs_path_t));
	if(request)
	{
		request->path = (char *)path;
		request->element = request->path;

		request->context = context;
		request->node  = context->chdir;
		request->atEnd = false;
	}

	return request;
}

void vfs_pathDestroy(vfs_path_t *path)
{
	hfree(NULL, path);
}


bool vfs_pathPeekElement(vfs_path_t *path)
{
	while(*path->element == '/')
		path->element ++;

	return (*path->element != '\0');
}

bool vfs_pathNextElement(vfs_path_t *path, int *errno)
{
	if(!path->element || path->atEnd)
	{
		path->atEnd = true;
		return false;
	}

	if(!vfs_pathPeekElement(path))
	{
		path->atEnd = true;
		return false;
	}

	char *element = strpbrk(path->element, "/");
	if(!element)
	{
		if(strlen(path->element) == 0)
		{
			path->element = NULL;
			path->atEnd = true;
			
			return false;
		}
		
		element = path->element + strlen(path->element);
	}

	strlcpy(path->name, path->element, element - path->element);
	if(strcmp(path->name, "..") == 0)
	{
		path->node = (vfs_node_t *)path->node->parent;
		path->element = element;
	}
	else
	{
		vfs_instance_t *instance = path->node->instance;
		path->node = instance->callbacks.accessNode(instance, path->context, path->node, path->name, errno);
	}

	path->element = element;
	path->atEnd = (strpbrk(path->element, "/") == NULL);

	return (path->node != NULL);
}

bool vfs_pathIsAtEnd(vfs_path_t *path)
{
	return (path->atEnd || !vfs_pathPeekElement(path));
}

vfs_node_t *vfs_pathCurrent(vfs_path_t *path)
{
	return path->node;
}

char *vfs_pathCurrentName(vfs_path_t *path)
{
	return path->name;
}
