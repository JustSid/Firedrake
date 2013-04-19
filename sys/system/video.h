//
//  video.h
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

/*
 * Overview:
 * Simple video out for the time the system initializes and has no other video driver open
 */
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <prefix.h>

typedef enum
{
	vd_color_black			= 0x0, // 020
	vd_color_blue			= 0x1, // 021
	vd_color_green			= 0x2, // 022
	vd_color_cyan			= 0x3, // 023
	vd_color_red			= 0x4, // 024
	vd_color_magenta		= 0x5, // 025
	vd_color_brown			= 0x6, // 026
	vd_color_lightGray		= 0x7, // 027
	vd_color_darkGray		= 0x8, // 030
	vd_color_lightBlue		= 0x9, // 031
	vd_color_lightGreen		= 0xA, // 032
	vd_color_lightCyan		= 0xB, // 033
	vd_color_lightRed		= 0xC, // 034
	vd_color_lightMagenta	= 0xD, // 035
	vd_color_yellow			= 0xE, // 036
	vd_color_white			= 0xF  // 037
} vd_color_t;

void vd_clear();

void vd_scrollLines(uint32_t lines);
void vd_setCursor(uint32_t x, uint32_t y);
void vd_setColor(vd_color_t color, bool foreground);

void vd_writeString(const char *string);

static inline uint8_t vd_translateColor(vd_color_t color)
{
	uint8_t character = (uint8_t)color;
	return character + 16;
}

#endif /* _VIDEO_H_ */
