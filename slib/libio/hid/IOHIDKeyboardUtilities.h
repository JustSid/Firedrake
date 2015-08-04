//
//  IOHIDKeyboardUtilities.h
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

#ifndef _IOHIDKEYBOARDUTILITIES_H_
#define _IOHIDKEYBOARDUTILITIES_H_

#include <libc/stdint.h>
#include <libc/string.h>

namespace IO
{
	enum KeyCode
	{
		KeyCodeESC = 1,
		KeyCodeBasckspace = 2,
		KeyCodeDelete = 3,
		KeyCodeEnter = '\n',
		KeyCodeTab = '\t',
		KeyCodeSpace = ' ',

		KeyCodeArrowUp = 180,
		KeyCodeArrowDown = 181,
		KeyCodeArrowLeft = 182,
		KeyCodeArrowRight = 183,

		KeyCodeRightShift = 20,
		KeyCodeLeftShift = 21,
		KeyCodeRightControl = 22,
		KeyCodeLeftControl = 23,
		KeyCodeRightOption = 24,
		KeyCodeLeftOption = 25,
		KeyCodeRightGUI = 26,
		KeyCodeLeftGUI = 27,

		KeyCodeF1 = 30,
		KeyCodeF2 = 31,
		KeyCodeF3 = 32,
		KeyCodeF4 = 33,
		KeyCodeF5 = 34,
		KeyCodeF6 = 35,
		KeyCodeF7 = 36,
		KeyCodeF8 = 37,
		KeyCodeF9 = 38,
		KeyCodeF10 = 39,
		KeyCodeF11 = 40,
		KeyCodeF12 = 41,

		KeyCode0 = 48,
		KeyCode1 = 49,
		KeyCode2 = 50,
		KeyCode3 = 51,
		KeyCode4 = 52,
		KeyCode5 = 53,
		KeyCode6 = 54,
		KeyCode7 = 55,
		KeyCode8 = 56,
		KeyCode9 = 57,

		KeyCodeA = 97,
		KeyCodeB = 98,
		KeyCodeC = 99,
		KeyCodeD = 100,
		KeyCodeE = 101,
		KeyCodeF = 102,
		KeyCodeG = 103,
		KeyCodeH = 104,
		KeyCodeI = 105,
		KeyCodeJ = 106,
		KeyCodeK = 107,
		KeyCodeL = 108,
		KeyCodeM = 109,
		KeyCodeN = 110,
		KeyCodeO = 111,
		KeyCodeP = 112,
		KeyCodeQ = 113,
		KeyCodeR = 114,
		KeyCodeS = 115,
		KeyCodeT = 116,
		KeyCodeU = 117,
		KeyCodeV = 118,
		KeyCodeW = 119,
		KeyCodeX = 120,
		KeyCodeY = 121,
		KeyCodeZ = 122,

		KeyCodeComma = 150,
		KeyCodeDot = 151,
		KeyCodeSlash = 152
	};

	struct KeyboardEvent
	{
	public:
		KeyboardEvent(uint32_t keyCode, uint32_t modifier, bool keyDown) :
			_keyCode(keyCode),
			_modifier(modifier),
			_keyDown(keyDown)
		{}
		KeyboardEvent(const KeyboardEvent &other) :
			_keyCode(other._keyCode),
			_modifier(other._modifier),
			_keyDown(other._keyDown)
		{}
		KeyboardEvent &operator= (const KeyboardEvent &other)
		{
			_keyCode = other._keyCode;
			_modifier = other._modifier;
			_keyDown = other._keyDown;

			return *this;
		}

		uint32_t GetKeyCode() const { return _keyCode; }
		uint32_t GetModifier() const { return _modifier; }
		bool IsKeyDown() const { return _keyDown; }

		char GetCharacter() const // This should be handled somewhere in the GUI...
		{
			if(_keyCode >= KeyCodeA && _keyCode <= KeyCodeZ)
			{
				char character = 'a' + (_keyCode - KeyCodeA);

				if(_modifier & (1 << KeyCodeLeftShift) || _modifier & (1 << KeyCodeRightShift))
					character = toupper(character);

				return character;
			}

			if(_keyCode >= KeyCode0 && _keyCode <= KeyCode9)
				return '0' + (_keyCode - KeyCode0);

			switch(_keyCode)
			{
				case KeyCodeSpace:
					return ' ';
				case KeyCodeTab:
					return '\t';
				case KeyCodeEnter:
					return '\n';
				case KeyCodeComma:
					return ',';
				case KeyCodeDot:
					return '.';
				case KeyCodeSlash:
					return '/';

				default:
					return 0x0;
			}
		}

	private:
		uint32_t _keyCode;
		uint32_t _modifier;
		bool _keyDown;
	};
}

#endif /* _IOHIDKEYBOARDUTILITIES_H_ */
