//
//  apic.h
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

#ifndef _APIC_H_
#define _APIC_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <kern/kern_return.h>
#include <machine/cpu.h>

#define APIC_REGISTER_ID                   0x20
#define APIC_REGISTER_VERSION              0x30
#define APIC_REGISTER_TPR                  0x80
#define APIC_REGISTER_APR                  0x90
#define APIC_REGISTER_PPR                  0xa0
#define APIC_REGISTER_EOI                  0xb0
#define APIC_REGISTER_RRD                  0xc0
#define APIC_REGISTER_LDR                  0xd0
#define APIC_REGISTER_DFR                  0xe0
#define APIC_REGISTER_SVR                  0xf0
#define APIC_REGISTER_ISR_BASE             0x100
#define APIC_REGISTER_TMR_BASE             0x180
#define APIC_REGISTER_IRR_BASE             0x200
#define APIC_REGISTER_ERROR_STATUS         0x280
#define APIC_REGISTER_LVT_CMCI             0x2f0
#define APIC_REGISTER_ICRL                 0x300
#define APIC_REGISTER_ICRH                 0x310
#define APIC_REGISTER_LVT_TIMER            0x320
#define APIC_REGISTER_LVT_THERMAL          0x330
#define APIC_REGISTER_LVT_PMC              0x340
#define APIC_REGISTER_LVT_LINT0            0x350
#define APIC_REGISTER_LVT_LINT1            0x360
#define APIC_REGISTER_LVT_ERROR            0x370
#define APIC_REGISTER_TIMER_INITAL_COUNT   0x380
#define APIC_REGISTER_TIMER_CURRENT_COUNT  0x390
#define APIC_REGISTER_TIMER_DIVIDE_CONFIG  0x3e0

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

namespace ir
{
	typedef struct
	{
		uint8_t id;
		uint8_t maxiumum_redirection_entry;
		uint32_t address;
		uint32_t interrupt_base;
	} ioapic_t;

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


	enum class apic_timer_mode_t
	{
		periodic = 0,
		one_shot = 1
	};

	enum class apic_timer_divisor_t : uint8_t
	{
		divide_by_1   = 0xb,
		divide_by_2   = 0x0,
		divide_by_4   = 0x1,
		divide_by_8   = 0x2,
		divide_by_16  = 0x3,
		divide_by_32  = 0x8,
		divide_by_64  = 0x9,
		divide_by_128 = 0xa
	};

	void apic_write(uint32_t reg, uint32_t value);
	uint32_t apic_read(uint32_t reg);

	bool apic_is_interrupt_pending();
	bool apic_is_interrupting(uint8_t vector);

	void apic_send_ipi(uint8_t vector, cpu_t *cpu);
	void apic_send_ipi(uint8_t vector, cpu_t *cpu, uint32_t flags);
	void apic_broadcast_ipi(uint8_t vector, bool self);
	void apic_broadcast_ipi(uint8_t vector, bool self, uint32_t flags);

	uint8_t apic_iopaic_resolve_interrupt(uint8_t irq);
	ioapic_t *apic_ioapic_servicing_interrupt(uint8_t irq);

	void apic_set_timer(bool masked, apic_timer_divisor_t divisor, apic_timer_mode_t mode, uint32_t initial_count);
	void apic_get_timer(apic_timer_divisor_t *divisor, apic_timer_mode_t *mode, uint32_t *initial, uint32_t *current);

	kern_return_t apic_add_interrupt_override(ioapic_interrupt_override_t *override);
	kern_return_t apic_add_ioapic(ioapic_t *ioapic);
	kern_return_t apic_init();
}

#endif /* _APIC_H_ */
