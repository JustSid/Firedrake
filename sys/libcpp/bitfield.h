//
//  bitfield.h
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

#ifndef _BITFIELD_H_
#define _BITFIELD_H_

#include "type_traits.h"

namespace cpp
{
	template<class T, bool overloadT = false>
	struct Bitfield;


	template<class T>
	struct Bitfield<T, false>
	{
		Bitfield() :
			_value(0)
		{}
		Bitfield(T value) :
			_value(value)
		{}

		
		T operator =(T value)
		{
			_value = value;
			return value;
		}
		operator T() const
		{
			return _value;
		}

		T operator ~() const
		{
			return ~_value;
		}
		T operator &(int value) const
		{
			return _value & value;
		}
		T operator |(int value) const
		{
			return _value | value;
		}
		
		T &operator &=(int value)
		{
			return _value &= value;
		}
		T &operator |=(int value)
		{
			return _value |= value;
		}

		T _value;
	};

	template<class T>
	struct Bitfield<T, true>
	{
		Bitfield() :
			_value(0)
		{}
		Bitfield(T value) :
			_value(value)
		{}

		
		T operator =(T value)
		{
			_value = value;
			return value;
		}
		operator T() const
		{
			return _value;
		}

		T operator ~() const
		{
			return ~_value;
		}
		T operator &(T value) const
		{
			return _value & value;
		}
		T operator |(T value) const
		{
			return _value | value;
		}
		
		T &operator &=(T value)
		{
			return _value &= value;
		}
		T &operator |=(T value)
		{
			return _value |= value;
		}

		T _value;
	};
}

#endif /* _BITFIELD_H_ */
