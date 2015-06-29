//
//  kerntrapTable.cpp
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

#include <os/ipc/IPCSyscall.h>

namespace OS
{
	#define KERN_TRAP(name, handler, argCount, argSize) \
		{ (KernReturn<uint32_t> (*)(Thread *, void *))handler, (uint32_t)argCount, argSize }

	#define KERN_TRAP_ARGENTRY(str, entry) \
		{ offsetof(str, entry), sizeof(str::entry) }

	#define KERN_TRAP_ARGENTRIES(...) \
		{ __VA_ARGS__ }

	#define KERN_TRAP0(name, handler) \
		KERN_TRAP(name, handler, 0, KERN_TRAP_ARGENTRIES())

	#define KERN_TRAP1(name, handler, st, arg0) \
		KERN_TRAP(name, handler, 1, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0)))

	#define KERN_TRAP2(name, handler, st, arg0, arg1) \
		KERN_TRAP(name, handler, 2, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1)))

	#define KERN_TRAP3(name, handler, st, arg0, arg1, arg2) \
		KERN_TRAP(name, handler, 3, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2)))

	#define KERN_TRAP4(name, handler, st, arg0, arg1, arg2, arg3) \
		KERN_TRAP(name, handler, 4, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2), KERN_TRAP_ARGENTRY(st, arg3)))

	#define KERN_TRAP5(name, handler, st, arg0, arg1, arg2, arg3, arg4) \
		KERN_TRAP(name, handler, 5, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2), KERN_TRAP_ARGENTRY(st, arg3), KERN_TRAP_ARGENTRY(st, arg4)))

	#define KERN_TRAP6(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5) \
		KERN_TRAP(name, handler, 6, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2), KERN_TRAP_ARGENTRY(st, arg3), KERN_TRAP_ARGENTRY(st, arg4), KERN_TRAP_ARGENTRY(st, arg5)))

	#define KERN_TRAP7(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
		KERN_TRAP(name, handler, 7, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2), KERN_TRAP_ARGENTRY(st, arg3), KERN_TRAP_ARGENTRY(st, arg4), KERN_TRAP_ARGENTRY(st, arg5), KERN_TRAP_ARGENTRY(st, arg6)))

	#define KERN_TRAP8(name, handler, st, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
		KERN_TRAP(name, handler, 8, KERN_TRAP_ARGENTRIES(KERN_TRAP_ARGENTRY(st, arg0), KERN_TRAP_ARGENTRY(st, arg1), KERN_TRAP_ARGENTRY(st, arg2), KERN_TRAP_ARGENTRY(st, arg3), KERN_TRAP_ARGENTRY(st, arg4), KERN_TRAP_ARGENTRY(st, arg5), KERN_TRAP_ARGENTRY(st, arg6), KERN_TRAP_ARGENTRY(st, arg7)))
		
	#define KERN_TRAP_INVALID() KERN_TRAP("invalid", &KerntrapInvalid, 0, KERN_TRAP_ARGENTRIES())


	KernReturn<uint32_t> KerntrapInvalid(__unused Thread *thread, __unused void *args)
	{
		kprintf("Invalid kernel trap\n");
		return 0;
	}

	SyscallTrap _kernTrapTable[128] = {
		/* 0 */ KERN_TRAP1("ipc_task_port", &OS::IPC::Syscall_IPCTaskPort, IPC::IPCPortCallArgs, port),
		/* 1 */ KERN_TRAP1("ipc_thread_port", &OS::IPC::Syscall_IPCThreadPort, IPC::IPCPortCallArgs, port),
		/* 2 */ KERN_TRAP3("ipc_message", &OS::IPC::Syscall_IPCMessage, IPC::IPCReadWriteArgs, header, size, mode),
		/* 3 */ KERN_TRAP3("ipc_allocate_port", &OS::IPC::Syscall_IPCAllocatePort, IPC::IPCAllocatePortArgs, result, name, options),
		/* 4 */ KERN_TRAP_INVALID(),
		/* 5 */ KERN_TRAP_INVALID(),
		/* 6 */ KERN_TRAP_INVALID(),
		/* 7 */ KERN_TRAP_INVALID(),
		/* 8 */ KERN_TRAP_INVALID(),
		/* 9 */ KERN_TRAP_INVALID(),
		/* 10 */ KERN_TRAP_INVALID(),
		/* 11 */ KERN_TRAP_INVALID(),
		/* 12 */ KERN_TRAP_INVALID(),
		/* 13 */ KERN_TRAP_INVALID(),
		/* 14 */ KERN_TRAP_INVALID(),
		/* 15 */ KERN_TRAP_INVALID(),
		/* 16 */ KERN_TRAP_INVALID(),
		/* 17 */ KERN_TRAP_INVALID(),
		/* 18 */ KERN_TRAP_INVALID(),
		/* 19 */ KERN_TRAP_INVALID(),
		/* 20 */ KERN_TRAP_INVALID(),
		/* 21 */ KERN_TRAP_INVALID(),
		/* 22 */ KERN_TRAP_INVALID(),
		/* 23 */ KERN_TRAP_INVALID(),
		/* 24 */ KERN_TRAP_INVALID(),
		/* 25 */ KERN_TRAP_INVALID(),
		/* 26 */ KERN_TRAP_INVALID(),
		/* 27 */ KERN_TRAP_INVALID(),
		/* 28 */ KERN_TRAP_INVALID(),
		/* 29 */ KERN_TRAP_INVALID(),
		/* 30 */ KERN_TRAP_INVALID(),
		/* 31 */ KERN_TRAP_INVALID(),
		/* 32 */ KERN_TRAP_INVALID(),
		/* 33 */ KERN_TRAP_INVALID(),
		/* 34 */ KERN_TRAP_INVALID(),
		/* 35 */ KERN_TRAP_INVALID(),
		/* 36 */ KERN_TRAP_INVALID(),
		/* 37 */ KERN_TRAP_INVALID(),
		/* 38 */ KERN_TRAP_INVALID(),
		/* 39 */ KERN_TRAP_INVALID(),
		/* 10 */ KERN_TRAP_INVALID(),
		/* 41 */ KERN_TRAP_INVALID(),
		/* 42 */ KERN_TRAP_INVALID(),
		/* 43 */ KERN_TRAP_INVALID(),
		/* 44 */ KERN_TRAP_INVALID(),
		/* 45 */ KERN_TRAP_INVALID(),
		/* 46 */ KERN_TRAP_INVALID(),
		/* 47 */ KERN_TRAP_INVALID(),
		/* 48 */ KERN_TRAP_INVALID(),
		/* 49 */ KERN_TRAP_INVALID(),
		/* 50 */ KERN_TRAP_INVALID(),
		/* 51 */ KERN_TRAP_INVALID(),
		/* 52 */ KERN_TRAP_INVALID(),
		/* 53 */ KERN_TRAP_INVALID(),
		/* 54 */ KERN_TRAP_INVALID(),
		/* 55 */ KERN_TRAP_INVALID(),
		/* 56 */ KERN_TRAP_INVALID(),
		/* 57 */ KERN_TRAP_INVALID(),
		/* 58 */ KERN_TRAP_INVALID(),
		/* 59 */ KERN_TRAP_INVALID(),
		/* 60 */ KERN_TRAP_INVALID(),
		/* 61 */ KERN_TRAP_INVALID(),
		/* 62 */ KERN_TRAP_INVALID(),
		/* 63 */ KERN_TRAP_INVALID(),
		/* 64 */ KERN_TRAP_INVALID(),
		/* 65 */ KERN_TRAP_INVALID(),
		/* 66 */ KERN_TRAP_INVALID(),
		/* 67 */ KERN_TRAP_INVALID(),
		/* 68 */ KERN_TRAP_INVALID(),
		/* 69 */ KERN_TRAP_INVALID(),
		/* 70 */ KERN_TRAP_INVALID(),
		/* 71 */ KERN_TRAP_INVALID(),
		/* 72 */ KERN_TRAP_INVALID(),
		/* 73 */ KERN_TRAP_INVALID(),
		/* 74 */ KERN_TRAP_INVALID(),
		/* 75 */ KERN_TRAP_INVALID(),
		/* 76 */ KERN_TRAP_INVALID(),
		/* 77 */ KERN_TRAP_INVALID(),
		/* 78 */ KERN_TRAP_INVALID(),
		/* 79 */ KERN_TRAP_INVALID(),
		/* 80 */ KERN_TRAP_INVALID(),
		/* 81 */ KERN_TRAP_INVALID(),
		/* 82 */ KERN_TRAP_INVALID(),
		/* 83 */ KERN_TRAP_INVALID(),
		/* 84 */ KERN_TRAP_INVALID(),
		/* 85 */ KERN_TRAP_INVALID(),
		/* 86 */ KERN_TRAP_INVALID(),
		/* 87 */ KERN_TRAP_INVALID(),
		/* 88 */ KERN_TRAP_INVALID(),
		/* 89 */ KERN_TRAP_INVALID(),
		/* 90 */ KERN_TRAP_INVALID(),
		/* 91 */ KERN_TRAP_INVALID(),
		/* 92 */ KERN_TRAP_INVALID(),
		/* 93 */ KERN_TRAP_INVALID(),
		/* 94 */ KERN_TRAP_INVALID(),
		/* 95 */ KERN_TRAP_INVALID(),
		/* 96 */ KERN_TRAP_INVALID(),
		/* 97 */ KERN_TRAP_INVALID(),
		/* 98 */ KERN_TRAP_INVALID(),
		/* 99 */ KERN_TRAP_INVALID(),
		/* 100 */ KERN_TRAP_INVALID(),
		/* 101 */ KERN_TRAP_INVALID(),
		/* 102 */ KERN_TRAP_INVALID(),
		/* 103 */ KERN_TRAP_INVALID(),
		/* 104 */ KERN_TRAP_INVALID(),
		/* 105 */ KERN_TRAP_INVALID(),
		/* 106 */ KERN_TRAP_INVALID(),
		/* 107 */ KERN_TRAP_INVALID(),
		/* 108 */ KERN_TRAP_INVALID(),
		/* 109 */ KERN_TRAP_INVALID(),
		/* 110 */ KERN_TRAP_INVALID(),
		/* 111 */ KERN_TRAP_INVALID(),
		/* 112 */ KERN_TRAP_INVALID(),
		/* 113 */ KERN_TRAP_INVALID(),
		/* 114 */ KERN_TRAP_INVALID(),
		/* 115 */ KERN_TRAP_INVALID(),
		/* 116 */ KERN_TRAP_INVALID(),
		/* 117 */ KERN_TRAP_INVALID(),
		/* 118 */ KERN_TRAP_INVALID(),
		/* 119 */ KERN_TRAP_INVALID(),
		/* 120 */ KERN_TRAP_INVALID(),
		/* 121 */ KERN_TRAP_INVALID(),
		/* 122 */ KERN_TRAP_INVALID(),
		/* 123 */ KERN_TRAP_INVALID(),
		/* 124 */ KERN_TRAP_INVALID(),
		/* 125 */ KERN_TRAP_INVALID(),
		/* 126 */ KERN_TRAP_INVALID(),
		/* 127 */ KERN_TRAP_INVALID()
	};
}
