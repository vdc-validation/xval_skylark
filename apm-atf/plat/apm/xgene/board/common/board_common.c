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

#include <platform_def.h>
#include <xlat_tables.h>

/*
 * Table of regions for different BL stages to map using the MMU.
 * This doesn't include Trusted RAM as the 'mem_layout' argument passed to
 * arm_configure_mmu_elx() will give the available subset of that,
 */
#if IMAGE_BL1
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_BL_SHARED_RAM,
	XGENE_MAP_DEVICE_MMIO,
	XGENE_MAP_NS_DRAM,
	{0}
};
#endif
#if IMAGE_BL2
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_BL_SHARED_RAM,
	XGENE_MAP_DEVICE_MMIO,
	XGENE_MAP_NS_DRAM,
#if BL32_BASE
        XGENE_MAP_SEC_MEM,
#endif
	{0}
};
#endif
#if IMAGE_BL2U
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_BL_SHARED_RAM,
	XGENE_MAP_DEVICE_MMIO,
	XGENE_MAP_NS_DRAM,
	{0}
};
#endif
#if IMAGE_BL31
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_BL_SHARED_RAM,
	XGENE_MAP_DEVICE_MMIO,
	XGENE_MAP_NS_DRAM,
	XGENE_MAP_DRAM_REGIONB,
	XGENE_MAP_DRAM_REGIONC,
	XGENE_MAP_DRAM_REGIOND,
	{0}
};
#endif
#if IMAGE_BL32
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_BL_SHARED_RAM,
	XGENE_MAP_DEVICE_MMIO,
	{0}
};
#endif
#if IMAGE_BL1U_NS
const mmap_region_t plat_xgene_mmap[] = {
	XGENE_MAP_DEVICE_MMIO,
	XGENE_MAP_NS_DRAM,
	{0}
};
#endif

/*
 * Board specify initialization
 *
 * The default implementation is do nothing. A board specific requirement
 * can override this weak function if required.
 */
#pragma weak xgene_board_init
void xgene_board_init(void)
{
}
