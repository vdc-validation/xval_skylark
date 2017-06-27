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
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <xgene_fwu.h>
#include <xgene_private.h>

extern unsigned long __RW_START__;
extern unsigned long __RW_END__;
extern unsigned long __RO_START__;
extern unsigned long __RO_END__;

/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define NS_BL1U_RO_BASE (unsigned long)(&__RO_START__)
#define NS_BL1U_RO_LIMIT (unsigned long)(&__RO_END__)
#define NS_BL1U_END (unsigned long)(&__RW_END__)

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define NS_BL1U_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define NS_BL1U_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/* Data structure which holds the extents of the non secure RAM for NS_BL1U*/
static meminfo_t ns_bl1u_dram_layout;

/* Weak definitions may be overridden */
#pragma weak ns_bl1u_platform_setup
#pragma weak ns_bl1u_early_platform_setup
#pragma weak ns_bl1u_plat_arch_setup
#pragma weak plat_ns_bl1u_get_mem_layout

meminfo_t *plat_ns_bl1u_get_mem_layout(void)
{
	return &ns_bl1u_dram_layout;
}

void plat_xgene_get_fip_block_memmap(io_block_spec_t *fip_block_memmap_spec)
{
	uint64_t	fip_base;
	uint64_t	size;
	int			rc;

	rc = xgene_search_fwu_fip(&fip_base, &size);
	if (rc) {
		ERROR("NS_BL1U: Failed to load FWU FIP image.\n");
		plat_error_handler(rc);
	}

	VERBOSE("NS_BL1U: Found FWU at 0x%p\n", fip_base);
	fip_block_memmap_spec->offset = (size_t) fip_base;
	fip_block_memmap_spec->length = size;
}

/*
 *
 * Perform platform setup for NS_BL1U
 */
void xgene_ns_bl1u_platform_setup(void)
{
	/* Initialize the IO layer and register platform IO devices */
	plat_xgene_io_setup();
}

void ns_bl1u_platform_setup(void)
{
	xgene_ns_bl1u_platform_setup();
}

void xgene_ns_bl1u_early_platform_setup(meminfo_t *mem_layout, void *plat_info)
{
	/* Initialize the console to provide early debug support */
	console_init(XGENE_BOOT_NS_UART_BASE, XGENE_BOOT_UART_CLK_IN_HZ,
			XGENE_BOOT_UART_BAUDRATE);

	/* Allow NS_BL1U see the whole Non DRAM */
	ns_bl1u_dram_layout.free_base = XGENE_NS_DRAM_BASE;
	ns_bl1u_dram_layout.free_size = XGENE_NS_DRAM_SIZE;
	ns_bl1u_dram_layout.total_base = XGENE_NS_DRAM_BASE;
	ns_bl1u_dram_layout.total_size = XGENE_NS_DRAM_SIZE;

	reserve_mem(&ns_bl1u_dram_layout.free_base,
		    &ns_bl1u_dram_layout.free_size,
			NS_BL1U_BASE,
			NS_BL1U_LIMIT - NS_BL1U_BASE);

}

/*******************************************************************************
 * X0 contains the extents of the memory available to NS_BL1U
 ******************************************************************************/
void ns_bl1u_early_platform_setup(meminfo_t *mem_layout, void *plat_info)
{
	xgene_ns_bl1u_early_platform_setup(mem_layout, plat_info);
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only initializes the mmu in a quick and dirty way.
 * The memory that is used by NS_BL1U is only mapped.
 ******************************************************************************/
void xgene_ns_bl1u_plat_arch_setup(void)
{
	xgene_configure_mmu_el2(NS_BL1U_RO_BASE,
			      (NS_BL1U_END - NS_BL1U_RO_BASE),
			      NS_BL1U_RO_BASE,
			      NS_BL1U_RO_LIMIT
#if USE_COHERENT_MEM
			      ,
			      NS_BL1U_COHERENT_RAM_BASE,
			      NS_BL1U_COHERENT_RAM_LIMIT
#endif
		);
}

void ns_bl1u_plat_arch_setup(void)
{
	xgene_ns_bl1u_plat_arch_setup();
}
