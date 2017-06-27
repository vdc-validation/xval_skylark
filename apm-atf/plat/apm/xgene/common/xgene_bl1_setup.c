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
#include <ahbc.h>
#include <apm_ddr.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <errno.h>
#include <memctrl.h>
#include <platform.h>
#include <plat_arm.h>
#include <platform_def.h>
#include <spi_flash.h>
#include <tbbr_img_def.h>
#include <xgene_fip_helper.h>
#include <xgene_fwu.h>
#include <xgene_private.h>
#include "../../../../bl1/bl1_private.h"
#include <smpro_misc.h>

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL1_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL1_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/* Secondary core handler callback function prototype */
typedef void (*secondary_core_hdlr_t)(void);

/* Weak definitions may be overridden in specific ARM standard platform */
#pragma weak bl1_early_platform_setup
#pragma weak bl1_plat_arch_setup
#pragma weak bl1_platform_setup
#pragma weak bl1_plat_sec_mem_layout

/* Data structure which holds the extents of the trusted SRAM for BL1*/
static meminfo_t bl1_tzram_layout;

/* Secondary core handler callback function */
static secondary_core_hdlr_t secondary_core_hdlr[PLATFORM_CORE_COUNT];

/*******************************************************************************
 * Take a secondary core start at an entry point
 ******************************************************************************/
int bl1_plat_core_on(unsigned int cpu, void (*core_hdlr)(void))
{
	if (secondary_core_hdlr[cpu])
		return -EBUSY;

	secondary_core_hdlr[cpu] = core_hdlr;
	flush_dcache_range((uint64_t)&secondary_core_hdlr[cpu],
			   sizeof(uint64_t));

	xgene_plat_program_trusted_mailbox(cpu, (uintptr_t)bl1_secondary_entrypoint);
	xgene_plat_core_reset(cpu);

	return 0;
}

/*******************************************************************************
 * Take a secondary core offline
 ******************************************************************************/
static void bl1_plat_core_off(unsigned int cpu)
{
	secondary_core_hdlr[cpu] = 0;
	flush_dcache_range((uint64_t)&secondary_core_hdlr[cpu],
			   sizeof(uint64_t));

	xgene_plat_program_trusted_mailbox(cpu, 0);
	xgene_plat_core_reset(cpu);
}

/*
 * Only called by the secondary cpu after a cold boot to run into BL1
 */
void bl1_secondary_main(void)
{
	unsigned int cpu = plat_my_core_pos();

	/* Secondary core handler */
	if (secondary_core_hdlr[cpu])
		secondary_core_hdlr[cpu]();

	/*
	 * After finish BL1 task. Secondary core is taken
	 * back into OFF state.
	 */
	bl1_plat_core_off(cpu);
}

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

/*******************************************************************************
 * BL1 specific platform actions shared between ARM standard platforms.
 ******************************************************************************/
void xgene_bl1_early_platform_setup(void)
{
	const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;
	struct smpro_info smpro_data;
	uint32_t uart_base;

	/* Initialize AHBC block*/
	ahbc_initialize();

	/*
	 * Initialize delay timer
	 *
	 */
	xgene_delay_timer_init();

	/*
	 * The serial console assignment is as follow:
	 *
	 *  UART4 - secure entity use only
	 *  UART0 - non-secure entity use only
	 *
	 * In a production system, the UART4 is for Optee. The UART0 is for
	 * BIOS/OS. But we also have to support no production and debugging
	 * environment. It is expected that the SMpro console/CLI can be
	 * enabled. In such case, the UART4 will be dedicate to SMpro.
	 * There will be no console for ATF or Optee in secure domain. But
	 * this doesn't quite work if you need to support debugging of ATF.
	 * The solution that was discussed is to direct ATF console output
	 * to UART1 (non-secure) if the secure console is enable for SMpro.
	 * Given that this only happens for debugging and internal diagnostic,
	 * this should not be an issue.
	 *
	 * First, check if SMpro enables the console. If so, use UART1 instead.
	 */
	plat_smpro_early_get_info(&smpro_data);
	if (smpro_data.cap.cap_bits.console)
		uart_base = XGENE_BOOT_NS2_UART_BASE;
	else
		uart_base = XGENE_BOOT_UART_BASE;
	/* Initialize the console to provide early debug support */
	console_init(uart_base, XGENE_BOOT_UART_CLK_IN_HZ,
		     XGENE_BOOT_UART_BAUDRATE);

	/*
	 * Probe SPI NOR flash after console and timer initialisation
	 * SPI : depends on console, timer and ahbc init.
	 */
	spi_flash_probe();

	/* DRAM init */
	ddr_xgene_hw_init();
	/* Enable SMpro BMC DIMM scan, must be after DDR initialization */
	plat_enable_smpro_bmc_dimm_scan();

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = TZRAM_BASE;
	bl1_tzram_layout.total_size = TZRAM_SIZE;

	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = TZRAM_BASE;
	bl1_tzram_layout.free_size = TZRAM_SIZE;
	reserve_mem(&bl1_tzram_layout.free_base,
		    &bl1_tzram_layout.free_size,
		    BL1_RAM_BASE,
		    bl1_size);
}

void bl1_early_platform_setup(void)
{
	xgene_bl1_early_platform_setup();
}

/*******************************************************************************
 * Perform the very early platform specific architecture setup.
 * This only does basic initialization. Later
 * architectural setup (bl1_arch_setup()) does not do anything platform
 * specific.
 ******************************************************************************/
void xgene_bl1_plat_arch_setup(void)
{
	/* Mapping the trusted mailboxes */
	mmap_add_region(OCM_RW_DATA_BASE,
			OCM_RW_DATA_BASE,
			OCM_RW_DATA_SIZE,
			MT_MEMORY | MT_NON_CACHEABLE | MT_RW | MT_SECURE);

	xgene_configure_mmu_el3(bl1_tzram_layout.total_base,
			      bl1_tzram_layout.total_size,
			      BL1_RO_BASE,
			      BL1_RO_LIMIT
#if USE_COHERENT_MEM
			      , BL1_COHERENT_RAM_BASE,
			      BL1_COHERENT_RAM_LIMIT
#endif
			     );
}

void bl1_plat_arch_setup(void)
{
	xgene_bl1_plat_arch_setup();
}

void plat_xgene_get_fip_block_memmap(io_block_spec_t *fip_block_memmap_spec)
{
	fip_mem_info_t info;

	assert(fip_block_memmap_spec);
	if (!plat_xgene_is_recovery_mode()) {
		/* Copy FW image from non volatile mem to RAM */
		xgene_fw_slimimg_copy_to_mem();
	} else {
		NOTICE("BL1: Enter recovery mode\n");
		NOTICE("BL1: Flashing Firmware\n");
		if (xgene_flash_fw_slimimg((uint8_t *)PLAT_XGENE_FW_RAM_BASE,
				PLAT_XGENE_FW_MAX_SIZE))
			ERROR("BL1: Failed to flash Firmware\n");
	}

	fip_mem_get_info(&info);
	fip_block_memmap_spec->offset = (uint64_t)info.base;
	fip_block_memmap_spec->length = info.len;
}

/*
 * Perform the platform specific architecture setup shared.
 */
void xgene_bl1_platform_setup(void)
{
	/*
	 * Initialize any board specific
	 */
	xgene_board_init();

	/*
	 * Initialize any SOC specific
	 */
	xgene_plat_setup();

	/*
	 * Initial Memory Controller configuration.
	 */
	xgene_memctrl_setup();

	/*
	 * Initialize the IO layer and register platform IO devices
	 */
	plat_xgene_io_setup();
}

void bl1_platform_setup(void)
{
	xgene_bl1_platform_setup();
}

void bl1_plat_prepare_exit(entry_point_info_t *ep_info)
{
	/* TODO: Disable watchdog before leaving BL1 */

#ifdef EL3_PAYLOAD_BASE
	/*
	 * Program the EL3 payload's entry point address into the CPUs mailbox
	 * in order to release secondary CPUs from their holding pen and make
	 * them jump there.
	 */
	/* TODO */
	dsbsy();
	sev();
#endif
}

/*******************************************************************************
 * The following function checks if Firmware update is needed,
 * by checking if TOC in FIP image is valid or watchdog reset happened.
 ******************************************************************************/
unsigned int bl1_plat_get_next_image_id(void)
{
	if (plat_xgene_is_fw_upgrade_mode()) {
		/* TODO: Ask SMPro to load NS_BL1U to NS_BL1U_BASE address */
		return NS_BL1U_IMAGE_ID;
	}

	return BL2_IMAGE_ID;
}

/*******************************************************************************
 * Update the arg2 with address of SCP_BL2U image info.
 ******************************************************************************/
void bl1_plat_set_ep_info(unsigned int image_id,
		entry_point_info_t *ep_info)
{
	ep_info->args.arg2 = 0;

	if (image_id == BL2U_IMAGE_ID) {
		image_desc_t *image_desc = bl1_plat_get_image_desc(SCP_BL2U_IMAGE_ID);

		if (image_desc->state == IMAGE_STATE_AUTHENTICATED)
			ep_info->args.arg2 =
					(unsigned long)&image_desc->image_info;
	}
}

/*******************************************************************************
 * Wait for watchdog reset.
 ******************************************************************************/
__dead2 void bl1_plat_fwu_done(void *client_cookie, void *reserved)
{
	/* TODO: Watchdog reset */
	while (1)
		wfi();
}
