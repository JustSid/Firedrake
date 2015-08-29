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
#include <kern/panic.h>
#else
#include "IORuntime.h"
#endif

namespace IO
{
	class Object;
	struct __CatalogueHelper;

	class MetaClass
	{
	public:
		MetaClass *GetSuperClass() const;

		const char *GetName() const;
		const char *GetFullname() const;

		bool InheritsFromClass(MetaClass *other) const;

		virtual Object *Alloc() const { panic("Alloc() called but not provided"); }
		virtual bool SupportsAllocation() const { return false; }

	protected:
		MetaClass() {}
		MetaClass(MetaClass *super, const char *signature);

	private:
		MetaClass *_super;

		char *_name;
		char *_fullname;
	};

	template<class T>
	class __MetaClassTraitNull0 : public virtual MetaClass
	{};

	template<class T>
	class MetaClassTraitConstructable : public virtual MetaClass
	{
	public:
		T *Alloc() const override { return T::Alloc(); }
		bool SupportsAllocation() const override { return true; }
	};


	template<class T, class... Traits>
	class __ConcreteMetaClass : public virtual MetaClass, public Traits...
	{};


	class Catalogue
	{
	public:
		friend struct __CatalogueHelper;
		friend class MetaClass;

		static Catalogue *GetSharedInstance();

		MetaClass *GetClassWithName(const char *name);

#if __KERNEL
		typedef void (*__MetaClassGetter)();

		static void __MarkClass(__MetaClassGetter entry);
#endif

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
