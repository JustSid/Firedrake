//
//  intrusive_list.h
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

#include <libc/stdbool.h>
#include <libc/stddef.h>

#ifndef _INTRUSIVE_LIST_H_
#define _INTRUSIVE_LIST_H_

namespace std
{
	template<class T>
	class intrusive_list
	{
	public:
		class member
		{
		public:
			member(T *value) :
				_value(value),
				_next(nullptr),
				_prev(nullptr)
			{}
			
			T *get() const { return _value; }
			
			member *next() const { return _next; }
			member *prev() const { return _prev; }
			
		private:
			friend class intrusive_list;
			
			T *_value;
			member *_next;
			member *_prev;
		};
		
		intrusive_list() :
			_head(nullptr),
			_tail(nullptr),
			_count(0)
		{}
		
		~intrusive_list()
		{
			member *value = _head;
			while(value)
			{
				member *temp = value->next();
				
				value->_next = nullptr;
				value->_prev = nullptr;
				
				value = temp;
			}
		}
		
		void push_back(member &value)
		{
			value._prev = _tail;
			
			if(_tail)
				_tail->_next = &value;
			
			if(!_head)
				_head = &value;
			
			_tail = &value;
			_count ++;
		}
		void push_front(member &value)
		{
			value._next = _head;
			
			if(_head)
				_head->_prev = &value;
			
			if(!_tail)
				_tail = &value;
			
			_head = &value;
			_count ++;
		}
		
		
		member *erase(member &value, bool next = true)
		{
			return erase(&value, next);
		}
		member *erase(member *value, bool next = true)
		{
			member *result = next ? value->next() : value->prev();
			
			if(value->_next)
				value->_next->_prev = value->_prev;
			if(value->_prev)
				value->_prev->_next = value->_next;
			
			value->_next = nullptr;
			value->_prev = nullptr;
			
			_count --;
			
			return result;
		}
		
		size_t size() const { return _count; }
		bool empty() const { return (_count == 0); }
		
		member *head() const { return _head; }
		member *tail() const { return _tail; }
		
	private:
		member *_head;
		member *_tail;
		size_t _count;
	};
}

#endif /* _INTRUSIVE_LIST_H_ */
