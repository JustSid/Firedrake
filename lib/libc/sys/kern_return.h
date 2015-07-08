//
//  kern_return.h
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

#ifndef _LIBC_KERNRETURN_H_
#define _LIBC_KERNRETURN_H_

#define KERN_SUCCESS 0
#define KERN_INVALID_ADDRESS    1
#define KERN_INVALID_ARGUMENT   2
#define KERN_NO_MEMORY          3
#define KERN_FAILURE            4
#define KERN_RESOURCES_MISSING  5
#define KERN_RESOURCE_IN_USE    6
#define KERN_RESOURCE_EXISTS    7
#define KERN_RESOURCE_EXHAUSTED 8
#define KERN_TIMEOUT            9
#define KERN_ACCESS_VIOLATION   10
#define KERN_IPC_NO_RECEIVER    11
#define KERN_IPC_NO_SENDER      12
#define KERN_TASK_RESTART       1024 // Only used internally

#endif /* _LIBC_KERNRETURN_H_ */
