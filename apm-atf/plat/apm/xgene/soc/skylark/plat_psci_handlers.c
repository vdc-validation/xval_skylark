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
#include <assert.h>
#include <bakery_lock.h>
#include <debug.h>
#include <gicv3.h>
#include <mmio.h>
#include <plat_arm.h>
#include <plat_psci_mbox.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <skylark_pcp_csr.h>
#include <xgene_pm_tracer.h>
#include <xgene_private.h>

#ifndef XGENE_VHP
#define XGENE_VHP	0
#endif

/* Macros to read the power domain state */
#define XGENE_CORE_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define XGENE_CLUSTER_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define XGENE_SYSTEM_PWR_STATE(state) \
	((PLAT_MAX_PWR_LVL > MPIDR_AFFLVL1) ?\
		(state)->pwr_domain_state[MPIDR_AFFLVL2] : 0)

#define CORE_PER_CLU		PLATFORM_MAX_CPUS_PER_CLUSTER

static uint32_t scr_el3[PLATFORM_CORE_COUNT];

DEFINE_SYSOP_TYPE_FUNC(ic, iallu)

static void xgene_power_down_common(uint32_t cpu_idx,
				    uint32_t cpu_state,
				    uint32_t cluster_state,
				    uint32_t system_state)
{
	uint32_t val;

	if (!XGENE_VHP) {
		/* Step 1.: Transition to EL3 and guarantee that the following have occurred: */
		/* FIXME: a. Disable CPU CTI input and output triggers */
		/* b. Disable caching and translation and then flush the TLBs and instruction cache */
		/*
		 * i. Branch to address where VA is mapped to PA
		 * ii. Write SCTLR_EL3 to clear the I, C, and M bits (others unchanged)
		 * iii. Execute DSB and ISB
		 */
		disable_mmu_icache_el3();

		/* iv. Execute IC IALLU */
		iciallu();

		/* v. Write SCR_EL3 to set NS (others unchanged) */
		val = read_scr_el3();
		write_scr_el3(val | SCR_NS_BIT);
		/* vi. Execute ISB */
		/* vii. Execute TLBI ALLE1 */
		/* viii. Execute TLBI ALLE2 */
		isb();
		tlbialle1();
		tlbialle2();

		/* ix. Write SCR_EL3 to clear NS (others unchanged) */
		/* x. Execute ISB */
		/* xi. Execute TLBI ALLE1 */
		/* xii. Execute TLBI ALLE3 */
		/* xiii. Execute DSB */
		write_scr_el3(val & (~SCR_NS_BIT));
		isb();
		tlbialle1();
		tlbialle3();
		dsb();

		/* c. Turn off all interrupt enables in the CPU interface */
		/* i. Write ICC_IGRPEN0_EL1 to clear Enable (others unchanged) */
		write_icc_igrpen0_el1(read_icc_igrpen0_el1() &
				      ~IGRPEN1_EL1_ENABLE_G0_BIT);

		/* ii. Write ICC_IGRPEN1_EL3 to clear EnableGrp1S and EnableGrp1NS */
		write_icc_igrpen1_el3(read_icc_igrpen1_el3() &
				      ~(IGRPEN1_EL3_ENABLE_G1NS_BIT |
					IGRPEN1_EL3_ENABLE_G1S_BIT));

		/* iii. Execute ISB */
		/* iv. Execute DSB */
		/* v. Execute ISB */
		isb();
		dsb();
		isb();
	}

	/*
	 * Step 2: Determine whether this CPU is the "Primary" or "Secondary" CPU.
	 * Synchronize with other CPU at CPU Flush Done
	 */
	if (!xgene_clu_barrier_dec_return(cpu_idx)) {
		/* Primary CPU only */
		/*
		 * Step 4: Write PMDx.L2CR to set HipDis and HdpDis (others unchanged)
		 */
		if (!XGENE_VHP) {
			val = mmio_read_32(PMDX_L2C_REG(cpu_idx/CORE_PER_CLU, PMD_L2C_L2CR));
			val |= L2CR_HIPDIS | L2CR_HDPDIS;
			mmio_write_32(PMDX_L2C_REG(cpu_idx/CORE_PER_CLU, PMD_L2C_L2CR), val);
		}

		/* Step 5: Execute DSB */
		dsb();

		/* Step 6: For each L2C set and way: Execute DC CISW */
		dcsw_op_all(DCCISW);

		/* Step 7: Execute DSB */
		dsb();
	}

	/* Step 8: Send Request for PMD Off-line message to PM Firmware */
	xgene_psci_set_lpi_state(read_mpidr_el1(), cpu_state,
				 cluster_state, system_state);
}

int32_t xgene_soc_validate_power_state(uint32_t power_state,
					psci_power_state_t *req_state)
{
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);
	int pstate = psci_get_pstate_type(power_state);
	int i;

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	/* Sanity check the requested state */
	if (pstate == PSTATE_TYPE_STANDBY) {
		for (i = MPIDR_AFFLVL0; i <= pwr_lvl; i++)
			req_state->pwr_domain_state[i] = ARM_LOCAL_STATE_RET;
	} else {
		for (i = MPIDR_AFFLVL0; i <= pwr_lvl; i++)
			req_state->pwr_domain_state[i] = ARM_LOCAL_STATE_OFF;
	}

	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	uint32_t cpu_state = XGENE_CORE_PWR_STATE(target_state);
	uint32_t cluster_state = XGENE_CLUSTER_PWR_STATE(target_state);
	uint32_t system_state = XGENE_SYSTEM_PWR_STATE(target_state);
	int cpu_idx = plat_my_core_pos();

	scr_el3[cpu_idx] = read_scr_el3();

	if (system_state == ARM_LOCAL_STATE_RET) {
		assert(cpu_state == PLAT_MAX_RET_STATE);
		assert(cluster_state == PLAT_MAX_RET_STATE);
		/* Prepare for SoC retention */
		trace_power_flow(cpu_idx, SOC_RETEN);
	} else if (system_state == ARM_LOCAL_STATE_OFF) {
		assert(cpu_state == PLAT_MAX_OFF_STATE);
		assert(cluster_state == PLAT_MAX_OFF_STATE);
		/* Prepare for SoC power-down */
		trace_power_flow(cpu_idx, SOC_PWRDN);
	} else if (cluster_state == ARM_LOCAL_STATE_RET) {
		assert(cpu_state == PLAT_MAX_RET_STATE);
		/* Prepare for cluster retention */
		trace_power_flow(cpu_idx / CORE_PER_CLU, CLUSTER_RETEN);
	} else if (cluster_state == ARM_LOCAL_STATE_OFF) {
		assert(cpu_state == PLAT_MAX_OFF_STATE);
		/* Prepare for cluster power-down */
		trace_power_flow(cpu_idx / CORE_PER_CLU, CLUSTER_PWRDN);
	} else if (cpu_state == ARM_LOCAL_STATE_RET) {
		/* Prepare for CPU retention */
		trace_power_flow(cpu_idx, CPU_RETEN);
	} else if (cpu_state == ARM_LOCAL_STATE_OFF) {
		/* Prepare for CPU power-down */
		trace_power_flow(cpu_idx, CPU_PWRDN);
	} else {
		ERROR("%s: Unknown state id\n", __func__);
		return PSCI_E_NOT_SUPPORTED;
	}

	/*
	 * Currently don't support retention. Just return
	 * as nothing is to be done for retention.
	 */
	if (cluster_state == ARM_LOCAL_STATE_RET)
		cluster_state = ARM_LOCAL_STATE_RUN;
	if (system_state == ARM_LOCAL_STATE_RET)
		system_state = ARM_LOCAL_STATE_RUN;
	if (cpu_state == ARM_LOCAL_STATE_RET) {
		/* Enable PhysicalIRQ/FIQ bit for NS world to wake the CPU */
		write_scr_el3(scr_el3[cpu_idx] | SCR_IRQ_BIT | SCR_FIQ_BIT);
		return PSCI_E_SUCCESS;
	}

	xgene_power_down_common(cpu_idx, cpu_state, cluster_state, system_state);

	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	uint32_t cpu_state = XGENE_CORE_PWR_STATE(target_state);
	int cpu_idx = plat_my_core_pos();

	/* Restore SCR to the original value */
	write_scr_el3(scr_el3[cpu_idx]);

	/*
	 * Currently don't support retention. Just return
	 * as nothing is to be done for retention.
	 */
	if (cpu_state == ARM_LOCAL_STATE_RET)
		return PSCI_E_SUCCESS;

	xgene_clu_barrier_inc_return(cpu_idx);

	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_on(u_register_t mpidr)
{
	int cpu_idx = xgene_calc_core_pos(mpidr);

	xgene_psci_set_lpi_state(mpidr, ARM_LOCAL_STATE_RUN,
				 ARM_LOCAL_STATE_RUN, ARM_LOCAL_STATE_RUN);

	xgene_clu_barrier_inc_return(cpu_idx);

	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_off(const psci_power_state_t *target_state)
{
	assert(XGENE_CORE_PWR_STATE(target_state) == ARM_LOCAL_STATE_OFF);
	xgene_power_down_common(plat_my_core_pos(),
				ARM_LOCAL_STATE_OFF,
				XGENE_CLUSTER_PWR_STATE(target_state),
				XGENE_SYSTEM_PWR_STATE(target_state));

	return PSCI_E_SUCCESS;
}

int xgene_soc_prepare_system_reset(void)
{
	return PSCI_E_SUCCESS;
}

void xgene_soc_down_wfi(const psci_power_state_t *target_state)
{

	/* Step 9: Execute DSB */
	/* Step 10: Execute ISB */
	/* Step 11: Execute WFI */
	dsb();
	isb();
	wfi();

	if (XGENE_VHP) {
		/* Warm reset the core to wake-up */
		plat_arm_gic_cpuif_disable();
		xgene_plat_core_reset(plat_my_core_pos());
	}
}
