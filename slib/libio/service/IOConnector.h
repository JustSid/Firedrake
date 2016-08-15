//
//  IOConnector.h
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

#ifndef _IOCONNECTOR_H_
#define _IOCONNECTOR_H_

#include <core/IOObject.h>
#include <libc/ipc/ipc_port.h>
#include "IOThread.h"
#include "IOService.h"
#include "../core/IOString.h"

namespace IO
{
	class Connector : public Object
	{
	public:
		typedef void (*DispatchCallback)(void *context, uint8_t *buffer, size_t size, uint8_t *outResponse, size_t &outResponseSize);

		Connector *InitWithName(const char *name);
		Connector *InitWithService(Service *service);

		void Dealloc() override;

		void Publish();
		void AddDispatchEntry(uint32_t vector, DispatchCallback callback, bool hasResponse, void *context);

	private:
		struct DispatchEntry
		{
			uint32_t vector;
			bool hasResponse;
			DispatchCallback callback;
			void *context;
		};

		void ThreadEntry(Thread *thred);
		bool DispatchMessage(uint32_t vector, uint8_t *buffer, size_t size, uint8_t *outResponse, size_t &outResponseSize) const;

		bool _published;

		Thread *_thread;
		String *_name;

		ipc_port_t _port;
		std::vector<DispatchEntry> _dispatchEntries;

		IODeclareMeta(Connector)
	};
}

#endif /* _IOCONNECTOR_H_ */
