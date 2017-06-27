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
#include <bl_common.h>
#include <console.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <xgene_fwu.h>
#include <xgene_private.h>

/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define BL2U_RO_BASE (unsigned long)(&__RO_START__)
#define BL2U_RO_LIMIT (unsigned long)(&__RO_END__)

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL2U_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL2U_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/* Weak definitions may be overridden in a specific platform */
#pragma weak bl2u_platform_setup
#pragma weak bl2u_early_platform_setup
#pragma weak bl2u_plat_arch_setup
#pragma weak bl2u_plat_handle_scp_bl2u

/* Data structure which holds the SCP_BL2U image info for BL2U */
static image_info_t *scp_bl2u_image_info;

/*
 * Perform XGene standard platform setup for BL2U
 */
void xgene_bl2u_platform_setup(void)
{
	xgene_flash_firmware();
}

void bl2u_platform_setup(void)
{
	xgene_bl2u_platform_setup();
}

void xgene_bl2u_early_platform_setup(meminfo_t *mem_layout, void *plat_info)
{
	/* Initialize the console to provide early debug support */
	console_init(XGENE_BOOT_UART_BASE, XGENE_BOOT_UART_CLK_IN_HZ,
			XGENE_BOOT_UART_BAUDRATE);

	scp_bl2u_image_info = NULL;
	if (plat_info) {
		/* Should be SCP_BL2U info */
		scp_bl2u_image_info = plat_info;
	}
}

/*******************************************************************************
 * BL1 can pass platform dependent information to BL2U in x1.
 * X0 contains the extents of the memory available to BL2U
 ******************************************************************************/
void bl2u_early_platform_setup(meminfo_t *mem_layout, void *plat_info)
{
	xgene_bl2u_early_platform_setup(mem_layout, plat_info);
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only initializes the mmu in a quick and dirty way.
 * The memory that is used by BL2U is only mapped.
 ******************************************************************************/
void xgene_bl2u_plat_arch_setup(void)
{
	xgene_configure_mmu_el1(TZRAM_BASE,
			      TZRAM_SIZE,
			      BL2U_RO_BASE,
			      BL2U_RO_LIMIT
#if USE_COHERENT_MEM
			      ,
			      BL2U_COHERENT_RAM_BASE,
			      BL2U_COHERENT_RAM_LIMIT
#endif
		);
}

void bl2u_plat_arch_setup(void)
{
	xgene_bl2u_plat_arch_setup();
}

int bl2u_plat_handle_scp_bl2u(void)
{
	if (scp_bl2u_image_info)
		return xgene_flash_scp(
			(uint8_t *)scp_bl2u_image_info->image_base,
			scp_bl2u_image_info->image_size);

	return 0;
}
