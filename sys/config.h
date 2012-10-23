//
//  config.h
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define CONF_RELEASE

#ifndef CONF_RELEASE
	//#define CONF_RUNKUNIT // runs kunit unit tests
	#define CONF_KUNITFAILSONLY // displays only the failed test cases. Needs CONF_RUNKUNIT
	#define CONF_KUNITEXIT // doesn't run anything after kunit ran. Needs CONF_RUNKUNIT

	// REMARK: disabling the inlining gives only a hint to the compiler to disable it!
	// REMARK: functions are inlined for a reason, disabling it WILL have a performance impact!
	#define CONF_VM_NOINLINE // forces virtual memory functions not to be inlined, should be defined when debugging of virtual memory bugs for cleaner stacktraces
#endif /* CONF_RELEASE */
#endif /* _CONFIG_H_ */
