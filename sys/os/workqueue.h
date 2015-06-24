//
//  workqueue.h
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

#ifndef _WORKQUEUE_H_
#define _WORKQUEUE_H_

#include <prefix.h>
#include <libc/sys/spinlock.h>
#include <libc/stdint.h>

namespace OS
{
	/**
	 * The work queue class is supposed to be used from within an interrupt context
	 * to communicate work to non-interrupt context kernel workers. They are per CPU
	 * and writes and reads happen with interrupts disabled. However, reads from the work queue
	 * from another CPU are additionally protected by a spin lock which is only taken
	 * when reading from another CPUs work queue.
	 *
	 * Inserts into the list can fail and should be handled gracefully
	 *
	 * Accessing other CPUs work queues requires the usage of the xxxRemote methods and must
	 * never be performed with the regular methods. Likewise, accessing the current CPUs work
	 * queue must only be done via the non suffixed methods.
	 **/
	class WorkQueue
	{
	public:
		typedef void (*Callback)(void *);
		struct Entry
		{
			Callback callback;
			void *context;
			Entry *next;
		};

		WorkQueue();

		bool PushEntry(Callback callback, void *context);

		Entry *PopAll();
		Entry *PopAllRemote();

		void RefurbishList(Entry *entry);
		void RefurbishListRemote(Entry *entry);

	private:
		void _ExtendFreeList(size_t size);

		spinlock_t _readWriterLock;
		bool _exhausted;
		Entry *_freeListHead;
		Entry *_workListHead;
	};
}

#endif /* _WORKQUEUE_H_ */
