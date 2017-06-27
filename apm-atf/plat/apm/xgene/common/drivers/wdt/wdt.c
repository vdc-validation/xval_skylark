/**
 * Copyright (c) 2016, Applied Micro Circuits Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Skylark SoC has two SBSA watch dog timers - one secure and one non-secure.
 * The secure watch dog timer interrupt WS0 is routed to IRQ94 EL3 and WS1 is
 * routed to SMpro. The non-secure watch dog timer interrupt WS0 is routed
 * to IRQ92 for EL2 and WS1 to IRQ93 for EL3.
 *
 * Here is the processing logic for each IRQ's:
 *
 * Non-secure WS0 - EL2 or BIOS/OS
 * Non-secure WS1 - EL3 (ATF) - This driver
 * Secure WS0 - EL3 (ATF) - Handled else where as this is part of failsafe
 * Secure WS1 - SMpro
 *
 * This module will wire up the IRQ handler only for non-secure WS1. It is
 * expected that the BIOS or OS will configure the watch dog if required.
 */
#include <arch_helpers.h>
#include <debug.h>
#include <mmio.h>
#include <xgene_def.h>
#include <xgene_irq.h>
#include <xgene_private.h>

#define WDT_CTRL_WCS_OFF		0x0
#define  WDT_CTRL_WCS_WS1_MASK		0x4

/*
 * Non-secure watch dog timer expired. We should do a warm reset the system
 */
uint64_t xgene_wdt_ws1_hdlr(uint32_t id, uint32_t flags, void *handle,
			    void *cookie)
{
	if (mmio_read_32(WDT_NS_REG_BASE + WDT_CTRL_WCS_OFF)
			& WDT_CTRL_WCS_WS1_MASK) {
		ERROR("Non-secure watch dog expired. Resetting system...\n");
		xgene_plat_cores_reset();
		/* We should never get to here */
		panic();
	}

	return 0;
}

void xgene_wdt_init(void)
{
	/* Setup interrupt processing for non-secure WS1 */
	xgene_irq_register_interrupt(
			XGENE_SPI_NS_WS1_TIMER, xgene_wdt_ws1_hdlr);
}
