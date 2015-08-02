//
//  IOModule.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#ifndef _IOMODULE_H_
#define _IOMODULE_H_

#include "../core/IOObject.h"
#include "../core/IOArray.h"
#include "IOThread.h"
#include "IOServiceProvider.h"
#include <kmod.h>

namespace IO
{
	class Module : public Object
	{
	public:
		Module *InitWithKmod(kmod_t *mod);

		virtual void Publish();
		virtual void Unpublish();

		kmod_t *GetKmod() const { return _module; }

	protected:
		void AddServiceProvider(ServiceProvider *provider);
		void RemoveServiceProvider(ServiceProvider *provider);

	private:
		kmod_t *_module;
		Array *_providers;

		IODeclareMeta(Module)
	};
}

#define IOModuleRegister(class) \
	extern "C" { \
		static IO::Module *__sharedModule = nullptr; \
		bool _kern_start(kmod_t *kmod) \
		{ \
			__sharedModule = class::Alloc()->InitWithKmod(kmod); \
			__sharedModule->Publish(); \
			return (__sharedModule != nullptr); \
		} \
		void _kern_stop(__unused kmod_t *kmod) \
		{ \
			__sharedModule->Unpublish(); \
		} \
	}

#endif /* _IOMODULE_H_ */
