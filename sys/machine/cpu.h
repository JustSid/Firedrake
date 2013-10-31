//
//  cpu.h
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

#ifndef _TSS_H_
#define _TSS_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <kern/kern_return.h>

typedef struct
{
	uint32_t back_link;	/* segment number of previous task, if nested */
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
	uint16_t trace_trap; /* trap on switch to this task */
	uint16_t io_bit_map_offset; /* offset to start of IO permission bit map */
} tss_t;

typedef struct 
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
} cpu_state_t;

typedef struct
{
	int8_t stepping;
	int8_t model;
	int8_t family;

	int8_t extendedModel;
	int8_t extendedFamily;
	int8_t type;

	uint64_t features;
} cpu_info_t;

#define CPU_FLAG_BOOTSTRAP (1 << 0)
#define CPU_FLAG_RUNNING   (1 << 1)

typedef struct
{
	uint8_t id;
	uint8_t apic_id;

	uint16_t flags;

	tss_t tss; // This one isn't used yet!
} cpu_t;

typedef struct
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
} cpuid_registers_t;

#define CPUID_FLAG_SSE3    (UINT64_C(1) << 0) // SSE3
#define CPUID_FLAG_PCLMUL  (UINT64_C(1) << 1) // PCLMULQDQ support
#define CPUID_FLAG_DTES    (UINT64_C(1) << 2) // 64 bit debug store
#define CPUID_FLAG_MONITOR (UINT64_C(1) << 3) // MONITOR and MWAIT instructions
#define CPUID_FLAG_DSCPL   (UINT64_C(1) << 4) // CPL qualified debug store (wtf is that?!)
#define CPUID_FLAG_VMX     (UINT64_C(1) << 5) // Virtual machine extensions
#define CPUID_FLAG_SMX     (UINT64_C(1) << 6) // Safer mode extensions
#define CPUID_FLAG_EST     (UINT64_C(1) << 7) // Enhanced speedstep
#define CPUID_FLAG_TM2     (UINT64_C(1) << 8) // Thermal Monitor 2 
#define CPUID_FLAG_SSSE3   (UINT64_C(1) << 9) // Supplemental SSE 3
#define CPUID_FLAG_CID     (UINT64_C(1) << 10) // Context ID   
#define CPUID_FLAG_FMA     (UINT64_C(1) << 12) // Fused multiply add
#define CPUID_FLAG_CX16    (UINT64_C(1) << 13) // cpmxchg16b instruction
#define CPUID_FLAG_XTPR    (UINT64_C(1) << 14) // Disable sending task priority messages
#define CPUID_FLAG_PDCM    (UINT64_C(1) << 15) // Perfmon and debug capability
#define CPUID_FLAG_PCID    (UINT64_C(1) << 17) // Process context identifiers
#define CPUID_FLAG_DCA     (UINT64_C(1) << 18) // Direct cache access for DMA writes
#define CPUID_FLAG_SSE41   (UINT64_C(1) << 19) 
#define CPUID_FLAG_SSE42   (UINT64_C(1) << 20) 
#define CPUID_FLAG_X2APIC  (UINT64_C(1) << 21)
#define CPUID_FLAG_MOVBE   (UINT64_C(1) << 22) // MOVBE instruction
#define CPUID_FLAG_POPCNT  (UINT64_C(1) << 23) // POPCNT instruction
#define CPUID_FLAG_TSCDEAD (UINT64_C(1) << 24) // One shot operation support
#define CPUID_FLAG_AES     (UINT64_C(1) << 25) // AES instruction set
#define CPUID_FLAG_XSAVE   (UINT64_C(1) << 26) // XSAVE, XRESTORE, XSETBV, XGETBV
#define CPUID_FLAG_OSXSAVE (UINT64_C(1) << 27) // XSAVE enabled by OS
#define CPUID_FLAG_AVX     (UINT64_C(1) << 28)
#define CPUID_FLAG_F16C    (UINT64_C(1) << 29) // CVT16 instruction set support
#define CPUID_FLAG_RDRAND  (UINT64_C(1) << 30) // RDRAND instruction
#define CPUID_FLAG_HYPER   (UINT64_C(1) << 31) // Running on a hypervisor 

#define CPUID_FLAG_FPU    (UINT64_C(1) << 32 + 0)
#define CPUID_FLAG_VME    (UINT64_C(1) << 32 + 1) // Virtual mode extension
#define CPUID_FLAG_DE     (UINT64_C(1) << 32 + 2) // Debug extensions
#define CPUID_FLAG_PSE    (UINT64_C(1) << 32 + 3) // Page size extension
#define CPUID_FLAG_TSC    (UINT64_C(1) << 32 + 4) // Time stamp counter
#define CPUID_FLAG_MSR    (UINT64_C(1) << 32 + 5)
#define CPUID_FLAG_PAE    (UINT64_C(1) << 32 + 6) // Physical address extension
#define CPUID_FLAG_MCE    (UINT64_C(1) << 32 + 7) // Machine check exception
#define CPUID_FLAG_CX8    (UINT64_C(1) << 32 + 8) // cmpxchg8 support
#define CPUID_FLAG_APIC   (UINT64_C(1) << 32 + 9)
#define CPUID_FLAG_SEP    (UINT64_C(1) << 32 + 11) // SYSENTER and SYSEXIT
#define CPUID_FLAG_MTRR   (UINT64_C(1) << 32 + 12) // Memory type range registers
#define CPUID_FLAG_PGE    (UINT64_C(1) << 32 + 13) // Page global enable bit
#define CPUID_FLAG_MCA    (UINT64_C(1) << 32 + 14) // Machine check architecture
#define CPUID_FLAG_CMOV   (UINT64_C(1) << 32 + 15) // Conditional move
#define CPUID_FLAG_PAT    (UINT64_C(1) << 32 + 16) // Page attribute table
#define CPUID_FLAG_PSE36  (UINT64_C(1) << 32 + 17) // 36bit page size extension
#define CPUID_FLAG_PN     (UINT64_C(1) << 32 + 18)
#define CPUID_FLAG_CFLUSH (UINT64_C(1) << 32 + 19) // cflush 
#define CPUID_FLAG_DTS    (UINT64_C(1) << 32 + 21) // Debug store
#define CPUID_FLAG_ACPI   (UINT64_C(1) << 32 + 22) // MSRs for ACPI
#define CPUID_FLAG_MMX    (UINT64_C(1) << 32 + 23) 
#define CPUID_FLAG_FXSR   (UINT64_C(1) << 32 + 24) // FXSAVE and FXRESTOR instructions
#define CPUID_FLAG_SSE    (UINT64_C(1) << 32 + 25)
#define CPUID_FLAG_SSE2   (UINT64_C(1) << 32 + 26) 
#define CPUID_FLAG_SS     (UINT64_C(1) << 32 + 27) // Cache supports self-snoop
#define CPUID_FLAG_HT     (UINT64_C(1) << 32 + 28) // Hyperthreading
#define CPUID_FLAG_TM     (UINT64_C(1) << 32 + 29) // Thermal monitor
#define CPUID_FLAG_IA64   (UINT64_C(1) << 32 + 30) // IA64 processor emulating x86
#define CPUID_FLAG_PBE    (UINT64_C(1) << 32 + 31) // Pending break enabled

static inline void cpu_write_msr(uint32_t msr, uint64_t value)
{
	uint32_t high = value >> 32;
	uint32_t low = value & UINT32_MAX;

	__asm__ volatile("wrmsr" :: "a" (low), "d" (high), "c" (msr));
}

static inline uint64_t cpu_read_msr(uint32_t msr)
{
	uint32_t high;
	uint32_t low;

	__asm__ volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

	return (low | (high << 31));
}

static inline void cpu_pause()
{
	__asm__ volatile("rep; nop");
}

static inline void cpu_halt()
{
	__asm__ volatile("hlt");
}

void cpuid(cpuid_registers_t &registers);

cpu_info_t *cpu_get_info();

cpu_t *cpu_get_cpu_with_id(uint32_t id);
cpu_t *cpu_get_cpu_with_apic(uint8_t apic);
cpu_t *cpu_get_current_cpu();
uint8_t cpu_get_cpu_number();
uint32_t cpu_get_cpu_count();

kern_return_t cpu_add_cpu(cpu_t *cpu);
kern_return_t cpu_init();

#endif
