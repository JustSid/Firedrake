//
//  type_traits.h
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

#ifndef _TYPE_TRAITS_H_
#define _TYPE_TRAITS_H_

namespace std
{
	template<class T>
	struct remove_const { typedef T type; };
	template<class T>
	struct remove_const<const T> { typedef T type; };

	template<class T>
	struct remove_reference { typedef T type; };

	template<class T>
	struct remove_reference<T &> { typedef T type; };

	template<class T>
	struct remove_reference<T &&> { typedef T type; };

	template<class T>
	struct remove_pointer { typedef T type; };

	template<class T>
	struct remove_pointer<T*> { typedef T type; };

	template<class T>
	struct remove_pointer<T* const> { typedef T type; };

	template<class T>
	struct remove_pointer<T* volatile> { typedef T type; };

	template<class T>
	struct remove_pointer<T* const volatile> { typedef T type; };

	template <class T, T v>
	struct integral_constant
	{
		static constexpr const T value = v;
		typedef T value_type;
		typedef integral_constant type;


		constexpr operator value_type() const { return value; }
		constexpr value_type operator ()() const { return value; }
	};

	template <class T, T v>
	constexpr const T integral_constant<T, v>::value;

	typedef integral_constant<bool, true>  true_type;
	typedef integral_constant<bool, false> false_type;

	template<class T, class U>
	struct is_same : public false_type {};
	template<class T>
	struct is_same<T, T> : public true_type {};	

	template<class T>
	inline T&& forward(typename std::remove_reference<T>::type &t) noexcept
	{
		return static_cast<T &&>(t);
	}

	template<class T>
	inline typename remove_reference<T>::type &&move(T &&t) noexcept
	{
	    typedef typename remove_reference<T>::type Type;
	    return static_cast<Type &&>(t);
	}
}

#endif /* _TYPE_TRAITS_H_ */
