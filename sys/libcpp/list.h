//
//  list.h
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

#ifndef _LIST_H_
#define _LIST_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libc/stddef.h>

namespace cpp
{
	template<class T>
	class list
	{
	public:
		class node
		{
		public:
			friend class list;

			virtual ~node()
			{
				if(_list)
				{
					if(_next)
						_next->_prev = _prev;
					if(_prev)
						_prev->_next = _next;

					if(static_cast<node *>(_list->_tail) == this)
						_list->_tail = _prev;
					if(static_cast<node *>(_list->_head) == this)
						_list->_head = _next;

					_list->_count --;
				}
			}

			T *get_next() const
			{
				return _next;
			}

			T *get_prev() const
			{
				return _prev;
			}

		protected:
			node()
			{
				_next = _prev = nullptr;
				_list = nullptr;
			}

		private:
			T *_next;
			T *_prev;

			list *_list;
		};

		friend class node;

		list()
		{
			_head  = _tail = nullptr;
			_count = 0;
		}

		~list()
		{
			while(_head != _tail)
			{
				T *temp = _head;
				_head = _head->_next;

				temp->_list = nullptr;
				temp->_prev = nullptr;
				temp->_next = nullptr;
			}
		}

		void push_back(T *val)
		{
			if(val->_list)
				val->_list->erase(val);

			if(_tail)
			{
				val->_prev = _tail;
				_tail->_next = val;
			}
			else
			{
				_head = val;
			}

			_count ++;
			_tail = val;

			val->_list = this;
		}

		void push_front(T *val)
		{
			if(val->_list)
				val->_list->erase(val);

			if(_head)
			{
				val->_next = _head;
				_head->_prev = val;
			}
			else
			{
				_tail = val;
			}

			_count ++;
			_head = val;

			val->_list = this;
		}

		void erase(T *val)
		{
			if(val->_list == this)
			{
				if(val->_next)
					val->_next->_prev = val->_prev;

				if(val->_prev)
					val->_prev->_next = val->_next;

				if(_head == val)
					_head = val->_next;

				if(_tail == val)
					_tail = val->_prev;

				_count --;
			}
		}


		T *get_head() const
		{
			return _head;
		}

		T *get_tail() const
		{
			return _tail;
		}

		size_t get_count() const
		{
			return _count;
		}

	private:
		T *_head;
		T *_tail;

		size_t _count;
	};
}

#endif /* _LIST_H_ */
