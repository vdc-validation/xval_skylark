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
#include <asm_macros.S>
#include <assert_macros.S>
#include <cpu_macros.S>
#include <platform_def.h>
#include <el3_common_macros.S>

#define SOC_EFUSE_SHADOW0		SOC_EFUSE_SHADOW_BASE
#define SOC_SECURE_MAILBOX_ID		3
#define SOC_SECURE_MAILBOX_STRIDE	0x1000
#define SOC_SECURE_MAILBOX3_REG_ADDR	SMPRO_SEC_MB_REG_BASE + \
					(SOC_SECURE_MAILBOX_ID * SOC_SECURE_MAILBOX_STRIDE)
#define MASTER_PMD_INFO_SHIFT		7
#define MASTER_PMD_INFO_MASK		0x7f

	/* Global functions */
	.globl	plat_is_my_cpu_primary
	.globl	plat_my_core_pos
	.globl	plat_get_my_entrypoint
	.globl	plat_secondary_cold_boot_setup
	.globl	platform_mem_init
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_reset_handler
	.globl	plat_report_exception
	.globl	xgene_calc_core_pos
	.globl	bl1_secondary_entrypoint

	/* -----------------------------------------------------
	 * unsigned int platform_is_primary_cpu();
	 *
	 * This function checks if this is the Primary CPU
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	/* Get core position */
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	lsr	x0, x0, #MPIDR_AFFINITY_BITS
	mov	x2, #PLATFORM_MAX_CPUS_PER_CLUSTER
	mul	x0, x0, x2
	add	x0, x1, x0
#if !XGENE_VHP
	/* bits[13:7] of mailbox 3 indicates master pmd no */
	ldr	x1, =SOC_SECURE_MAILBOX3_REG_ADDR
	ldr	w1, [x1]
	/* read the bits[13:7] of mailbox 3 */
	lsr	w2, w1, #MASTER_PMD_INFO_SHIFT
	and	w2, w2, #MASTER_PMD_INFO_MASK
	/* primary_core = master_pmd * PLATFORM_MAX_CPUS_PER_CLUSTER */
	mov 	w3, #PLATFORM_MAX_CPUS_PER_CLUSTER
	mul	w2, w2, w3
	cmp	w0, w2
#else
	cmp	x0, #PLATFORM_PRIMARY_CPU
#endif
	cset	w0, eq
	ret
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * unsigned int xgene_calc_core_pos(int mpidr);
	 *
	 * With this function: CorePos =
	 *  (ClusterId * PLATFORM_MAX_CPUS_PER_CLUSTER) + CoreId
	 * -----------------------------------------------------
	 */
func xgene_calc_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	lsr x0, x0, #MPIDR_AFFINITY_BITS
	mov x2, #PLATFORM_MAX_CPUS_PER_CLUSTER
	mul x0, x0, x2
	add	x0, x1, x0
	ret
endfunc xgene_calc_core_pos

	/* -----------------------------------------------------
	 * unsigned int plat_my_core_pos(void);
	 *
	 * result: CorePos =
	 *  (ClusterId * PLATFORM_MAX_CPUS_PER_CLUSTER) + CoreId
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b xgene_calc_core_pos
endfunc plat_my_core_pos

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset. Right
	 * now this is a stub function.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
secondary_wait:
	bl	plat_get_my_entrypoint
	cbz	x0, 1f
	br	x0
1:
	wfi
	b	secondary_wait
endfunc plat_secondary_cold_boot_setup

	/* -----------------------------------------------------
	 * void platform_get_entrypoint (void);
	 *
	 * Main job of this routine is to distinguish between
	 * a cold and warm boot. If the sec_entry_point for
	 * this CPU is present, then it's a warm boot.
	 *
	 * -----------------------------------------------------
	 */
func plat_get_my_entrypoint
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	lsr x0, x0, #MPIDR_AFFINITY_BITS
	mov x2, #PLATFORM_MAX_CPUS_PER_CLUSTER
	mul x0, x0, x2
	add	x0, x1, x0
	ldr	x1, =PLAT_XGENE_TRUSTED_MAILBOX_BASE
	ldr	x0, [x1, x0, lsl #3]
	ret
endfunc plat_get_my_entrypoint

	/* --------------------------------------------------------
	 * void platform_mem_init (void);
	 *
	 * Any memory init, relocation to be done before the
	 * platform boots. Called very early in the boot process.
	 * --------------------------------------------------------
	 */
func platform_mem_init
	mov	x0, #0
	ret
endfunc platform_mem_init

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, XGENE_BOOT_UART_BASE
	mov_imm	x1, XGENE_BOOT_UART_CLK_IN_HZ
	mov_imm	x2, XGENE_BOOT_UART_BAUDRATE
	b	console_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(void)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, XGENE_BOOT_UART_BASE
	b	console_core_putc
endfunc plat_crash_console_putc

	/* ---------------------------------------------------
	 * Function to handle a platform reset and store
	 * input parameters passed by BL2.
	 * ---------------------------------------------------
	 */
func plat_reset_handler
	ret
endfunc plat_reset_handler

	/* ---------------------------------------------
	 * void plat_report_exception(unsigned int type)
	 * Function to report an unhandled exception
	 * with platform-specific means.
	 * ---------------------------------------------
	 */
func plat_report_exception
	ret
endfunc plat_report_exception

	/* -----------------------------------------------------
	 * bl1_secondary_entrypoint() is the entry point into the BL1
	 * code when a secondary cpu is released from cold reset.
	 * -----------------------------------------------------
	 */
func bl1_secondary_entrypoint
	/*
	 * On the secondary core, most of the EL3 initialisations performed by
	 * 'el3_entrypoint_common' must be skipped:
	 *
	 *  - Only when the platform bypasses the BL1/BL31 entrypoint by
	 *    programming the reset address do we need to set the CPU endianness.
	 *    In other cases, we assume this has been taken care by the
	 *    entrypoint code.
	 *
	 *  - No need to determine the type of boot.
	 *
	 *  - Do not try to distinguish between primary and secondary CPUs.
	 *
	 *  - No need to initialise the memory or the C runtime environment,
	 *    it has been done once and for all on the master core boot path.
	 */

	el3_entrypoint_common					\
		_set_endian=PROGRAMMABLE_RESET_ADDRESS		\
		_warm_boot_mailbox=0				\
		_secondary_cold_boot=0				\
		_init_memory=0					\
		_init_c_runtime=0				\
		_exception_vectors=bl1_exceptions

	bl	bl1_secondary_main

endfunc	bl1_secondary_entrypoint
