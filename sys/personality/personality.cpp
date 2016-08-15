//
//  personality.cpp
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

#include <prefix.h>
#include <libcpp/new.h>

#include "personality.h"
#include CONFIG_PERSONALITY_HEADER

const char *kVersionBeast    = "Nidhogg";
const char *kVersionAppendix = "";

extern void cxa_init();

namespace Sys
{
	static uint8_t _personalitySpace[sizeof(PERSONALITY_CLASS)];
	static Personality *_personality = nullptr;

	Personality::Personality()
	{}
	Personality::~Personality()
	{}

	
	Personality *Personality::GetPersonality()
	{
		return _personality;
	}

	void Personality::Bootstrap()
	{
		// Get the C++ runtime going
		cxa_init();

		// Create the kernel personality and let that take care of the rest of the bootstrapping
		_personality = new(_personalitySpace) PERSONALITY_CLASS;

		if(_personality)
			_personality->InitSystem();
	}

	void Personality::PrintHeader() const
	{
		kprintf("Firedrake v%i.%i.%i:%i (%s, '%s' personality)\n", kVersionMajor, kVersionMinor, kVersionPatch, VersionCurrent(), kVersionBeast, GetName());
		kprintf("Here be dragons\n\n");
	}
}
