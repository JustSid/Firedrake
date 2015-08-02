//
//  kmod.h
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

#ifndef _KMOD_H_
#define _KMOD_H_

#include "libc/sys/cdefs.h"

__BEGIN_DECLS

typedef void *kmod_t;

#ifdef __cplusplus

#define kern_start_stub(function) \
		extern "C" { \
			bool _kern_start(kmod_t *mod) \
			{ \
				return function(mod); \
			} \
		}

	#define kern_stop_stub(function) \
		extern "C" { \
			void _kern_stop(kmod_t *mod) \
			{ \
				function(mod); \
			} \
		}

#else

#define kern_start_stub(function) \
		bool _kern_start(kmod_t *mod) \
		{ \
			return function(mod); \
		}

#define kern_stop_stub(function) \
		void _kern_stop(kmod_t *mod) \
		{ \
			function(mod); \
		}

#endif /* __cplusplus */

__END_DECLS

#endif /* _KMOD_H_ */
