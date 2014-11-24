//
//  IOCatalogue.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#ifndef _IOCATALOGUE_H_
#define _IOCATALOGUE_H_

#include <libcpp/vector.h>
#include <libc/sys/spinlock.h>
#include <libc/string.h>

#if __KERNEL
#include <kern/kern_return.h>
#endif

namespace IO
{
	class MetaClass
	{
	public:
		MetaClass *GetSuperClass() const;

		const char *GetName() const;
		const char *GetFullname() const;

		bool InheritsFromClass(MetaClass *other) const;

	protected:
		MetaClass(MetaClass *super, const char *signature);

	private:
		MetaClass *_super;

		char *_name;
		char *_fullname;
	};

	class Catalogue
	{
	public:
		static Catalogue *GetSharedInstance();

		MetaClass *GetClassWithName(const char *name);

	protected:
		Catalogue();

	private:
		void AddMetaClass(MetaClass *meta);

		std::vector<MetaClass *> _classes;
		spinlock_t _lock;
	};

#if __KERNEL
	KernReturn<void> CatalogueInit();
#endif
}

#endif /* _IOCATALOGUE_H_ */
