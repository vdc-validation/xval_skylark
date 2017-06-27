/*
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
 */

#include <arch.h>
#include <arch_helpers.h>
#include <bl_common.h>
#include <clk.h>
#include <config.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <mmio.h>
#include <platform_def.h>
#include <smpro_mbox.h>

#define MBOX_POLL_TIME			10	/* us */
#define MBOX_AVAIL_WAIT_TIMEOUT		1000000	/* Operation time out in us */
#define MBOX_ACK_WAIT_TIMEOUT		1000000	/* Operation time out in us */
#define MBOX_REG_SET_OFFSET		0x1000
#define MBOX_CNT			8
#define MBOX_STATUS_AVAIL_MASK		(1 << 16)
#define MBOX_STATUS_ACK_MASK		(1)

/* Configuration and Status Registers */
#define REG_DB_IN			0x00
#define REG_DB_DIN0			0x04
#define REG_DB_DIN1			0x08
#define REG_DB_OUT			0x10
#define REG_DB_DOUT0			0x14
#define REG_DB_DOUT1			0x18
#define REG_DB_STAT			0x20
#define REG_DB_STATMASK			0x24

static uint32_t read32(uintptr_t addr)
{
	return mmio_read_32(addr);
}

static void write32(uintptr_t addr, uint32_t val)
{
	mmio_write_32(addr, val);
}

int plat_smpro_mbox_rx(int mbox, uint32_t *msg)
{
	uintptr_t reg = SMPRO_SEC_MB_REG_BASE + mbox * MBOX_REG_SET_OFFSET;
	int32_t timeout = MBOX_AVAIL_WAIT_TIMEOUT;

	if (XGENE_VHP)
		return -ETIMEDOUT;


	while (!(read32(reg + REG_DB_STAT) & MBOX_STATUS_AVAIL_MASK)
			&& (timeout > 0)) {
		udelay(MBOX_POLL_TIME);
		timeout -= MBOX_POLL_TIME;
	}

	if (timeout <= 0) {
		ERROR("SMPro : MBox %d : Timeout waiting for RX AVAIL\n", mbox);
		return -ETIMEDOUT;
	}

	msg[1] = read32(reg + REG_DB_DIN0);
	msg[2] = read32(reg + REG_DB_DIN1);
	msg[0] = read32(reg + REG_DB_IN);
	write32(reg + REG_DB_STAT, MBOX_STATUS_AVAIL_MASK);

	return 0;
}

int plat_smpro_mbox_tx(int mbox, uint32_t *msg)
{
	uintptr_t reg = SMPRO_SEC_MB_REG_BASE + mbox * MBOX_REG_SET_OFFSET;
	int32_t timeout = MBOX_ACK_WAIT_TIMEOUT;

	if (XGENE_VHP)
		return -ETIMEDOUT;

	if (read32(reg + REG_DB_STAT) & MBOX_STATUS_ACK_MASK)
		write32(reg + REG_DB_STAT, MBOX_STATUS_ACK_MASK);

	write32(reg + REG_DB_DOUT0, msg[1]);
	write32(reg + REG_DB_DOUT1, msg[2]);
	write32(reg + REG_DB_OUT, msg[0]);

	while (!(read32(reg + REG_DB_STAT) & MBOX_STATUS_ACK_MASK)
			&& (timeout > 0)) {
		udelay(MBOX_POLL_TIME);
		timeout -= MBOX_POLL_TIME;
	}

	if (timeout <= 0) {
		ERROR("SMPro : MBox %d : Timeout waiting for TX ACK\n", mbox);
		return -ETIMEDOUT;
	}

	write32(reg + REG_DB_STAT, MBOX_STATUS_ACK_MASK);

	return 0;
}

int plat_smpro_mbox_read(int mbox, uint32_t *msg)
{
	uintptr_t reg = SMPRO_SEC_MB_REG_BASE + mbox * MBOX_REG_SET_OFFSET;

	if (XGENE_VHP)
		return -ETIMEDOUT;

	msg[1] = read32(reg + REG_DB_DIN0);
	msg[2] = read32(reg + REG_DB_DIN1);
	msg[0] = read32(reg + REG_DB_IN);
	return 0;
}
