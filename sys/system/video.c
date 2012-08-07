//
//  video.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include <libc/string.h>
#include "video.h"
#include "port.h"

static uint8_t *_vd_address = (uint8_t *)0xB8000;
static uint32_t _vd_width  = 80;
static uint32_t _vd_height = 25;
static uint32_t _vd_cursorX = 0;
static uint32_t _vd_cursorY = 0;

// Mark: Private inlined functions
static inline void __vd_setColor(uint32_t x, uint32_t y, bool foreground, vd_color_t color)
{
	uint32_t index = (y * _vd_width + x) * 2;
	uint8_t otherColor = _vd_address[index + 1];
	uint8_t result;
	
	otherColor	= (foreground == true) ? otherColor >> 4 : otherColor & 0x0F;
	result 		= (foreground == true) ? (otherColor << 4) | (color & 0x0F) : (color << 4) | (otherColor & 0x0F);
	
	_vd_address[index + 1] = result;
}

static inline void __vd_setCharacter(uint32_t x, uint32_t y, char character)
{
	uint32_t index = (y * _vd_width + x) * 2;
	_vd_address[index] = character;
}

static inline void __vd_setCursor(uint32_t x, uint32_t y)
{
	uint16_t pos = y * _vd_width + x;
	
	_vd_cursorX = x;
	_vd_cursorY = y;
	
	outb(0x3D4, 0x0F);
   	outb(0x3D5, (uint8_t)(pos & 0xFF));
	
   	outb(0x3D4, 0x0E);
   	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
	
	__vd_setColor(x, y, true, vd_color_lightGray); // Give the cursor a color so that its visible
}

static inline void __vd_scrollLine()
{
	for(uint32_t i=0; i<_vd_height - 1; i++)
	{
		memcpy(&_vd_address[(i * _vd_width) * 2], &_vd_address[((i + 1) * _vd_width) * 2], _vd_width * 2);	
	}

	memset(&_vd_address[((_vd_height - 1) * _vd_width) * 2], 0, _vd_width * 2); // Blank out the last line
	_vd_cursorY --;
}


// MARK: Public functions
void vd_clear()
{
	_vd_cursorX = 0;
	_vd_cursorY = 0;
	
	memset(_vd_address, 0, _vd_width * _vd_height * 2);
	__vd_setCursor(_vd_cursorX, _vd_cursorY);
}

void vd_setCursor(uint32_t x, uint32_t y)
{
	__vd_setCursor(x, y);
}

void vd_scrollLines(uint32_t lines)
{
	for(uint32_t i=0; i<lines; i++)
		__vd_scrollLine();
	
	__vd_setCursor(_vd_cursorX, _vd_cursorY); // Update the cursor so that it gets it gray color again.
}

void vd_setColor(uint32_t x, uint32_t y, bool foreground, vd_color_t color, size_t size)
{
	uint32_t index = (y * _vd_width + x) * 2; 
	size *= 2; // Remember that each character takes up 16 bytes

	for(size_t i=0; i<size; i+=2)
	{
		uint8_t otherColor = _vd_address[index + i + 1];
		uint8_t result;
		
		otherColor 	= foreground ? otherColor >> 4 : otherColor & 0x0F; // Grab the other color, eg. if we were to change the foreground color we would grab the background color
		result 		= foreground ? (otherColor << 4) | (color & 0x0F) : (color << 4) | (otherColor & 0x0F); // Calculate the new color, this is, the otherColor and the new color.
		
		_vd_address[index + i + 1] = result;
	}
}

void vd_printString(char *string, vd_color_t color)
{
	while(*string != '\0')
	{
		if(*string == '\n')
		{
			_vd_cursorX = 0;
			_vd_cursorY ++;

			if(_vd_cursorY >= _vd_height)
				__vd_scrollLine();
			
			string ++;
			continue;
		}

		__vd_setCharacter(_vd_cursorX, _vd_cursorY, *string);
		__vd_setColor(_vd_cursorX, _vd_cursorY, true, color);

		_vd_cursorX ++;
		if(_vd_cursorX >= _vd_width)
		{
			_vd_cursorX = 0;
			_vd_cursorY ++;

			if(_vd_cursorY >= _vd_height)
				__vd_scrollLine();
		}

		string ++;	
	}

	__vd_setCursor(_vd_cursorX, _vd_cursorY);
}


void _vd_init()
{
	vd_clear();
}
