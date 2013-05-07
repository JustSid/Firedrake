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

/**
 * Overview:
 * Defines helper related to the CPU
 **/
#ifndef _CPU_H_
#define _CPU_H_

#include <prefix.h>

/**
 * Struct that is able to hold the state of the CPU
 **/
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

typedef enum
{
	cpu_vendor_Unknown,
	cpu_vendor_Intel,
	cpu_vendor_AMD
} cpu_vendor_t;

#define kCPUFeatureSSE3    (UINT64_C(1) << 0) // SSE3
#define kCPUFeaturePCLMUL  (UINT64_C(1) << 1) // PCLMULQDQ support
#define kCPUFeatureDTES    (UINT64_C(1) << 2) // 64 bit debug store
#define kCPUFeatureMONITOR (UINT64_C(1) << 3) // MONITOR and MWAIT instructions
#define kCPUFeatureDSCPL   (UINT64_C(1) << 4) // CPL qualified debug store (wtf is that?!)
#define kCPUFeatureVMX     (UINT64_C(1) << 5) // Virtual machine extensions
#define kCPUFeatureSMX     (UINT64_C(1) << 6) // Safer mode extensions
#define kCPUFeatureEST     (UINT64_C(1) << 7) // Enhanced speedstep
#define kCPUFeatureTM2     (UINT64_C(1) << 8) // Thermal Monitor 2 
#define kCPUFeatureSSSE3   (UINT64_C(1) << 9) // Supplemental SSE 3
#define kCPUFeatureCID     (UINT64_C(1) << 10) // Context ID   
#define kCPUFeatureFMA     (UINT64_C(1) << 12) // Fused multiply add
#define kCPUFeatureCX16    (UINT64_C(1) << 13) // cpmxchg16b instruction
#define kCPUFeatureXTPR    (UINT64_C(1) << 14) // Disable sending task priority messages
#define kCPUFeaturePDCM    (UINT64_C(1) << 15) // Perfmon and debug capability
#define kCPUFeaturePCID    (UINT64_C(1) << 17) // Process context identifiers
#define kCPUFeatureDCA     (UINT64_C(1) << 18) // Direct cache access for DMA writes
#define kCPUFeatureSSE41   (UINT64_C(1) << 19) 
#define kCPUFeatureSSE42   (UINT64_C(1) << 20) 
#define kCPUFeatureX2APIC  (UINT64_C(1) << 21)
#define kCPUFeatureMOVBE   (UINT64_C(1) << 22) // MOVBE instruction
#define kCPUFeaturePOPCNT  (UINT64_C(1) << 23) // POPCNT instruction
#define kCPUFeatureTSCDEAD (UINT64_C(1) << 24) // One shot operation support
#define kCPUFeatureAES     (UINT64_C(1) << 25) // AES instruction set
#define kCPUFeatureXSAVE   (UINT64_C(1) << 26) // XSAVE, XRESTORE, XSETBV, XGETBV
#define kCPUFeatureOSXSAVE (UINT64_C(1) << 27) // XSAVE enabled by OS
#define kCPUFeatureAVX     (UINT64_C(1) << 28)
#define kCPUFeatureF16C    (UINT64_C(1) << 29) // CVT16 instruction set support
#define kCPUFeatureRDRAND  (UINT64_C(1) << 30) // RDRAND instruction
#define kCPUFeatureHYPER   (UINT64_C(1) << 31) // Running on a hypervisor 

#define kCPUFeatureFPU    (UINT64_C(1) << 32 + 0)
#define kCPUFeatureVME    (UINT64_C(1) << 32 + 1) // Virtual mode extension
#define kCPUFeatureDE     (UINT64_C(1) << 32 + 2) // Debug extensions
#define kCPUFeaturePSE    (UINT64_C(1) << 32 + 3) // Page size extension
#define kCPUFeatureTSC    (UINT64_C(1) << 32 + 4) // Time stamp counter
#define kCPUFeatureMSR    (UINT64_C(1) << 32 + 5)
#define kCPUFeaturePAE    (UINT64_C(1) << 32 + 6) // Physical address extension
#define kCPUFeatureMCE    (UINT64_C(1) << 32 + 7) // Machine check exception
#define kCPUFeatureCX8    (UINT64_C(1) << 32 + 8) // cmpxchg8 support
#define kCPUFeatureAPIC   (UINT64_C(1) << 32 + 9)
#define kCPUFeatureSEP    (UINT64_C(1) << 32 + 11) // SYSENTER and SYSEXIT
#define kCPUFeatureMTRR   (UINT64_C(1) << 32 + 12) // Memory type range registers
#define kCPUFeaturePGE    (UINT64_C(1) << 32 + 13) // Page global enable bit
#define kCPUFeatureMCA    (UINT64_C(1) << 32 + 14) // Machine check architecture
#define kCPUFeatureCMOV   (UINT64_C(1) << 32 + 15) // Conditional move
#define kCPUFeaturePAT    (UINT64_C(1) << 32 + 16) // Page attribute table
#define kCPUFeaturePSE36  (UINT64_C(1) << 32 + 17) // 36bit page size extension
#define kCPUFeaturePN     (UINT64_C(1) << 32 + 18)
#define kCPUFeatureCFLUSH (UINT64_C(1) << 32 + 19) // cflush 
#define kCPUFeatureDTS    (UINT64_C(1) << 32 + 21) // Debug store
#define kCPUFeatureACPI   (UINT64_C(1) << 32 + 22) // MSRs for ACPI
#define kCPUFeatureMMX    (UINT64_C(1) << 32 + 23) 
#define kCPUFeatureFXSR   (UINT64_C(1) << 32 + 24) // FXSAVE and FXRESTOR instructions
#define kCPUFeatureSSE    (UINT64_C(1) << 32 + 25)
#define kCPUFeatureSSE2   (UINT64_C(1) << 32 + 26) 
#define kCPUFeatureSS     (UINT64_C(1) << 32 + 27) // Cache supports self-snoop
#define kCPUFeatureHT     (UINT64_C(1) << 32 + 28) // Hyperthreading
#define kCPUFeatureTM     (UINT64_C(1) << 32 + 29) // Thermal monitor
#define kCPUFeatureIA64   (UINT64_C(1) << 32 + 30) // IA64 processor emulating x86
#define kCPUFeaturePBE    (UINT64_C(1) << 32 + 31) // Pending break enale

typedef struct
{
	cpu_vendor_t vendor;

	char stepping;
	char model;
	char family;

	char extendedModel;
	char extendedFamily;
	char type;

	uint64_t features;
} cpu_info_t;

struct cpuid_registers_s
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};


static inline void cpu_writeMSR(uint32_t msr, uint64_t value)
{
	uint32_t high = value >> 32;
	uint32_t low = value & UINT32_MAX;

	__asm__ volatile("wrmsr" :: "a" (low), "d" (high), "c" (msr));
}

static inline uint64_t cpu_readMSR(uint32_t msr)
{
	uint32_t high;
	uint32_t low;

	__asm__ volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

	return (low | (high << 31));
}


void cpuid(struct cpuid_registers_s *registers);

bool cpu_init(void *data);

#endif /* _CPU_H_ */
