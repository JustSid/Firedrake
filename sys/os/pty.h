//
//  pty.h
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

#ifndef _PTY_H_
#define _PTY_H_

#include <prefix.h>
#include <libio/core/IOObject.h>
#include <libc/sys/termios.h>
#include <libc/sys/ioctl.h>
#include <libcpp/ringbuffer.h>
#include <vfs/vfs.h>
#include <vfs/cfs/cfs_node.h>

#define kPTYMaxBufferSize 4096

namespace OS
{
	class PTY;
	class PTYDevice : public IO::Object
	{
	public:
		friend class PTY;

		typedef void (*PutcProc)(char character);

		void SetPutcProc(PutcProc proc);

	private:
		PTYDevice *Init(PTY *pty, bool master, uint32_t id);
		void Connect(PTYDevice *other);

		size_t Write(VFS::Context *context, off_t offset, const void *data, size_t size);
		size_t Read(VFS::Context *context, off_t offset, void *data, size_t size);
		size_t GetSize();

		void Open();

		KernReturn<void> Ioctl(VFS::Context *context, uint32_t request, void *arg);

		void ParseCanonicalInput(char data);
		void ParseInput(char data);

		void Putc(char character);

		PTY *_pty;

		bool _master;
		PTYDevice *_other;
		CFS::Node *_node;

		PutcProc _putProc;
		std::ringbuffer<char, kPTYMaxBufferSize> _data;

		IODeclareMeta(PTYDevice)
	};

	class PTY : public IO::Object
	{
	public:
		friend class PTYDevice;

		PTY *Init();

		PTYDevice *GetMasterDevice() const;
		PTYDevice *GetSlaveDevice() const;

	private:
		KernReturn<void> Ioctl(bool fromMaster, VFS::Context *context, uint32_t request, void *arg);
		void FlushCanonicalBuffer();

		spinlock_t _lock;

		uint32_t _id;

		PTYDevice *_master;
		PTYDevice *_slave;

		termios _termios;
		winsize _winsize;

		char _canonicalBuffer[1024];
		size_t _canonicalBufferSize;

		IODeclareMeta(PTY)
	};
}

#endif /* _PTY_H_ */
