//
//  IOService.h
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

#ifndef _IOSERVICE_H_
#define _IOSERVICE_H_

#include "../core/IOObject.h"

namespace IO
{
	class Service : public Object
	{
	public:
		enum class Type
		{
			HID
		};

		enum HIDSubtype
		{
			Keyboard
		};

		virtual bool Start();
		virtual void Stop();

		Type GetType() const { return _type; }
		uint32_t GetSubType() const { return _subType; }

	protected:
		Service *InitWithType(Type type, uint32_t subType);

	private:
		Type _type;
		uint32_t _subType;

		IODeclareMeta(Service)
	};
}

#endif /* _IOSERVICE_H_ */
