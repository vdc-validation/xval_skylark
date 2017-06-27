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

#include <ahbc.h>
#include <arch_helpers.h>
#include <mmio.h>
#include <plat_psci_mbox.h>
#include <platform_def.h>
#include <skylark_pcp_csr.h>
#include <xgene_nvparam.h>

const unsigned char xgene_power_domain_tree_desc[] = {
	/* No of root nodes */
	1,
	/* No of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* No of CPU cores - cluster0 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster1 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster2 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster3 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster4 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster5 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster6 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster7 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster8 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster9 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster10 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster11 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster12 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster13 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster14 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster15 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
};

static uint32_t tb0_val, tb1_val;

/*******************************************************************************
 * Private function to issue a warm reset on a cpu.
 ******************************************************************************/
void xgene_plat_core_reset(uint32_t cpu_idx)
{
	mmio_write_32(CPUX_CPU_REG(cpu_idx, CPU_CRCR), CRCR_CWRR_MASK);
}

/*
 * Reset all core(s)
 *
 * Issue a warm reset to all cores (including itself)
 */
void xgene_plat_cores_reset(void)
{
	/* TODO: Warm reset has not supported yet so call cold reset instead */
	xgene_psci_set_power_state(SYS_REBOOT);
}

/*******************************************************************************
 * Private function to program the mailbox for a cpu before it is released
 * from reset.
 ******************************************************************************/
void xgene_plat_program_trusted_mailbox(uint32_t cpu_idx, uintptr_t address)
{
	uintptr_t *mailbox = (void *) PLAT_XGENE_TRUSTED_MAILBOX_BASE;

	mailbox[cpu_idx] = address;
}

/*******************************************************************************
 * Set PSCR value based on NVRAM param
 ******************************************************************************/
void plat_xgene_set_pscr(void)
{
	if (tb0_val != 0)
		write_S3_0_C15_C1_0(tb0_val);

	if (tb1_val != 0)
		write_S3_0_C15_C1_1(tb1_val);
}

void plat_xgene_init_pscr(void)
{
	int ret;

	/* Getting and setting TB0PSCR_EL1 */
	ret = plat_nvparam_get(NV_TB0PSCR, NV_PERM_ALL, &tb0_val);
	if (!ret)
		tb0_val = 0;

	/* Getting and setting TB1PSCR_EL1 */
	ret = plat_nvparam_get(NV_TB1PSCR, NV_PERM_ALL, &tb1_val);
	if (!ret)
		tb0_val = 0;

}

void xgene_plat_setup(void)
{
}
