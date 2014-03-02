//
//  textdevice.h
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

#include "video.h"

#ifndef _TEXTDEVICE_H_
#define _TEXTDEVICE_H_

namespace vd
{
	class text_device : public video_device
	{
	public:
		text_device();

		bool is_text() override { return true; }

		size_t get_width() override { return _width; }
		size_t get_height() override { return _height; }
		size_t get_depth() override { return 0; }

		void set_colors(color foreground, color background);
		void set_cursor(size_t x, size_t y);

		void write_string(const char *string);
		void scroll_lines(size_t lines);
		void clear();

	private:
		void putc(char character);
		void color_point(size_t x, size_t y, color foreground, color background);

		uint8_t *_base;
		size_t _width;
		size_t _height;
	};
}

#endif /* _TEXTDEVICE_H_ */
