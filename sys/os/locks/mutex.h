//
//  mutext.h
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

#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libcpp/atomic.h>
#include <kern/kprintf.h>
#include <machine/cpu.h>
#include <kern/panic.h>
#include <os/waitqueue.h>

namespace OS
{
	class Mutex
	{
	public:
		Mutex() :
			_owned(false)
		{}

		~Mutex()
		{
			if(_owned.load())
				panic("Mutex destructor called while mutex is locked");
		}

		void Unlock()
		{
			_owned.store(false, std::memory_order_release);
			WakeupOne(this);
		}

		void Lock()
		{
			if(TryLock())
				return;

			while(1)
			{
				for(size_t i = 0; i < 10000; i ++)
				{
					if(TryLock())
						return;

					Sys::CPUPause();
				}
				
				Wait(this).Suppress();
			}
		}

		bool TryLock()
		{
			bool expected = false;
			return _owned.compare_exchange(expected, true, std::memory_order_release);
		}

	private:
		std::atomic<bool> _owned;
	};
}

#endif /* _OS_MUTEX_H_ */
