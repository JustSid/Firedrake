//
//  cmos.h
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

#ifndef _CMOS_H_
#define _CMOS_H_

#include <libc/stdint.h>
#include <libcpp/bitfield.h>
#include "port.h"

namespace Sys
{
	namespace CMOS
	{
		enum class Register : uint8_t
		{
			Second = 0x0,
			Minute = 0x2,
			Hour   = 0x4,
			WDay   = 0x6,
			DMonth = 0x7,
			Month  = 0x8,
			Year   = 0x9,

			StateA = 0xa,
			StateB = 0xb
		};

		CPP_BITFIELD(Flags, uint8_t,
			DST       = (1 << 0),
			Hours24   = (1 << 1),
			Binary    = (1 << 2),
			Frequency = (1 << 3),
			Interrupt = (1 << 4),
			Alarm     = (1 << 5),
			Periodic  = (1 << 6),
			NoUpdate  = (1 << 7),
		);

		static inline void Write(Register reg, uint8_t data)
		{
			uint8_t temp = inb(0x70);
			outb(0x70, (temp & 0x80) | (static_cast<uint8_t>(reg) & 0x7f));
			outb(0x71, data);
		}

		static inline uint8_t Read(Register reg)
		{
			uint8_t temp = inb(0x70);
			outb(0x70, (temp & 0x80) | (static_cast<uint8_t>(reg) & 0x7f));

			return inb(0x71);
		}

		static inline void WriteRTCFlags(Flags flags)
		{
			Write(Register::StateB, static_cast<uint8_t>(flags));
		}
	}
}

#endif
