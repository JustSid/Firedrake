//
//  spinlock.h
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

/*
 * Overview:
 * Implementation of a simple spinlock.
 */
#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <types.h>

typedef struct
{
	int8_t locked; // 1 if the lock is locked, otherwise 0
} spinlock_t;

#define SPINLOCK_INITIALIZER 			{.locked = 0} // Initializer for an unlocked spinlock
#define SPINLOCK_INITIALIZER_LOCKED 	{.locked = 1} // Initializer for a locked spinlock

void spinlock_init(spinlock_t *spinlock);
void spinlock_lock(spinlock_t *spinlock);
void spinlock_unlock(spinlock_t *spinlock);

#endif /* _SPINLOCK_H_ */
