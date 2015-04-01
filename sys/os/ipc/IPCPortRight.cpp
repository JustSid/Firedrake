//
//  IPCPortRight.cpp
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

#include "IPCPortRight.h"
#include "IPCPort.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(PortRight, Port)

		PortRight *PortRight::Init(uint16_t name, Port *port, Task *holder, bool oneTime)
		{
			if(!Port::InitAsPortRight(port, name))
				return nullptr;

			_name = name;
			_oneTime = oneTime;
			_port = IO::SafeRetain(port);
			_holder = holder;

			return this;
		}

		void PortRight::Dealloc()
		{
			IO::SafeRelease(_port);
			Port::Dealloc();
		}

		bool Port::IsPortRight() const
		{
			return IPCIsPortRight(_portName);
		}
	}
}