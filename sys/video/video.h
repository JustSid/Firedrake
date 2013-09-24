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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <libc/stdint.h>
#include <libc/stddef.h>

namespace vd
{
	class video_device
	{
	public:
		enum class color : char
		{
			black = 0x0,
			white = 0xf,
			blue = 0x1,
			green,
			cyan,
			red,
			magenta,
			brown,
			light_gray,
			dark_ray,
			light_blue,
			light_green,
			light_cyan,
			light_red,
			light_magenta,
			yellow
		};

		video_device();
		virtual ~video_device();

		virtual bool is_text() = 0;

		virtual size_t get_width()  = 0;
		virtual size_t get_height() = 0;
		virtual size_t get_depth() = 0;

		color get_background_color() { return backgroundColor; }
		color get_foreground_color() { return foregroundColor; }
		size_t get_cursor_x() { return cursorX; }
		size_t get_cursor_y() { return cursorY; }

		virtual void set_colors(color foreground, color background) = 0;
		virtual void set_cursor(size_t x, size_t y) = 0;

		virtual void write_string(const char *string) = 0;
		virtual void scroll_lines(size_t numLines) = 0;
		virtual void clear() = 0;

	protected:
		color backgroundColor;
		color foregroundColor;

		size_t cursorX;
		size_t cursorY;
	};

	void init();
	video_device *get_active_device();
}

#endif /* _VIDEO_H_ */
