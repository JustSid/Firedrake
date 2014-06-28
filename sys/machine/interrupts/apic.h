//
//  apic.h
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

#ifndef _APIC_H_
#define _APIC_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <kern/kern_return.h>
#include <machine/cpu.h>

#define APIC_DFR_FLAT    0xffffffff
#define APIC_DFR_CLUSTER 0x0fffffff

#define APIC_LVT_MASKED   (1 << 16)
#define APIC_LVT_PERIODIC (1 << 17)

#define APIC_ICR_DM_FIXED   (0 << 8)
#define APIC_ICR_DM_LOWEST  (1 << 8)
#define APIC_ICR_DM_SMI     (2 << 8)
#define APIC_ICR_DM_NMI     (4 << 8)
#define APIC_ICR_DM_INIT    (5 << 8)
#define APIC_ICR_DM_STARTUP (6 << 8)

#define APIC_ICR_DS_PENDING  (1 << 12)
#define APIC_ICR_LV_ASSERT   (1 << 14)
#define APIC_ICR_TM_LEVEL    (1 << 15)

#define APIC_ICR_DSS_DEST   (0 << 18)
#define APIC_ICR_DSS_SELF   (1 << 18)
#define APIC_ICR_DSS_ALL    (2 << 18)
#define APIC_ICR_DSS_OTHERS (3 << 18)

#define APICRegisterWithOffset(reg, offset) static_cast<Sys::APIC::Register>((static_cast<uint32_t>(reg) + offset))

namespace Sys
{
	namespace APIC
	{
		enum class Register : uint32_t
		{
			ID                = 0x20,
			Version           = 0x30,
			TPR               = 0x80,
			APR               = 0x90,
			PPR               = 0xa0,
			EOI               = 0xb0,
			RRD               = 0xc0,
			LDR               = 0xd0,
			DFR               = 0xe0,
			SVR               = 0xf0,
			ISR_BASE          = 0x100,
			TMR_BASE          = 0x180,
			IRR_BASE          = 0x200,
			ERROR_STATUS      = 0x280,
			LVT_CMCI          = 0x2f0,
			ICRL              = 0x300,
			ICRH              = 0x310,
			LVT_TIMER         = 0x320,
			LVT_THERMAL       = 0x330,
			LVT_PMC           = 0x340,
			LVT_LINT0         = 0x350,
			LVT_LINT1         = 0x360,
			LVT_ERROR         = 0x370,
			TMR_INITIAL_COUNT = 0x380,
			TMR_CURRENT_COUNT = 0x390,
			TMR_DIVIDE_CONFIG = 0x3e0
		};

		enum class TimerMode
		{
			Periodic = 0,
			OneShot  = 1
		};

		enum class TimerDivisor : uint8_t
		{
			DivideBy1   = 0xb,
			DivideBy2   = 0x0,
			DivideBy4   = 0x1,
			DivideBy8   = 0x2,
			DivideBy16  = 0x3,
			DivideBy32  = 0x8,
			DivideBy64  = 0x9,
			DivideBy128 = 0xa
		};

		void Write(Register reg, uint32_t value);
		uint32_t Read(Register reg);

		bool IsInterruptPending();
		bool IsInterrupting(uint8_t vector);

		// IPIs
		void SendIPI(uint8_t vector, CPU *cpu);
		void SendIPI(uint8_t vector, CPU *cpu, uint32_t flags);
		void BroadcastIPI(uint8_t vector, bool self);
		void BroadcastIPI(uint8_t vector, bool self, uint32_t flags);

		// Timer
		void ArmTimer(uint32_t count);
		void UnarmTimer();

		void SetTimer(TimerDivisor divisor, TimerMode mode, uint32_t initial);
		void GetTimer(TimerDivisor *divisor, TimerMode *mode, uint32_t *initial, uint32_t *current);

		// Misc
		void MaskInterrupt(uint8_t irq, bool masked); // Technically IOAPIC, but who cares?
	}

	typedef struct
	{
		uint8_t source;
		uint16_t flags;
		uint32_t system_interrupt;

		bool is_active_high() const;
		bool is_active_low() const;

		bool is_level_triggered() const;
		bool is_edge_triggered() const;
	} ioapic_interrupt_override_t;

	class IOAPIC
	{
	public:
		class InterruptOverride
		{
		public:
			friend class IOAPIC;

			bool IsActiveHigh() const { uint16_t polarity = (_flags & 0x3); return (polarity == 1); }
			bool IsLevelTriggered() const { uint16_t trigger = ((_flags >> 2) & 0x3); return (trigger == 3); }

			uint8_t GetSource() const { return _source; }
			uint16_t GetFlags() const { return _flags; }
			uint32_t GetSystemInterrupt() const { return _systemInterrupt; }

		private:
			InterruptOverride(uint8_t source, uint16_t flags, uint32_t systemInterrupt) :
				_source(source),
				_flags(flags),
				_systemInterrupt(systemInterrupt)
			{}

			uint8_t _source;
			uint16_t _flags;
			uint32_t _systemInterrupt;
		};

		enum class Register : uint32_t
		{
			ID      = 0x0,
			VER     = 0x1,
			ARB     = 0x2,
			REDTBL0 = 0x10
		};

		static IOAPIC *RegisterIOAPIC(uint8_t id, uint32_t address, uint32_t base);
		static InterruptOverride *RegisterInterruptOverride(uint8_t source, uint16_t flags, uint32_t systemInterrupt);

		static uint8_t ResolveInterrupt(uint8_t irq);
		static IOAPIC *GetIOAPICForInterrupt(uint8_t irq);
		static IOAPIC *GetIOAPICWithID(uint8_t id);

		static InterruptOverride *GetInterruptOverride(uint8_t irq);

		void Write(Register reg, uint32_t value);
		uint32_t Read(Register reg);

		uint8_t GetID() const { return _id; }
		uint8_t GetMaximumRedirectionEntry() const { return _maxRedirectionEntry; }
		uint32_t GetAddress() const { return _address; }
		uint32_t GetInterruptBase() const { return _interruptBase; }

		static kern_return_t MapIOAPICs();

	private:
		IOAPIC(uint8_t id, uint32_t address, uint32_t base);

		uint8_t _id;
		uint8_t _maxRedirectionEntry;
		uint32_t _address;
		uint32_t _interruptBase;
	};

	kern_return_t APICInit();
	kern_return_t APICInitCPU();
}

#endif /* _APIC_H_ */
