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

#ifndef __XGENE_PRIVATE_H__
#define __XGENE_PRIVATE_H__

#include <io_storage.h>
#include <platform_def.h>
#include <psci.h>
#include <xlat_tables.h>

#define XGENE_BL31_PLAT_PARAM_VAL		0x0f1e2d3c4b5a6978ULL

/* Declarations for plat_psci_handlers.c */
int32_t xgene_soc_validate_power_state(unsigned int power_state,
					psci_power_state_t *req_state);

/* Atomic increase and return cluster barrier value */
uint32_t xgene_clu_barrier_inc_return(uint32_t cpu);

/* Atomic decrease and return cluster barrier value */
uint32_t xgene_clu_barrier_dec_return(uint32_t cpu);

/* Get logical position cpu id from mpidr */
unsigned int xgene_calc_core_pos(u_register_t mpidr);

/* Declarations for xgene_bl31_setup.c */
uint32_t xgene_get_spsr_for_bl32_entry(void);
uint32_t xgene_get_spsr_for_bl33_entry(void);

/* Declarations for xgene_delay_timer.c */
void xgene_delay_timer_init(void);

/* Declarations for xgene_io_storage.c */
void plat_xgene_get_fip_block_memmap(io_block_spec_t *fip_block_memmap_spec);
void plat_xgene_io_setup(void);
int plat_xgene_get_alt_image_source(
			unsigned int image_id,
			uintptr_t *dev_handle,
			uintptr_t *image_spec);

void xgene_configure_mmu_el1(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit
#if USE_COHERENT_MEM
			, unsigned long coh_start,
			unsigned long coh_limit
#endif
);

void xgene_configure_mmu_el2(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit
#if USE_COHERENT_MEM
			, unsigned long coh_start,
			unsigned long coh_limit
#endif
);

void xgene_configure_mmu_el3(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit
#if USE_COHERENT_MEM
			, unsigned long coh_start,
			unsigned long coh_limit
#endif
);

/*
 * Program CPU trusted mailbox
 */
void xgene_plat_program_trusted_mailbox(uint32_t cpu_idx, uintptr_t address);

/*
 * Reset a core
 *
 * It issues a warm reset on a core
 */
void xgene_plat_core_reset(uint32_t cpu_idx);

/*
 * Reset all core(s)
 *
 * Issue a warm reset to all cores (including itself)
 */
void xgene_plat_cores_reset(void);

/*
 * Get the PSCR config information from NVParam
 */
void plat_xgene_init_pscr(void);

/*
 * Set the PSCR config
 */
void plat_xgene_set_pscr(void);

/*
 * X-Gene platform setup
 *
 * This function initializes the X-Gene platform
 */
void xgene_plat_setup(void);

/*
 * Board setup
 *
 * This function initializes the board
 */
void xgene_board_init(void);

/*
 * The entry point into the BL1 code when a secondary cpu is released from reset
 */
void bl1_secondary_entrypoint(void);

/*
 * Take a secondary core ON and run core_hdlr() function
 *
 * It takes secondary core online into BL1 with stack size defined by
 * BL1_SECONDARY_STACK_SIZE. Secondary executes core_hdlr() function
 * then comes back to offline.
 */
int bl1_plat_core_on(unsigned int cpu, void (*core_hdlr)(void));

#endif /* __XGENE_PRIVATE_H__ */
