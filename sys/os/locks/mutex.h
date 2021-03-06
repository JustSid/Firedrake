//
//  mutex.h
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

namespace OS
{
	class Mutex
	{
	public:
		enum class Mode
		{
			Simple, // Only locks the mutex
			NoScheduler, // Locks the mutex and disables the scheduler
			NoInterrupts // Locks the mutex and disables interrupts
		};

		Mutex() :
			_owned(false)
		{}

		~Mutex()
		{
			if(_owned.load())
				panic("Mutex destructor called while mutex is locked");
		}

		void Unlock();
		void Lock(Mode mode = Mode::Simple);
		bool TryLock(Mode mode);

	private:
		std::atomic<bool> _owned;
		Mode _lockMode;
		bool _wasEnabled;
	};
}

#endif /* _OS_MUTEX_H_ */
