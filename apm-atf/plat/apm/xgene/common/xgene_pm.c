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
#include <bl_common.h>
#include <context.h>
#include <context_mgmt.h>
#include <debug.h>
#include <memctrl.h>
#include <mmio.h>
#include <plat_arm.h>
#include <plat_psci_mbox.h>
#include <platform.h>
#include <psci.h>
#include <platform_def.h>
#include <xgene_pm_tracer.h>
#include <xgene_private.h>
#include <gicv3.h>

/* Macros to read the power domain state */
#define XGENE_CORE_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define XGENE_CLUSTER_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define XGENE_SYSTEM_PWR_STATE(state) \
	((PLAT_MAX_PWR_LVL > MPIDR_AFFLVL1) ?\
			(state)->pwr_domain_state[MPIDR_AFFLVL2] : 0)

static uint64_t *xgene_trusted_mailboxes;
static uint64_t xgene_store_sec_entry_point;
static uint32_t pmd_barrier[PLATFORM_CLUSTER_COUNT];
DEFINE_BAKERY_LOCK(clu_locks[PLATFORM_CLUSTER_COUNT]);

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that will be overridden by a SoC.
 */
#pragma weak xgene_soc_pwr_domain_suspend
#pragma weak xgene_soc_pwr_domain_on
#pragma weak xgene_soc_pwr_domain_off
#pragma weak xgene_soc_pwr_domain_on_finish
#pragma weak xgene_soc_pwr_domain_suspend_finish
#pragma weak xgene_soc_prepare_system_reset
#pragma weak xgene_soc_down_wfi

int xgene_soc_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	return PSCI_E_NOT_SUPPORTED;
}

int xgene_soc_pwr_domain_on(u_register_t mpidr)
{
	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_off(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int xgene_soc_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int xgene_soc_prepare_system_reset(void)
{
	return PSCI_E_SUCCESS;
}

void xgene_soc_down_wfi(const psci_power_state_t *target_state)
{
	dsb();
	wfi();
}

uint32_t xgene_clu_barrier_inc_return(uint32_t cpu)
{
	uint32_t ret = 0;

	bakery_lock_get(&clu_locks[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER]);
	ret = ++pmd_barrier[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER];
	bakery_lock_release(&clu_locks[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER]);

	return ret;
}

uint32_t xgene_clu_barrier_dec_return(uint32_t cpu)
{
	uint32_t ret = 0;

	bakery_lock_get(&clu_locks[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER]);
	ret = --pmd_barrier[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER];
	bakery_lock_release(&clu_locks[cpu / PLATFORM_MAX_CPUS_PER_CLUSTER]);

	return ret;
}

/*******************************************************************************
 * This handler is called by the PSCI implementation during the `SYSTEM_SUSPEND`
 * call to get the `power_state` parameter. This allows the platform to encode
 * the appropriate State-ID field within the `power_state` parameter which can
 * be utilized in `pwr_domain_suspend()` to suspend to system affinity level.
******************************************************************************/
void xgene_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	/* lower affinities use PLAT_MAX_OFF_STATE */
	for (int i = MPIDR_AFFLVL0; i < PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;

	/* max affinity uses system suspend state id */
	req_state->pwr_domain_state[PLAT_MAX_PWR_LVL] = ARM_LOCAL_STATE_OFF;
}

/*******************************************************************************
 * Handler called when an affinity instance is about to enter standby.
 ******************************************************************************/
void xgene_cpu_standby(plat_local_state_t cpu_state)
{
	unsigned int scr;

	trace_power_flow(plat_my_core_pos(), CPU_STANDBY);

	scr = read_scr_el3();
	/*
	 * Enable PhysicalIRQ/FIQ bit for NS world to wake the CPU
	 */
	write_scr_el3(scr | SCR_IRQ_BIT | SCR_FIQ_BIT);

	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	dsb();
	wfi();

	/*
	 * Restore SCR to the original value
	 */
	write_scr_el3(scr);
}

/*******************************************************************************
 * Handler called when an affinity instance is about to be turned on. The
 * level and mpidr determine the affinity instance.
 ******************************************************************************/
int xgene_pwr_domain_on(u_register_t mpidr)
{
	int cpu_idx = xgene_calc_core_pos(mpidr);

	trace_power_flow(cpu_idx, CPU_ON);

	xgene_plat_program_trusted_mailbox(cpu_idx, xgene_store_sec_entry_point);

	/* Set PSCR for primary core */
	plat_xgene_set_pscr();

	return xgene_soc_pwr_domain_on(mpidr);
}

/*******************************************************************************
 * Handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void xgene_pwr_domain_off(const psci_power_state_t *target_state)
{
	trace_power_flow(plat_my_core_pos(), CPU_OFF);

	/* Prevent interrupts from spuriously waking up this cpu */
	plat_arm_gic_cpuif_disable();

	xgene_soc_pwr_domain_off(target_state);
}

/*******************************************************************************
 * Handler called when called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void xgene_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	xgene_soc_pwr_domain_suspend(target_state);
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
void xgene_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/*
	 * Reset hardware settings.
	 */
	xgene_soc_pwr_domain_on_finish(target_state);

	/* Enable the gic cpu interface */
	plat_arm_gic_pcpu_init();

	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_arm_gic_cpuif_enable();

	/* Program PSCR optimized value if located in NVRAM */
	plat_xgene_set_pscr();
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 ******************************************************************************/
void xgene_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	/* Return as nothing is to be done on waking up from retention. */
	if (XGENE_CORE_PWR_STATE(target_state) == ARM_LOCAL_STATE_RET)
		return;

	/* Perform system domain restore if woken up from system suspend */
	if (XGENE_CORE_PWR_STATE(target_state) == ARM_LOCAL_STATE_OFF) {
		/* Enable the gic cpu interface */
		plat_arm_gic_cpuif_enable();
	}

	xgene_soc_pwr_domain_suspend_finish(target_state);
}

/*******************************************************************************
 * Handler called when the system wants to be powered off
 ******************************************************************************/
__dead2 void xgene_pwr_domain_down_wfi(const psci_power_state_t *target_state)
{
	xgene_soc_down_wfi(target_state);
	panic();
}

__dead2 void xgene_system_off(void)
{
	trace_power_flow(plat_my_core_pos(), SYSTEM_OFF);

	xgene_psci_set_power_state(SYS_SHUTDOWN);

	/*
	 * Due to fact that OS running in NS-EL1 doesn't
	 * disable few interrupts like NS-EL1 timer before entering
	 * EL3, this may still wake up cpu from wfi
	 * To prevent this interrupts from spuriously waking up this cpu,
	 * disable the gic cpu interface.
	 */
	plat_arm_gic_cpuif_disable();

	wfi();

	ERROR("Interrupt after system shutdown request\n");

	panic();
}

/*******************************************************************************
 * Handler called when the system wants to be restarted.
 ******************************************************************************/
__dead2 void xgene_system_reset(void)
{
	trace_power_flow(plat_my_core_pos(), SYSTEM_RESET);

	/* per-SoC system reset handler */
	xgene_soc_prepare_system_reset();

	xgene_psci_set_power_state(SYS_REBOOT);

	/*
	 * Due to fact that OS running in NS-EL1 doesn't
	 * disable few interrupts like NS-EL1 timer before entering
	 * EL3, this may still wake up cpu from wfi
	 * To prevent this interrupts from spuriously waking up this cpu,
	 * disable the gic cpu interface.
	 */
	plat_arm_gic_cpuif_disable();

	wfi();

	ERROR("Interrupt after system reboot request\n");

	panic();
}

/*******************************************************************************
 * Handler called to check the validity of the power state parameter.
 ******************************************************************************/
int32_t xgene_validate_power_state(unsigned int power_state,
				   psci_power_state_t *req_state)
{
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	return xgene_soc_validate_power_state(power_state, req_state);
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure entrypoint.
 ******************************************************************************/
int xgene_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * TODO: Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Export the platform handlers to enable psci to invoke them
 ******************************************************************************/
static const plat_psci_ops_t xgene_plat_psci_ops = {
	.cpu_standby			= xgene_cpu_standby,
	.pwr_domain_on			= xgene_pwr_domain_on,
	.pwr_domain_off			= xgene_pwr_domain_off,
	.pwr_domain_suspend		= xgene_pwr_domain_suspend,
	.pwr_domain_on_finish		= xgene_pwr_domain_on_finish,
	.pwr_domain_suspend_finish	= xgene_pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi	= xgene_pwr_domain_down_wfi,
	.system_off			= xgene_system_off,
	.system_reset			= xgene_system_reset,
	.validate_power_state		= xgene_validate_power_state,
	.validate_ns_entrypoint		= xgene_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= xgene_get_sys_suspend_power_state,
};

/*******************************************************************************
 * Export the platform specific power ops and initialize Power Controller
 ******************************************************************************/
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	psci_power_state_t target_state = { { PSCI_LOCAL_STATE_RUN } };
	int cpu_idx;

	xgene_trusted_mailboxes = (uint64_t *)PLAT_XGENE_TRUSTED_MAILBOX_BASE;
	xgene_store_sec_entry_point = sec_entrypoint;

	xgene_trusted_mailboxes[0] = xgene_store_sec_entry_point;

	for (cpu_idx = 1; cpu_idx < PLATFORM_CORE_COUNT; cpu_idx++)
		xgene_trusted_mailboxes[cpu_idx] = 0;

	/*
	 * Reset hardware settings.
	 */
	xgene_soc_pwr_domain_on_finish(&target_state);

	/*
	 * Initialize PSCI ops struct
	 */
	*psci_ops = &xgene_plat_psci_ops;

	xgene_clu_barrier_inc_return(0);

	return 0;
}
