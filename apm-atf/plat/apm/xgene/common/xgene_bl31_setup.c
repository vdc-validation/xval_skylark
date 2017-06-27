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
#include <assert.h>
#include <bl31.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <errno.h>
#include <l3c.h>
#include <memctrl.h>
#include <mmio.h>
#include <platform.h>
#include <stddef.h>
#include <plat_arm.h>
#include <platform_def.h>
#include <xgene_fwu.h>
#include <xgene_irq.h>
#include <xgene_private.h>
#include <spi_flash.h>
#include <smpro_misc.h>
#include <wdt.h>
#include <ras.h>

/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted SRAM
 ******************************************************************************/
extern unsigned long __RO_START__;
extern unsigned long __RO_END__;
extern unsigned long __BL31_END__;

#if USE_COHERENT_MEM
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;
#endif

/*
 * The next 3 constants identify the extents of the code, RO data region and the
 * limit of the BL3-1 image.  These addresses are used by the MMU setup code and
 * therefore they must be page-aligned.  It is the responsibility of the linker
 * script to ensure that __RO_START__, __RO_END__ & __BL31_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_RO_BASE (unsigned long)(&__RO_START__)
#define BL31_RO_LIMIT (unsigned long)(&__RO_END__)
#define BL31_END (unsigned long)(&__BL31_END__)

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL31_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

#undef L3C_ENABLE

static entry_point_info_t bl33_image_ep_info, bl32_image_ep_info;

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE)
			? &bl33_image_ep_info : &bl32_image_ep_info;

	return next_image_info;
}

/*******************************************************************************
 * Perform any BL31 specific platform actions. Populate the BL33 and BL32 image
 * info.
 ******************************************************************************/
void bl31_early_platform_setup(bl31_params_t *from_bl2,
				void *plat_params_from_bl2)
{
	struct smpro_info smpro_data;
	uint32_t uart_base;

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

	/*
	 * Configure the UART port to be used as the console
	 */
	console_init(uart_base, XGENE_BOOT_UART_CLK_IN_HZ,
		     XGENE_BOOT_UART_BAUDRATE);

	/* Initialise crash console */
	plat_crash_console_init();

	/*
	 * Check params passed from BL2 should not be NULL,
	 */
	assert(from_bl2 != NULL);
	assert(from_bl2->h.type == PARAM_BL31);
	assert(from_bl2->h.version >= VERSION_1);
	/*
	 * In debug builds, we pass a special value in 'plat_params_from_bl2'
	 * to verify platform parameters from BL2 to BL31.
	 * In release builds, it's not used.
	 */
	assert(((unsigned long long)plat_params_from_bl2) ==
		XGENE_BL31_PLAT_PARAM_VAL);

	/*
	 * Copy BL32 (if populated by BL2) and BL33 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	if (from_bl2->bl32_ep_info)
		bl32_image_ep_info = *from_bl2->bl32_ep_info;
	bl33_image_ep_info = *from_bl2->bl33_ep_info;
}

/*******************************************************************************
 * Initialize the gic, configure the SCR.
 ******************************************************************************/
void bl31_platform_setup(void)
{
	uint32_t tmp_reg;

	/*
	 * Initialize delay timer
	 */
	xgene_delay_timer_init();

	/* Set the next EL to be AArch64 */
	tmp_reg = SCR_RES1_BITS | SCR_RW_BIT;
	write_scr(tmp_reg);

	/* Initialize the gic cpu and distributor interfaces */
	plat_arm_gic_driver_init();
	plat_arm_gic_init();

	/* Initialize xgene irq handler */
	xgene_irq_init();

	/* Initialize FW upgrade */
	xgene_init_fwu();
}

/*******************************************************************************
 * Perform any BL31 platform runtime setup prior to BL31 exit
 ******************************************************************************/
void xgene_bl31_plat_runtime_setup(void)
{
	struct smpro_info smpro_data;
#if defined(L3C_ENABLE)
	uint32_t l3c_size;
#endif
	uint32_t uart_base;
	uint8_t *buf;
	int ret;

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
		uart_base = XGENE_BL31_RUN2_UART_BASE;
	else
		uart_base = XGENE_BL31_RUN_UART_BASE;

	/* Initialize the runtime console */
	console_init(uart_base, XGENE_BL31_RUN_UART_CLK_IN_HZ,
		     XGENE_BL31_RUN_UART_BAUDRATE);

	/* Configure RAS to handle Error Source interrupts */
	xgene_ras_init();

	/* Configure non-secure watch dog for handling WS1 interrupt */
	xgene_wdt_init();

	/* We need to shadow copy NVRAM used for BL33 to DDR */
	buf = (uint8_t *)
		(XGENE_NS_BL33_IMAGE_OFFSET + PLAT_XGENE_NS_NVRAM_OFFSET);
	ret = spi_flash_read(PLAT_XGENE_NS_NVRAM_OFFSET,
				PLAT_XGENE_NS_NVRAM_SIZE, buf);
	if (ret)
		ERROR("Failed to setup shadow NVRAM for BL33\n");

	/* Init and get the PSCR config used for OS */
	plat_xgene_init_pscr();
}

void bl31_plat_runtime_setup(void)
{
	xgene_bl31_plat_runtime_setup();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void xgene_bl31_plat_arch_setup(void)
{
	/* Mapping the trusted mailboxes */
	mmap_add_region(OCM_RW_DATA_BASE,
			OCM_RW_DATA_BASE,
			OCM_RW_DATA_SIZE,
			MT_MEMORY | MT_NON_CACHEABLE | MT_RW | MT_SECURE);

	xgene_configure_mmu_el3(BL31_RO_BASE,
			      (BL31_END - BL31_RO_BASE),
			      BL31_RO_BASE,
			      BL31_RO_LIMIT
#if USE_COHERENT_MEM
			      , BL31_COHERENT_RAM_BASE,
			      BL31_COHERENT_RAM_LIMIT
#endif
			      );
}

void bl31_plat_arch_setup(void)
{
	xgene_bl31_plat_arch_setup();
}
