//
//  cmos.h
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

#ifndef _CMOS_H_
#define _CMOS_H_

#include <libc/stdint.h>
#include "port.h"

#define CMOS_REGISTER_SECOND    0x00
#define CMOS_REGISTER_MINUTE    0x02
#define CMOS_REGISTER_HOUR      0x04
#define CMOS_REGISTER_WDAY      0x06
#define CMOS_REGISTER_DMONTH    0x07
#define CMOS_REGISTER_MONTH	    0x08
#define CMOS_REGISTER_YEAR      0x09

#define CMOS_REGISTER_STATEA 0x0A
#define CMOS_REGISTER_STATEB 0x0B

#define CMOS_RTC_FLAG_DST        (1 << 0)
#define CMOS_RTC_FLAG_24HOURS    (1 << 1)
#define COMS_RTC_FLAG_BINARY     (1 << 2)
#define CMOS_RTC_FLAG_FREQUENCY  (1 << 3)
#define CMOS_RTC_FLAG_INTERRUPT  (1 << 4)
#define CMOS_RTC_FLAG_ALARM      (1 << 5)
#define CMOS_RTC_FLAG_PERIODIC   (1 << 6)
#define CMOS_RTC_FLAG_NOUPDATE   (1 << 7)


static inline uint8_t cmos_read(uint8_t offset)
{
	uint8_t temp = inb(0x70);
	outb(0x70, (temp & 0x80) | (offset & 0x7f));

	return inb(0x71);
}

static inline void cmos_write(uint8_t offset, uint8_t data)
{
	uint8_t temp = inb(0x70);
	outb(0x70, (temp & 0x80) | (offset & 0x7f));
	outb(0x71, data);
}

static inline void cmos_write_rtc_flags(uint8_t flags)
{
	cmos_write(CMOS_REGISTER_STATEB, flags);
}

#endif
