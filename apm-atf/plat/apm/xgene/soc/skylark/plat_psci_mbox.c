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

#include <arch_helpers.h>
#include <bakery_lock.h>
#include <debug.h>
#include <delay_timer.h>
#include <mmio.h>
#include <plat_psci_mbox.h>
#include <platform.h>
#include <platform_def.h>
#include <skylark_pcp_csr.h>
#include <xgene_private.h>

#ifndef XGENE_VHP
#define XGENE_VHP			0
#endif

#define ACPI_SECURE_DB			5
#define MBOX_REG_SET_OFFSET		0x1000

/* Configuration and Status Registers */
#define REG_DB_OUT			0x10
#define REG_DB_DOUT0			0x14
#define REG_DB_STAT			0x20
#define  MBOX_STATUS_ACK_MASK		(1)
#define ACPI_PMPRO_DBx_REG(reg) \
	(PMPRO_SEC_MB_REG_BASE + MBOX_REG_SET_OFFSET * ACPI_SECURE_DB + (reg))
#define ACPI_SMPRO_DBx_REG(reg) \
	(SMPRO_SEC_MB_REG_BASE + MBOX_REG_SET_OFFSET * ACPI_SECURE_DB + (reg))

#define LPI_MSG_CMD			0x92800000
#define LPI_MSG_TIMEOUT			100000
#define ACPI_MBOX_POLL_TIME		1 /* us */

enum firmware_agent {
	SMPRO = 0,
	PMPRO = 1,
};

DEFINE_BAKERY_LOCK(psci_db_lock);

static void __xgene_psci_send_msg(uint32_t fw_agent, uint32_t msg)
{
	int timeout = LPI_MSG_TIMEOUT;
	uintptr_t reg_base;

	if (XGENE_VHP)
		return;

	if (fw_agent == SMPRO) {
		reg_base = SMPRO_SEC_MB_REG_BASE +
				MBOX_REG_SET_OFFSET * ACPI_SECURE_DB;
	} else {
		reg_base = PMPRO_SEC_MB_REG_BASE +
				MBOX_REG_SET_OFFSET * ACPI_SECURE_DB;
	}

	bakery_lock_get(&psci_db_lock);

	if (mmio_read_32(reg_base + REG_DB_STAT) & MBOX_STATUS_ACK_MASK)
		mmio_write_32(reg_base + REG_DB_STAT, MBOX_STATUS_ACK_MASK);

	mmio_write_32(reg_base + REG_DB_DOUT0, msg);
	mmio_write_32(reg_base + REG_DB_OUT, LPI_MSG_CMD);

	do {
		if (mmio_read_32(reg_base + REG_DB_STAT) & MBOX_STATUS_ACK_MASK)
			break;
		udelay(ACPI_MBOX_POLL_TIME);
	} while (timeout-- > 0);

	if (timeout <= 0)
		ERROR("ACPI MBox : Timeout waiting for TX ACK\n");

	mmio_write_32(reg_base + REG_DB_STAT, MBOX_STATUS_ACK_MASK);

	bakery_lock_release(&psci_db_lock);
}

void xgene_psci_set_lpi_state(uint32_t mpidr,
			      uint32_t cpu_state,
			      uint32_t cluster_state,
			      uint32_t system_state)
{
	uint32_t cluster_id, cpu_id;
	uint32_t state = 0;

	/*
	 * LPI Message Data Format (32 bit)
	 * --------------------------------------------------------------------
	 * | 31..28 | 27..24 | 23..20 | 19..16 | 15..12 | 11..8 | 7..4 | 3..0 |
	 * |-----------------|--------|--------|--------|--------------|------|
	 * |Reserved|CmdType | SYSLPI | CLULPI | CPULPI |    CLUID     | CPUID|
	 * --------------------------------------------------------------------
	 */
	cluster_id = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;
	cpu_id = (mpidr >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK;

	state |= cpu_id & 0xF;			/* CPU ID */
	state |= (cluster_id & 0xFF) << 4;	/* Cluster ID */
	state |= (cpu_state & 0xF) << 12;	/* CPU LPI state */
	state |= (cluster_state & 0xF) << 16;	/* CLU LPI state */
	state |= (system_state & 0xF) << 20;	/* SYS LPI state */
	state |= (ACPI_CMD_LPI_POWER_STATE & 0xF) << 24; /* ACPI command type */

	__xgene_psci_send_msg(PMPRO, state);
}

void xgene_psci_set_power_state(uint32_t state)
{
	uint32_t msg = 0;

	/*
	 * System Message Data Format (32 bit)
	 * --------------------------------------------------------------------
	 * | 31..28 | 27..24 | 23..20 | 19..16 | 15..12 | 11..8 | 7..4 | 3..0 |
	 * |-----------------|--------|--------|--------|--------------|------|
	 * |Reserved|CmdType |              Reserved                   | Cmd  |
	 * --------------------------------------------------------------------
	 */
	msg |= (ACPI_CMD_SYS_POWER_STATE & 0xF) << 24; /* ACPI command type */
	msg |= state & 0xF;			/* System command */

	__xgene_psci_send_msg(SMPRO, msg);
}
