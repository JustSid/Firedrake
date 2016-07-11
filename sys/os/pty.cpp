//
//  pty.cpp
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

#include <libc/stdio.h>
#include <libcpp/algorithm.h>
#include "pty.h"

namespace OS
{
	IODefineMeta(PTY, IO::Object)
	IODefineMeta(PTYDevice, IO::Object)

	PTYDevice *PTYDevice::Init(PTY *pty, bool master, uint32_t id)
	{
		if(!IO::Object::Init())
			return nullptr;

		char name[64];
		sprintf(name, "%cty%03d", master ? 'p' : 't', id);

		_pty = pty;
		_master = master;
		_other = nullptr;
		_putProc = nullptr;
		_node = VFS::GetDevFS()->CreateNode(name, this, IOMemberFunctionCast(CFS::Node::ReadProc, this, &PTYDevice::Read), IOMemberFunctionCast(CFS::Node::WriteProc , this, &PTYDevice::Write));
		_node->SetSizeProc(IOMemberFunctionCast(CFS::Node::SizeProc, this, &PTYDevice::GetSize));
		_node->SetIoctlProc(IOMemberFunctionCast(CFS::Node::IoctlProc, this, &PTYDevice::Ioctl));
		_node->SetOpenProc(IOMemberFunctionCast(CFS::Node::OpenCloseProc, this, &PTYDevice::Open));

		return this;
	}

	void PTYDevice::Connect(PTYDevice *other)
	{
		_other = other;
	}

	size_t PTYDevice::GetSize()
	{
		spinlock_lock(&_pty->_lock);
		size_t size = _data.size();
		spinlock_unlock(&_pty->_lock);

		return size;
	}

	void PTYDevice::SetPutcProc(PutcProc proc)
	{
		_putProc = proc;
	}

	void PTYDevice::Open()
	{
		_data.clear();
	}

	void PTYDevice::Putc(char character)
	{
		if(__expect_false(_putProc))
		{
			_putProc(character);
			return;
		}

		_data.push(character);
	}

	KernReturn<void> PTYDevice::Ioctl(VFS::Context *context, uint32_t request, void *arg)
	{
		return _pty->Ioctl(_master, context, request, arg);
	}

	void PTYDevice::ParseCanonicalInput(char data)
	{
		if(data == _pty->_termios.c_cc[VKILL])
		{
			while(_pty->_canonicalBufferSize)
			{
				_pty->_canonicalBufferSize --;
				_pty->_canonicalBuffer[_pty->_canonicalBufferSize] = '\0';

				if(_pty->_termios.c_lflag & ECHO)
				{
					_other->Putc('\010');
					_other->Putc(' ');
					_other->Putc('\010');
				}
			}

			return;
		}

		if(data == _pty->_termios.c_cc[VERASE])
		{
			if(_pty->_canonicalBufferSize)
			{
				_pty->_canonicalBufferSize --;
				_pty->_canonicalBuffer[_pty->_canonicalBufferSize] = '\0';

				if(_pty->_termios.c_lflag & ECHO)
				{
					_other->Putc('\010');
					_other->Putc(' ');
					_other->Putc('\010');
				}
			}

			return;
		}

		if(data == _pty->_termios.c_cc[VEOF])
		{
			if(_pty->_canonicalBufferSize)
				_pty->FlushCanonicalBuffer();

			return;
		}



		_pty->_canonicalBuffer[_pty->_canonicalBufferSize] = data;

		if(_pty->_termios.c_lflag & ECHO)
			Putc(data);

		if(data == '\n')
		{
			_pty->_canonicalBufferSize ++;
			_pty->FlushCanonicalBuffer();

			return;
		}

		if(_pty->_canonicalBufferSize == sizeof(_pty->_canonicalBuffer))
		{
			_pty->FlushCanonicalBuffer();
			return;
		}

		_pty->_canonicalBufferSize ++;
	}

	void PTYDevice::ParseInput(char data)
	{
		if(data == '\n' && _pty->_termios.c_oflag & ONLCR)
			_other->Putc('\r');

		_other->Putc(data);
	}

	size_t PTYDevice::Write(VFS::Context *context, off_t offset, const void *data, size_t size)
	{
		spinlock_lock(&_pty->_lock);

		char buffer[kPTYMaxBufferSize];
		size_t write = std::min(size, sizeof(buffer));

		offset = size - write;
		context->CopyDataOut(reinterpret_cast<const uint8_t *>(data) + offset, buffer, write);


		if(_master)
		{
			for(size_t i = 0; i < write; i ++)
			{
				char data = buffer[i];

				if(_pty->_termios.c_lflag & ICANON)
				{
					ParseCanonicalInput(data);
					continue;
				}
				else if(_pty->_termios.c_lflag & ECHO)
					Putc(data);

				_other->Putc(data);
			}
		}
		else
		{
			for(size_t i = 0; i < write; i ++)
			{
				char data = buffer[i];
				ParseInput(data);
			}
		}

		spinlock_unlock(&_pty->_lock);

		return size;
	}

	size_t PTYDevice::Read(VFS::Context *context, __unused off_t offset, void *data, size_t size)
	{
		spinlock_lock(&_pty->_lock);

		size_t read = std::min(size, _data.size());
		if(read == 0)
		{
			spinlock_unlock(&_pty->_lock);
			return read;
		}

		char buffer[kPTYMaxBufferSize];

		for(size_t i = 0; i < read; i ++)
			buffer[i] = _data.pop();

		context->CopyDataIn(buffer, data, read);

		// TODO: Support VMIN for non ICANON TTYs

		spinlock_unlock(&_pty->_lock);

		return read;
	}


	static std::atomic<uint32_t> _ptyIDs = 0;

	PTY *PTY::Init()
	{
		if(!IO::Object::Init())
			return nullptr;

		_id = (++ _ptyIDs);

		_master = PTYDevice::Alloc()->Init(this, true, _id);
		_slave = PTYDevice::Alloc()->Init(this, false, _id);

		_master->Connect(_slave);
		_slave->Connect(_master);

		_winsize.ws_col = 80;
		_winsize.ws_row = 25;

		_termios.c_iflag = ICRNL | BRKINT;
		_termios.c_oflag = ONLCR | OPOST;
		_termios.c_lflag = ECHO | ECHOE | ECHOK | ICANON | ISIG | IEXTEN;
		_termios.c_cflag = CREAD;
		_termios.c_cc[VEOF] = 4; /* ^D */
		_termios.c_cc[VEOL] = 0; /* Not set */
		_termios.c_cc[VERASE] = '\b';
		_termios.c_cc[VINTR] = 3; /* ^C */
		_termios.c_cc[VKILL] = 21; /* ^U */
		_termios.c_cc[VMIN] = 1;
		_termios.c_cc[VQUIT] = 28; /* ^\ */
		_termios.c_cc[VSTART] = 17; /* ^Q */
		_termios.c_cc[VSTOP] = 19; /* ^S */
		_termios.c_cc[VSUSP] = 26; /* ^Z */
		_termios.c_cc[VTIME] = 0;

		return this;
	}

	PTYDevice *PTY::GetMasterDevice() const
	{
		return _master;
	}

	PTYDevice *PTY::GetSlaveDevice() const
	{
		return _slave;
	}

	KernReturn<void> PTY::Ioctl(bool fromMaster, VFS::Context *context, uint32_t request, void *arg)
	{
		switch(request)
		{
			case TIOCSWINSZ:
				if(!arg || !fromMaster)
					return Error(KERN_INVALID_ARGUMENT);

				context->CopyDataOut(arg, &_winsize, sizeof(winsize));
				return ErrorNone;

			case TIOCGWINSZ:
				if(!arg)
					return Error(KERN_INVALID_ARGUMENT);

				context->CopyDataIn(&_winsize, arg, sizeof(winsize));
				return ErrorNone;

			case TCGETS:
				if(!arg)
					return Error(KERN_INVALID_ARGUMENT);

				context->CopyDataIn(&_termios, arg, sizeof(termios));
				return ErrorNone;

			case TCSETS:
				if(!arg)
					return Error(KERN_INVALID_ARGUMENT);

				context->CopyDataOut(arg, &_termios, sizeof(termios));
				return ErrorNone;

			case TCSETSW:
			case TCSETSF:
			{
				if(!arg)
					return Error(KERN_INVALID_ARGUMENT);

				bool wasCanon = (_termios.c_lflag & ICANON);

				context->CopyDataOut(arg, &_termios, sizeof(termios));

				if(wasCanon && !(_termios.c_cflag & ICANON))
					FlushCanonicalBuffer();

				return ErrorNone;
			}
		}

		return Error(KERN_INVALID_ARGUMENT);
	}

	void PTY::FlushCanonicalBuffer()
	{
		for(size_t i = 0; i < _canonicalBufferSize; i ++)
			_slave->Putc(_canonicalBuffer[i]);

		_canonicalBufferSize = 0;
	}
}
