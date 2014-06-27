//
//  queue.h
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

			entry(const entry &other) :
				_data(other._data),
				_next(other._next),
				_prev(other._next),
				_queue(other._queue)
			{}

			entry(T *data) :
				_data(data),
				_next(nullptr),
				_prev(nullptr),
				_queue(nullptr)
			{}

			T *get() const { return _data; }

			queue *get_queue() { return _queue; }

			void remove_from_queue()
			{
				if(_queue)
					_queue->erase(this);
			}

			entry *get_next() const { return _next; }
			entry *get_prev() const { return _prev; }

		private:
			entry *_next;
			entry *_prev;

			T *_data;
			queue *_queue;
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

				_head->_next  = _head->_prev = nullptr;
				_head->_queue = nullptr;
				_head = temp;
			}
		}

		entry *get_head() const
		{
			return _head;
		}

		entry *get_tail() const
		{
			return _tail;
		}


		void insert_back(entry& tentry)
		{
			entry *ptr = &tentry;
			if(ptr->_queue)
				ptr->_queue->erase(*ptr);

			ptr->_queue = this;

			if(_tail)
			{
				ptr->_prev = _tail;
				ptr->_next = nullptr;

				_tail->_next = ptr;
				_tail = ptr;
			}
			else
			{
				_head = _tail = ptr;
				ptr->_next = ptr->_prev = nullptr;
			}
		}

		void insert_front(entry& tentry)
		{
			entry *ptr = &tentry;
			if(ptr->_queue)
				ptr->_queue->erase(*ptr);

			ptr->_queue = this;

			if(_head)
			{
				ptr->_next = _head;
				ptr->_prev = nullptr;

				_head->_prev = ptr;
				_head = ptr;
			}
			else
			{
				_head = _tail = ptr;
				ptr->_next = ptr->_prev = nullptr;
			}
		}

		void erase(entry& tentry)
		{
			entry *ptr = &tentry;

			if(ptr->_queue == this)
			{
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
				ptr->_queue = nullptr;
			}
		}

	private:
		entry *_head;
		entry *_tail;
	};
}

#endif /* _QUEUE_H_ */
