//
//  queue.h
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

#ifndef _QUEUE_H_
#define _QUEUE_H_

namespace cpp
{
	template<class T>
	class queue
	{
	public:
		class entry
		{
		public:
			friend class queue;

			entry(const entry& other) :
				_data(other._data),
				_next(other._next),
				_prev(other._next)
			{}

			entry(T *data) :
				_data(data),
				_next(nullptr),
				_prev(nullptr)
			{}

			T *get() const { return _data; }

			entry *get_next() const { return _next; }
			entry *get_prev() const { return _prev; }

		private:
			entry *_next;
			entry *_prev;

			T *_data;
		};

		queue() :
			_head(nullptr),
			_tail(nullptr)
		{}

		~queue()
		{
			while(_head)
			{
				entry *temp = _head->_next;

				_head->_next = _head->_prev = nullptr;
				_head = temp;
			}
		}

		void insert_back(entry& tentry)
		{
			entry *ptr = &tentry;

			if(_tail)
			{
				ptr->_prev = _tail;
				_tail->_next = ptr;
				_tail = ptr;
			}
			else
			{
				_head = _tail = ptr;
			}
		}

		void insert_front(entry& tentry)
		{
			entry *ptr = &tentry;

			if(_head)
			{
				ptr->_next = _head;
				_head->_prev = ptr;
				_head = ptr;
			}
			else
			{
				_head = _tail = ptr;
			}
		}

		void erase(entry& tentry)
		{
			entry *ptr = &tentry;

			if(ptr == _head)
			{
				_head = ptr->_next;
				_head->_prev = nullptr;
			}

			if(ptr == _tail)
			{
				_tail = ptr->_prev;
				_tail->_next = nullptr;
			}

			ptr->_next = ptr->_prev = nullptr;
		}

	private:
		entry *_head;
		entry *_tail;
	};
}

#endif /* _QUEUE_H_ */
