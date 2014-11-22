//
//  personality.h
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

#ifndef _PERSONALITY_H_
#define _PERSONALITY_H_

#include <prefix.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <kern/kern_return.h>
#include <libcpp/type_traits.h>

namespace Sys
{
	class Personality
	{
	public:
		virtual ~Personality();

		static void Bootstrap(); // Called once on the bootstrap CPU
		virtual void FinishBootstrapping() = 0; // Called once from the kernel task on any CPU

		static Personality *GetPersonality();
		virtual const char *GetName() const = 0;

	protected:
		Personality();
		virtual void InitSystem() = 0;

		// Prints the default Firedrake header via kprintf
		void PrintHeader() const;

		template<class F, class ...Args>
		void Init(const char *name, F &&f, Args &&... args) const
		{
			kprintf("Initializing %s... { ", name);

			auto result = f(std::forward<Args>(args)...);

			if(!result.IsValid())
			{
				kprintf("} failed\n");
				panic("Failed to initialize %s", name); 
			}
			else
			{
				kprintf(" } ok\n");
			}
		}
	};
}

#endif /* _PERSONALITY_H_ */
