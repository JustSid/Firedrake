//
//  mutex.cpp
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

#include <os/waitqueue.h>
#include <os/scheduler/scheduler.h>
#include <machine/interrupts/interrupts.h>

#include "mutex.h"

namespace OS
{
	void Mutex::Unlock()
	{
		_owned.store(false, std::memory_order_release);

		switch(_lockMode)
		{
			case Mode::Simple:
				break;
			case Mode::NoScheduler:
				if(_wasEnabled)
					Scheduler::GetScheduler()->EnableCPU(nullptr); // todo: Maybe keep track of the disabled CPU?!
				break;
			case Mode::NoInterrupts:
				if(_wasEnabled)
					Sys::EnableInterrupts();
				break;
		}
	}

	void Mutex::Lock(Mode mode)
	{
		if(TryLock(mode))
			return;

		while(1)
		{
			for(size_t i = 0; i < 10000; i ++)
			{
				if(TryLock(mode))
					return;

				Sys::CPUPause();
			}
		}
	}

	bool Mutex::TryLock(Mode mode)
	{
		bool expected = false;
		if(_owned.compare_exchange(expected, true, std::memory_order_release))
		{
			_lockMode = mode;

			switch(_lockMode)
			{
				case Mode::Simple:
					break;
				case Mode::NoScheduler:
					_wasEnabled = Scheduler::GetScheduler()->DisableCPU(nullptr);
				case Mode::NoInterrupts:
					_wasEnabled = Sys::DisableInterrupts();
					break;
			}

			return true;
		}

		return false;
	}
}
