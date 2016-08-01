//
//  syscallTable.cpp
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

#include <kern/kprintf.h>
#include "syscall.h"

#include <vfs/vfs_syscall.h>
#include <os/scheduler/scheduler_syscall.h>
#include "syscall_mmap.h"

namespace OS
{
	#define SYSCALL_TRAP(name, handler, argCount, argSize) \
		{ name, (KernReturn<uint32_t> (*)(Thread *, void *))handler, (uint32_t)argCount, argSize }

	#define SYSCALL_ARGENTRY(struct, entry) \
		{ offsetof(struct, entry), sizeof(struct::entry) }

	#define SYSCALL_ARGENTRIES(...) \
		{ __VA_ARGS__ }

	#define SYSCALL_TRAP0(name, handler) \
		SYSCALL_TRAP(name, handler, 0, SYSCALL_ARGENTRIES())

	#define SYSCALL_TRAP1(name, handler, st, arg0) \
		SYSCALL_TRAP(name, handler, 1, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0)))

	#define SYSCALL_TRAP2(name, handler, st, arg0, arg1) \
		SYSCALL_TRAP(name, handler, 2, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1)))

	#define SYSCALL_TRAP3(name, handler, st, arg0, arg1, arg2) \
		SYSCALL_TRAP(name, handler, 3, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2)))

	#define SYSCALL_TRAP4(name, handler, st, arg0, arg1, arg2, arg3) \
		SYSCALL_TRAP(name, handler, 4, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2), SYSCALL_ARGENTRY(st, arg3)))

	#define SYSCALL_TRAP5(name, handler, st, arg0, arg1, arg2, arg3, arg4) \
		SYSCALL_TRAP(name, handler, 5, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2), SYSCALL_ARGENTRY(st, arg3), SYSCALL_ARGENTRY(st, arg4)))

	#define SYSCALL_TRAP6(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5) \
		SYSCALL_TRAP(name, handler, 6, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2), SYSCALL_ARGENTRY(st, arg3), SYSCALL_ARGENTRY(st, arg4), SYSCALL_ARGENTRY(st, arg5)))

	#define SYSCALL_TRAP7(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
		SYSCALL_TRAP(name, handler, 7, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2), SYSCALL_ARGENTRY(st, arg3), SYSCALL_ARGENTRY(st, arg4), SYSCALL_ARGENTRY(st, arg5), SYSCALL_ARGENTRY(st, arg6)))

	#define SYSCALL_TRAP8(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
		SYSCALL_TRAP(name, handler, 8, SYSCALL_ARGENTRIES(SYSCALL_ARGENTRY(st, arg0), SYSCALL_ARGENTRY(st, arg1), SYSCALL_ARGENTRY(st, arg2), SYSCALL_ARGENTRY(st, arg3), SYSCALL_ARGENTRY(st, arg4), SYSCALL_ARGENTRY(st, arg5), SYSCALL_ARGENTRY(st, arg6), SYSCALL_ARGENTRY(st, arg7)))
		
	#define SYSCALL_TRAP_INVALID() SYSCALL_TRAP("invalid", &SyscallInvalid, 0, SYSCALL_ARGENTRIES())

	KernReturn<uint32_t> SyscallInvalid(__unused Thread *thread, __unused void *args)
	{
		kprintf("Invalid syscall trap\n");
		return 0;
	}

	SyscallTrap _syscallTrapTable[128] = {
		/* 0 */ SYSCALL_TRAP1("exit", &OS::Syscall_SchedThreadExit, SchedThreadExitArgs, exitCode),
		/* 1 */ SYSCALL_TRAP2("open", &VFS::Syscall_VFSOpen, VFS::VFSOpenArgs, path, flags),
		/* 2 */ SYSCALL_TRAP1("close", &VFS::Syscall_VFSClose, VFS::VFSCloseArgs, fd),
		/* 3 */ SYSCALL_TRAP3("read", &VFS::Syscall_VFSRead, VFS::VFSReadArgs, fd, data, size),
		/* 4 */ SYSCALL_TRAP3("write", &VFS::Syscall_VFSWrite, VFS::VFSWriteArgs, fd, data, size),
		/* 5 */ SYSCALL_TRAP3("seek", &VFS::Syscall_VFSSeek, VFS::VFSSeekArgs, fd, offset, whence),
		/* 6 */ SYSCALL_TRAP3("ioctl", &VFS::Syscall_VFSIoctl, VFS::VFSIoctlArgs, fd, request, arg),
		/* 7 */ SYSCALL_TRAP_INVALID(),
		/* 8 */ SYSCALL_TRAP_INVALID(),
		/* 9 */ SYSCALL_TRAP_INVALID(),
		/* 10 */ SYSCALL_TRAP4("thread_create", &OS::Syscall_SchedThreadCreate, OS::SchedThreadCreateArgs, entry, argument1, argument2, stack),
		/* 11 */ SYSCALL_TRAP1("thread_join", &OS::Syscall_SchedThreadJoin, OS::SchedThreadJoinArgs, tid),
		/* 12 */ SYSCALL_TRAP0("thread_yield", &OS::Syscall_SchedThreadYield),
		/* 13 */ SYSCALL_TRAP_INVALID(),
		/* 14 */ SYSCALL_TRAP_INVALID(),
		/* 15 */ SYSCALL_TRAP_INVALID(),
		/* 16 */ SYSCALL_TRAP_INVALID(),
		/* 17 */ SYSCALL_TRAP_INVALID(),
		/* 18 */ SYSCALL_TRAP_INVALID(),
		/* 19 */ SYSCALL_TRAP_INVALID(),
		/* 20 */ SYSCALL_TRAP6("mmap", &OS::Syscall_mmap, OS::MmapArgs, address, length, protection, flags, fd, offset),
		/* 21 */ SYSCALL_TRAP_INVALID(),
		/* 22 */ SYSCALL_TRAP_INVALID(),
		/* 23 */ SYSCALL_TRAP3("msync", &OS::Syscall_msync, OS::MsyncArgs, address, length, flags),
		/* 24 */ SYSCALL_TRAP_INVALID(),
		/* 25 */ SYSCALL_TRAP_INVALID(),
		/* 26 */ SYSCALL_TRAP_INVALID(),
		/* 27 */ SYSCALL_TRAP_INVALID(),
		/* 28 */ SYSCALL_TRAP_INVALID(),
		/* 29 */ SYSCALL_TRAP_INVALID(),
		/* 30 */ SYSCALL_TRAP_INVALID(),
		/* 31 */ SYSCALL_TRAP_INVALID(),
		/* 32 */ SYSCALL_TRAP_INVALID(),
		/* 33 */ SYSCALL_TRAP_INVALID(),
		/* 34 */ SYSCALL_TRAP_INVALID(),
		/* 35 */ SYSCALL_TRAP_INVALID(),
		/* 36 */ SYSCALL_TRAP_INVALID(),
		/* 37 */ SYSCALL_TRAP_INVALID(),
		/* 38 */ SYSCALL_TRAP_INVALID(),
		/* 39 */ SYSCALL_TRAP_INVALID(),
		/* 10 */ SYSCALL_TRAP_INVALID(),
		/* 41 */ SYSCALL_TRAP_INVALID(),
		/* 42 */ SYSCALL_TRAP_INVALID(),
		/* 43 */ SYSCALL_TRAP_INVALID(),
		/* 44 */ SYSCALL_TRAP_INVALID(),
		/* 45 */ SYSCALL_TRAP_INVALID(),
		/* 46 */ SYSCALL_TRAP_INVALID(),
		/* 47 */ SYSCALL_TRAP_INVALID(),
		/* 48 */ SYSCALL_TRAP_INVALID(),
		/* 49 */ SYSCALL_TRAP_INVALID(),
		/* 50 */ SYSCALL_TRAP_INVALID(),
		/* 51 */ SYSCALL_TRAP_INVALID(),
		/* 52 */ SYSCALL_TRAP_INVALID(),
		/* 53 */ SYSCALL_TRAP_INVALID(),
		/* 54 */ SYSCALL_TRAP_INVALID(),
		/* 55 */ SYSCALL_TRAP_INVALID(),
		/* 56 */ SYSCALL_TRAP_INVALID(),
		/* 57 */ SYSCALL_TRAP_INVALID(),
		/* 58 */ SYSCALL_TRAP_INVALID(),
		/* 59 */ SYSCALL_TRAP_INVALID(),
		/* 60 */ SYSCALL_TRAP_INVALID(),
		/* 61 */ SYSCALL_TRAP_INVALID(),
		/* 62 */ SYSCALL_TRAP_INVALID(),
		/* 63 */ SYSCALL_TRAP_INVALID(),
		/* 64 */ SYSCALL_TRAP_INVALID(),
		/* 65 */ SYSCALL_TRAP_INVALID(),
		/* 66 */ SYSCALL_TRAP_INVALID(),
		/* 67 */ SYSCALL_TRAP_INVALID(),
		/* 68 */ SYSCALL_TRAP_INVALID(),
		/* 69 */ SYSCALL_TRAP_INVALID(),
		/* 70 */ SYSCALL_TRAP_INVALID(),
		/* 71 */ SYSCALL_TRAP_INVALID(),
		/* 72 */ SYSCALL_TRAP_INVALID(),
		/* 73 */ SYSCALL_TRAP_INVALID(),
		/* 74 */ SYSCALL_TRAP_INVALID(),
		/* 75 */ SYSCALL_TRAP_INVALID(),
		/* 76 */ SYSCALL_TRAP_INVALID(),
		/* 77 */ SYSCALL_TRAP_INVALID(),
		/* 78 */ SYSCALL_TRAP_INVALID(),
		/* 79 */ SYSCALL_TRAP_INVALID(),
		/* 80 */ SYSCALL_TRAP_INVALID(),
		/* 81 */ SYSCALL_TRAP_INVALID(),
		/* 82 */ SYSCALL_TRAP_INVALID(),
		/* 83 */ SYSCALL_TRAP_INVALID(),
		/* 84 */ SYSCALL_TRAP_INVALID(),
		/* 85 */ SYSCALL_TRAP_INVALID(),
		/* 86 */ SYSCALL_TRAP_INVALID(),
		/* 87 */ SYSCALL_TRAP_INVALID(),
		/* 88 */ SYSCALL_TRAP_INVALID(),
		/* 89 */ SYSCALL_TRAP_INVALID(),
		/* 90 */ SYSCALL_TRAP_INVALID(),
		/* 91 */ SYSCALL_TRAP_INVALID(),
		/* 92 */ SYSCALL_TRAP_INVALID(),
		/* 93 */ SYSCALL_TRAP_INVALID(),
		/* 94 */ SYSCALL_TRAP_INVALID(),
		/* 95 */ SYSCALL_TRAP_INVALID(),
		/* 96 */ SYSCALL_TRAP_INVALID(),
		/* 97 */ SYSCALL_TRAP_INVALID(),
		/* 98 */ SYSCALL_TRAP_INVALID(),
		/* 99 */ SYSCALL_TRAP_INVALID(),
		/* 100 */ SYSCALL_TRAP_INVALID(),
		/* 101 */ SYSCALL_TRAP_INVALID(),
		/* 102 */ SYSCALL_TRAP_INVALID(),
		/* 103 */ SYSCALL_TRAP_INVALID(),
		/* 104 */ SYSCALL_TRAP_INVALID(),
		/* 105 */ SYSCALL_TRAP_INVALID(),
		/* 106 */ SYSCALL_TRAP_INVALID(),
		/* 107 */ SYSCALL_TRAP_INVALID(),
		/* 108 */ SYSCALL_TRAP_INVALID(),
		/* 109 */ SYSCALL_TRAP_INVALID(),
		/* 110 */ SYSCALL_TRAP_INVALID(),
		/* 111 */ SYSCALL_TRAP_INVALID(),
		/* 112 */ SYSCALL_TRAP_INVALID(),
		/* 113 */ SYSCALL_TRAP_INVALID(),
		/* 114 */ SYSCALL_TRAP_INVALID(),
		/* 115 */ SYSCALL_TRAP_INVALID(),
		/* 116 */ SYSCALL_TRAP_INVALID(),
		/* 117 */ SYSCALL_TRAP_INVALID(),
		/* 118 */ SYSCALL_TRAP_INVALID(),
		/* 119 */ SYSCALL_TRAP_INVALID(),
		/* 120 */ SYSCALL_TRAP_INVALID(),
		/* 121 */ SYSCALL_TRAP_INVALID(),
		/* 122 */ SYSCALL_TRAP_INVALID(),
		/* 123 */ SYSCALL_TRAP_INVALID(),
		/* 124 */ SYSCALL_TRAP_INVALID(),
		/* 125 */ SYSCALL_TRAP_INVALID(),
		/* 126 */ SYSCALL_TRAP_INVALID(),
		/* 127 */ SYSCALL_TRAP_INVALID()
	};
}
