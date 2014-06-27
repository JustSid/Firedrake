//
//  cpu.h
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

#ifndef _TSS_H_
#define _TSS_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libc/stddef.h>
#include <libcpp/bitfield.h>
#include <kern/kern_return.h>

namespace ir
{
	struct trampoline_cpu_data_t;
}

namespace Sys
{
	struct TSS
	{
		uint32_t backLink;	/* segment number of previous task, if nested */
		uint32_t esp0; /* initial stack pointer ... */
		uint32_t ss0; /* and segment for ring 0 */
		uint32_t esp1; /* initial stack pointer ... */
		uint32_t ss1; /* and segment for ring 1 */
		uint32_t esp2; /* initial stack pointer ... */
		uint32_t ss2; /* and segment for ring 2 */
		uint32_t cr3; /* CR3 - page table directory physical address */
		uint32_t eip;
		uint32_t eflags;
		uint32_t eax;
		uint32_t ecx;
		uint32_t edx;
		uint32_t ebx;
		uint32_t esp; /* current stack pointer */
		uint32_t ebp;
		uint32_t esi;
		uint32_t edi;
		uint32_t es;
		uint32_t cs;
		uint32_t ss; /* current stack segment */
		uint32_t ds;
		uint32_t fs;
		uint32_t gs;
		uint32_t ldt; /* local descriptor table segment */
		uint16_t traceTrap; /* trap on switch to this task */
		uint16_t IOBitmapOffset; /* offset to start of IO permission bit map */
	};

	struct CPUState
	{
		uint16_t gs __attribute__((aligned(4)));
		uint16_t fs __attribute__((aligned(4)));
		uint16_t es __attribute__((aligned(4)));
		uint16_t ds __attribute__((aligned(4)));

		uint32_t edi;
		uint32_t esi;
		uint32_t ebp;
		uint32_t esp_;
		uint32_t ebx;
		uint32_t edx;
		uint32_t ecx;
		uint32_t eax;

		uint32_t interrupt;
		uint32_t error;

		uint32_t eip;
		uint32_t cs;
		uint32_t eflags;
		uint32_t esp;
		uint32_t ss;
	};

	enum class CPUVendor
	{
		Intel,
		AMD,
		Unknown
	};

	

	class CPUInfo
	{
	public:
		struct Feature : public cpp::Bitfield<uint64_t, true>
		{
			Feature()
			{}
			Feature(uint64_t value) :
				Bitfield(value)
			{}

			enum
			{
				SSE3    = (UINT64_C(1) << 0), // SSE3
				PCLMUL  = (UINT64_C(1) << 1), // PCLMULQDQ support
				DTES    = (UINT64_C(1) << 2), // 64 bit debug store
				MONITOR = (UINT64_C(1) << 3), // MONITOR and MWAIT instructions
				DSCPL   = (UINT64_C(1) << 4), // CPL qualified debug store (wtf is that?!)
				VMX     = (UINT64_C(1) << 5), // Virtual machine extensions
				SMX     = (UINT64_C(1) << 6), // Safer mode extensions
				EST     = (UINT64_C(1) << 7), // Enhanced speedstep
				TM2     = (UINT64_C(1) << 8), // Thermal Monitor 2 
				SSSE3   = (UINT64_C(1) << 9), // Supplemental SSE 3
				CID     = (UINT64_C(1) << 10), // Context ID   
				FMA     = (UINT64_C(1) << 12), // Fused multiply add
				CX16    = (UINT64_C(1) << 13), // cpmxchg16b instruction
				XTPR    = (UINT64_C(1) << 14), // Disable sending task priority messages
				PDCM    = (UINT64_C(1) << 15), // Perfmon and debug capability
				PCID    = (UINT64_C(1) << 17), // Process context identifiers
				DCA     = (UINT64_C(1) << 18), // Direct cache access for DMA writes
				SSE41   = (UINT64_C(1) << 19), 
				SSE42   = (UINT64_C(1) << 20),
				X2APIC  = (UINT64_C(1) << 21),
				MOVBE   = (UINT64_C(1) << 22), // MOVBE instruction
				POPCNT  = (UINT64_C(1) << 23), // POPCNT instruction
				TSCDEAD = (UINT64_C(1) << 24), // One shot operation support
				AES     = (UINT64_C(1) << 25), // AES instruction set
				XSAVE   = (UINT64_C(1) << 26), // XSAVE, XRESTORE, XSETBV, XGETBV
				OSXSAVE = (UINT64_C(1) << 27), // XSAVE enabled by OS
				AVX     = (UINT64_C(1) << 28),
				F16C    = (UINT64_C(1) << 29), // CVT16 instruction set support
				RDRAND  = (UINT64_C(1) << 30), // RDRAND instruction
				HYPER   = (UINT64_C(1) << 31), // Running on a hypervisor

				FPU    = (UINT64_C(1) << 32 + 0),
				VME    = (UINT64_C(1) << 32 + 1), // Virtual mode extension
				DE     = (UINT64_C(1) << 32 + 2), // Debug extensions
				PSE    = (UINT64_C(1) << 32 + 3), // Page size extension
				TSC    = (UINT64_C(1) << 32 + 4), // Time stamp counter
				MSR    = (UINT64_C(1) << 32 + 5),
				PAE    = (UINT64_C(1) << 32 + 6), // Physical address extension
				MCE    = (UINT64_C(1) << 32 + 7), // Machine check exception
				CX8    = (UINT64_C(1) << 32 + 8), // cmpxchg8 support
				APIC   = (UINT64_C(1) << 32 + 9),
				SEP    = (UINT64_C(1) << 32 + 11), // SYSENTER and SYSEXIT
				MTRR   = (UINT64_C(1) << 32 + 12), // Memory type range registers
				PGE    = (UINT64_C(1) << 32 + 13), // Page global enable bit
				MCA    = (UINT64_C(1) << 32 + 14), // Machine check architecture
				CMOV   = (UINT64_C(1) << 32 + 15), // Conditional move
				PAT    = (UINT64_C(1) << 32 + 16), // Page attribute table
				PSE36  = (UINT64_C(1) << 32 + 17), // 36bit page size extension
				PN     = (UINT64_C(1) << 32 + 18),
				CFLUSH = (UINT64_C(1) << 32 + 19), // cflush 
				DTS    = (UINT64_C(1) << 32 + 21), // Debug store
				ACPI   = (UINT64_C(1) << 32 + 22), // MSRs for ACPI
				MMX    = (UINT64_C(1) << 32 + 23), 
				FXSR   = (UINT64_C(1) << 32 + 24), // FXSAVE and FXRESTOR instructions
				SSE    = (UINT64_C(1) << 32 + 25),
				SSE2   = (UINT64_C(1) << 32 + 26), 
				SS     = (UINT64_C(1) << 32 + 27), // Cache supports self-snoop
				HT     = (UINT64_C(1) << 32 + 28), // Hyperthreading
				TM     = (UINT64_C(1) << 32 + 29), // Thermal monitor
				IA64   = (UINT64_C(1) << 32 + 30), // IA64 processor emulating x86
				PBE    = (UINT64_C(1) << 32 + 31) // Pending break enabled
			};
		};

		CPUInfo();

		int8_t GetStepping() const { return _stepping; }
		int8_t GetModel() const { return _model; }
		int8_t GetFamily() const { return _family; }

		int8_t GetExtendedModel() const { return _extendedModel; }
		int8_t GetExtendedFamily() const { return _extendedFamily; }
		int8_t GetType() const { return _type; }

		CPUVendor GetVendor() const { return _vendor; }
		Feature GetFeatures() const { return _features; }

	private:
		int8_t _stepping;
		int8_t _model;
		int8_t _family;

		int8_t _extendedModel;
		int8_t _extendedFamily;
		int8_t _type;

		Feature _features;
		CPUVendor _vendor;
	};

	class CPU
	{
	public:
		struct Flags : cpp::Bitfield<uint32_t>
		{
			Flags()
			{}

			Flags(int value) :
				Bitfield(value)
			{}

			enum
			{
				Bootstrap = (1 << 0),
				Running   = (1 << 1),
				Timedout  = (1 << 2)
			};
		};

		CPU();
		CPU(uint8_t apicID, Flags flags);
		CPU(const CPU &other);

		CPU &operator =(const CPU &other);

		// Must be called from the bootstrap CPU before any other CPU is bootstrapped
		void Register();

		// Must be called from the actual CPU!
		void Bootstrap();

		uint8_t GetID() const { return _id; }
		uint8_t GetApicID() const { return _apicID; }

		Flags GetFlags() const { return _flags; }
		CPUState *GetLastState() const { return const_cast<CPUState *>(_lastState); }
		CPUInfo *GetInfo() const { return const_cast<CPUInfo *>(&_info); } // Not valid until bootstrapped
		ir::trampoline_cpu_data_t *GetTrampoline() const { return const_cast<ir::trampoline_cpu_data_t *>(_trampoline); }

		void SetState(CPUState *state);
		void SetTrampoline(ir::trampoline_cpu_data_t *trampoline);
		void SetFlags(Flags flags);

		static CPU *GetCPUWithID(uint32_t id);
		static CPU *GetCPUWithApicID(uint32_t apicID);
		static CPU *GetCurrentCPU();

		static uint32_t GetCPUID();
		static size_t GetCPUCount();

	private:
		uint8_t _id;
		uint8_t _apicID;

		Flags _flags;
		CPUState *_lastState;
		CPUInfo _info;

		ir::trampoline_cpu_data_t *_trampoline;

		bool _registered;
	};

	class CPUID
	{
	public:
		CPUID(uint32_t id);

		uint32_t GetEAX() const { return _eax; }
		uint32_t GetEBX() const { return _ebx; }
		uint32_t GetECX() const { return _ecx; }
		uint32_t GetEDX() const { return _edx; }

	private:
		uint32_t _eax;
		uint32_t _ebx;
		uint32_t _ecx;
		uint32_t _edx;
	};


	static inline void CPUWriteMSR(uint32_t msr, uint64_t value)
	{
		uint32_t high = value >> 32;
		uint32_t low = value & UINT32_MAX;

		__asm__ volatile("wrmsr" :: "a" (low), "d" (high), "c" (msr));
	}

	static inline uint64_t CPUReadMSR(uint32_t msr)
	{
		uint32_t high;
		uint32_t low;

		__asm__ volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

		return (low | (high << 31));
	}

	static inline void CPUPause()
	{
		__asm__ volatile("rep; nop");
	}

	static inline void CPUHalt()
	{
		__asm__ volatile("hlt");
	}

	kern_return_t CPUInit();
}

#endif
