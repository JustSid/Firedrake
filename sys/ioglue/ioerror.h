//
//  ioerror.h
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

#ifndef _IOERROR_H_
#define _IOERROR_H_

#include <types.h>

typedef uint32_t IOReturn;

#define kIOReturnSuccess 			0 // Everything is just fine, move on
#define kIOReturnError				1 // A general error
#define kIOReturnNoMemory 			2 // We are out of memory
#define kIOReturnInvalidArgument 	3 // One or more argument passed was invalid
#define kIOReturnAlignmentError 	4 // Passed data wasn't aligned correctly
#define kIOReturnTimeout 			5 // The request timed out
#define kIOReturnNoInterrupt 		6 // The specified interrupt isn't valid
#define kIOReturnNotFound 			7 // The requested resource wasn't found
#define kIOReturnUnsupported 		8 // The requested operation isn't supported
#define kIOReturnNoExclusiveAccess	9 // A request for an exclusive resource couldn't be fulfilled

#endif /* _IOERROR_H_ */
