//
//  iomodule.c
//  Firedrake
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

#include <system/syslog.h>
#include <libc/string.h>
#include "iomodule.h"
#include "iostore.h"

io_module_t *io_moduleCreateWithLibrary(io_library_t *library)
{
	io_module_t *module = halloc(NULL, sizeof(io_module_t));
	if(module)
	{
		module->library   = library;
		module->module    = NULL;

		module->publish   = (io_module_publish_t)io_libraryFindSymbol(library, "IOModulePublish");
		module->unpublish = (io_module_unpublish_t)io_libraryFindSymbol(library, "IOModuleUnpublish");

		if(!module->publish || !module->unpublish)
		{
			dbg("%s doesn't provide IOModule entry points!\n", library->path);
			hfree(NULL, module);

			return false;
		}
		
		// Call the init functions
		for(size_t i=0; i<library->initArrayCount; i++)
		{
			io_module_init_t init = (io_module_init_t)(library->initArray[i]);
			init();
		}

		// Call IOModulePublish
		module->module = module->publish();
		if(!module->module)
		{
			hfree(NULL, module);
			return false;
		}
	}

	return module;
}

void io_moduleDelete(io_module_t *module)
{
	module->unpublish(module->module);
	hfree(NULL, module);
}
