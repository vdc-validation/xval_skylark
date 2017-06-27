/**
 * Copyright (c) 2017, Applied Micro Circuits Corporation. All rights reserved.
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
 */
#include <arch_helpers.h>
#include <debug.h>
#include <mmio.h>
#include <skylark_soc.h>
#include <xgene_def.h>
#include <xgene_irq.h>
#include <xgene_private.h>
#include "ras.h"

#define PCPLPERRINTSTS		0x800010
#define PCPLPERRINTMSK		0x800014
#define MEMERRINTMSK		0x80001c
#define MEMERRINTSTS		0x800018

#define PCPLPERRINTMSK_EN	0x00000000
#define MEMERRINTMSK_EN		0x70C0CF00

uint64_t xgene_ras_pcplperr_hdlr(uint32_t id, uint32_t flags, void *handle,
				 void *cookie)
{
	uint32_t sts;

	sts = mmio_read_32(GICD_REG_BASE + PCPLPERRINTSTS);
	if (sts) {
		tf_printf("%s: sts = 0x%x\n\r", __func__, sts);
	}

	return 0;
}

uint64_t xgene_ras_memerr_hdlr(uint32_t id, uint32_t flags, void *handle,
				 void *cookie)
{
	uint32_t sts;

	sts = mmio_read_32(GICD_REG_BASE + MEMERRINTSTS);
	if (sts) { 
		tf_printf("%s: sts = 0x%x\n\r", __func__, sts);
	}

	return 0;
}

void xgene_ras_enable_interrupts(void)
{
	mmio_write_32(GICD_REG_BASE + PCPLPERRINTMSK, PCPLPERRINTMSK_EN);
	mmio_write_32(GICD_REG_BASE + MEMERRINTMSK, MEMERRINTMSK_EN);
}

void xgene_ras_init_interrupts(void)
{
	xgene_irq_register_interrupt(
			XGENE_SPI_PCPLPERR, xgene_ras_pcplperr_hdlr);
	xgene_irq_register_interrupt(
			XGENE_SPI_MEMERR, xgene_ras_memerr_hdlr);
}
