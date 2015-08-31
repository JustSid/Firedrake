//
//  IOObject.h
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

#ifndef _IOOBJECT_H_
#define _IOOBJECT_H_

#include <libcpp/atomic.h>
#include <libcpp/type_traits.h>
#include <libc/stdint.h>

#include "IOCatalogue.h"

namespace IO
{
	class Object
	{
	public:
		virtual ~Object();

		Object *Retain();
		Object *Release();

		virtual size_t GetHash() const;
		virtual bool IsEqual(Object *other) const;
		bool IsKindOfClass(MetaClass *other) const;

		static void InitialWakeUp(MetaClass *meta);

		template<class T>
		T *Downcast()
		{
			if(IsKindOfClass(T::GetMetaClass()))
				return static_cast<T *>(this);
			
			return nullptr;
		}

		virtual MetaClass *GetClass() const;
		static MetaClass *GetMetaClass();

	protected:
		Object();

		Object *Init();
		virtual void Dealloc();

	private:
		mutable std::atomic<uint32_t> _references;
	};

#define IODeclareMeta(cls) \
	public: \
		cls *Retain() \
		{ \
			return static_cast<cls *>(Object::Retain()); \
		} \
		cls *Release() \
		{ \
			return static_cast<cls *>(Object::Release()); \
		} \
		static cls *Alloc() \
		{ \
			cls *result = new cls(); \
			return result; \
		} \
		IO::MetaClass *GetClass() const override; \
		static IO::MetaClass *GetMetaClass();

#define IODeclareMetaVirtual(cls) \
	public: \
		cls *Retain() \
		{ \
			return static_cast<cls *>(Object::Retain()); \
		} \
		cls *Release() \
		{ \
			return static_cast<cls *>(Object::Release()); \
		} \
		IO::MetaClass *GetClass() const override; \
		static IO::MetaClass *GetMetaClass();

#define __IODefineMeta(cls, super, ...) \
	class cls##MetaType : public IO::__ConcreteMetaClass<cls, __VA_ARGS__> \
	{ \
	public: \
		cls##MetaType() : \
			MetaClass(super::GetMetaClass(), __PRETTY_FUNCTION__) \
		{} \
	}; \
	void *__kIO##cls##__metaClass = nullptr; \
	IO::MetaClass *cls::GetClass() const \
	{ \
		return cls::GetMetaClass(); \
	} \
	IO::MetaClass *cls::GetMetaClass() \
	{ \
		if(!__kIO##cls##__metaClass) \
			__kIO##cls##__metaClass = new cls##MetaType(); \
		return reinterpret_cast<cls##MetaType *>(__kIO##cls##__metaClass); \
	}

#ifndef __KERNEL

#define __IODefineIOCoreMetaInternal(cls, super, ...) \
	class cls##MetaType : public IO::__ConcreteMetaClass<cls, __VA_ARGS__> \
	{ \
	public: \
		cls##MetaType() : \
			MetaClass(super::GetMetaClass(), __PRETTY_FUNCTION__) \
		{} \
	}; \
	void *__kIO##cls##__metaClass = nullptr; \
	IO::MetaClass *cls::GetClass() const \
	{ \
		return cls::GetMetaClass(); \
	} \
	IO::MetaClass *cls::GetMetaClass() \
	{ \
		if(!__kIO##cls##__metaClass) \
			__kIO##cls##__metaClass = IO::Catalogue::GetSharedInstance()->GetClassWithName("IO::" #cls); \
		return reinterpret_cast<IO::MetaClass *>(__kIO##cls##__metaClass); \
	}

#endif /* __KERNEL */

#define __IODefineScopedMeta(scope, cls, super, ...) \
	class scope##cls##MetaType : public IO::__ConcreteMetaClass<scope::cls, __VA_ARGS__> \
	{ \
	public: \
		scope##cls##MetaType() : \
			MetaClass(super::GetMetaClass(), __PRETTY_FUNCTION__) \
		{} \
	}; \
	void *__kIO##scope##cls##__metaClass = nullptr; \
	IO::MetaClass *scope::cls::GetClass() const \
	{ \
		return cls::GetMetaClass(); \
	} \
	IO::MetaClass *scope::cls::GetMetaClass() \
	{ \
		if(!__kIO##scope##cls##__metaClass) \
			__kIO##scope##cls##__metaClass = new scope##cls##MetaType(); \
		return reinterpret_cast<cls##MetaType *>(__kIO##scope##cls##__metaClass); \
	}

#ifndef __KERNEL

#define IODefineMeta(cls, super) \
	__IODefineMeta(cls, super, IO::MetaClassTraitConstructable<cls>) \
	IO_REGISTER_INITIALIZER(cls##Init, cls::InitialWakeUp(cls::GetMetaClass()))

#define IODefineMetaVirtual(cls, super) \
	__IODefineMeta(cls, super, IO::__MetaClassTraitNull0<cls>) \
	IO_REGISTER_INITIALIZER(cls##Init, cls::InitialWakeUp(cls::GetMetaClass()))

#define __IODefineIOCoreMeta(cls, super) \
	__IODefineIOCoreMetaInternal(cls, super, IO::MetaClassTraitConstructable<cls>) \
	IO_REGISTER_INITIALIZER(cls##Init, cls::GetMetaClass())

#else

	namespace __IO
	{
		class Initializer
		{
		public:
			Initializer(Catalogue::__MetaClassGetter fn)
			{
				Catalogue::__MarkClass(fn);
			}
		};
	}

#define __IORegisterClass(cls) \
	namespace { \
		void __##cls##RegisterAndWakeUp() \
		{ \
			cls::InitialWakeUp(cls::GetMetaClass()); \
		} \
		static IO::__IO::Initializer __##cls##Bootstrap (&__##cls##RegisterAndWakeUp); \
	}

#define IODefineMeta(cls, super) \
	__IODefineMeta(cls, super, IO::MetaClassTraitConstructable<cls>) \
	__IORegisterClass(cls)

#define IODefineMetaVirtual(cls, super) \
	__IODefineMeta(cls, super, IO::__MetaClassTraitNull0<cls>) \
	__IORegisterClass(cls)

#define __IODefineIOCoreMeta(cls, super) \
	__IODefineMeta(cls, super, IO::MetaClassTraitConstructable<cls>) \
	__IORegisterClass(cls)

#endif /* __KERNEL */


	// ---------------------
	// MARK: -
	// MARK: Helper
	// ---------------------

	template<class T>
	static void SafeRelease(T *&object)
	{
		if(object)
		{
			object->Release();
			object = nullptr;
		}
	}
	
	template<class T>
	static T *SafeRetain(T *object)
	{
		return object ? static_cast<T *>(object->Retain()) : nullptr;
	}

	// ---------------------
	// MARK: -
	// MARK: Smart pointers
	// ---------------------

	template<class T>
	struct StrongRef
	{
		StrongRef() :
			_value(nullptr)
		{}
		
		StrongRef(T *value) :
			_value(nullptr)
		{
			Assign(value);
		}
		
		StrongRef(const StrongRef<T> &other) :
			_value(nullptr)
		{
			Assign(other._value);
		}
		~StrongRef()
		{
			if(_value)
				_value->Release();
		}
		
		StrongRef &operator =(const StrongRef<T> &other)
		{
			Assign(other._value);
			return *this;
		}
		
		StrongRef &operator =(T *value)
		{
			Assign(value);
			return *this;
		}
		
		operator T* () const
		{
			return Load();
		}
		
		
		T *operator ->() const
		{
			return const_cast<T *>(_value);
		}
		
		T *Load() const
		{
			T *object = const_cast<T *>(_value);
			return object;
		}
		
	private:
		void Assign(T *value)
		{
			if(_value)
				_value->Release();
			
			if((_value = value))
				_value->Retain();
		}
		
		T *_value;
	};

#define IOTransferRef(t) \
	static_cast<decltype(t)>((IO::StrongRef<std::remove_pointer<decltype(t)>::type>{(t)})->Release())
}

#endif /* _IOOBJECT_H_ */
