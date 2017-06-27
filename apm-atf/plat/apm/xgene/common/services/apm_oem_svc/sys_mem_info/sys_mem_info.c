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
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
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
#include <apm_oem_svc.h>
#include <config.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <sys_mem_info.h>
#include <xgene_dimm_info.h>
#include <xgene_nvparam.h>

#define NR_MEM_REGIONS	4

struct xgene_plat_dram_info {
	uint32_t nr_regions;
	uint64_t total_size;
	uint64_t base[NR_MEM_REGIONS];
	uint64_t size[NR_MEM_REGIONS];
	uint32_t cur_speed;
};

enum {
	MEM_FUNC_GET_NR_REGIONS = 0,
	MEM_FUNC_GET_TOTAL_SIZE,
	MEM_FUNC_GET_REGION_MEM,
	MEM_FUNC_GET_DRAM_SPEED, /* in MHz */
	MEM_FUNC_GET_DRAM_SPEED_CONFIG,
	MEM_FUNC_SET_DRAM_SPEED_CONFIG,
	MEM_FUNC_DRAM_SCRUB_STATUS,
	MEM_FUNC_DRAM_SCRUB_SET,
	MEM_FUNC_DIMM_GET_INFO,
	MEM_FUNC_DIMM_GET_SPD_DATA,
};

/* This structure used by SMC caller to parse DIMM information */
#define xgene_svc_dimm_infoV1 xgene_plat_dimm_info
#define xgene_mem_spd_dataV1 xgene_mem_spd_data

static uint64_t mem_svc_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	struct xgene_plat_dram_info *meminfo;
	struct xgene_plat_dimm_list *dimmlist;
	uint8_t *buf;
	uint32_t idx, val32, length, version;
	int ret;

	meminfo = (struct xgene_plat_dram_info *) PLAT_XGENE_MEMINFO_BASE;
	dimmlist = (struct xgene_plat_dimm_list *) PLAT_XGENE_DIMMINFO_BASE;

	switch (x1) {
	case MEM_FUNC_GET_NR_REGIONS:
		SMC_RET2(handle, 0, (uint64_t) meminfo->nr_regions);
		break;
	case MEM_FUNC_GET_TOTAL_SIZE:
		SMC_RET2(handle, 0, meminfo->total_size);
		break;
	case MEM_FUNC_GET_REGION_MEM:
		idx = x2;
		if (idx >= meminfo->nr_regions)
			SMC_RET1(handle, -EINVAL);
		SMC_RET3(handle, 0, meminfo->base[idx], meminfo->size[idx]);
		break;
	case MEM_FUNC_GET_DRAM_SPEED:
		SMC_RET2(handle, 0, meminfo->cur_speed);
		break;
	case MEM_FUNC_GET_DRAM_SPEED_CONFIG:
		ret = plat_nvparam_get(NV_DRAM_SPEED, NV_PERM_ATF, &val32);
		if (ret)
			SMC_RET1(handle, -EPERM);
		SMC_RET2(handle, 0, val32);
		break;
	case MEM_FUNC_SET_DRAM_SPEED_CONFIG:
		val32 = (uint32_t) x2;
		ret = plat_nvparam_set(NV_DRAM_SPEED, NV_PERM_ATF | NV_PERM_MANU,
				NV_PERM_ATF | NV_PERM_MANU, val32);
		if (ret)
			SMC_RET1(handle, -EPERM);
		SMC_RET1(handle, 0);
		break;
	case MEM_FUNC_DRAM_SCRUB_STATUS:
		/* TODO: DDR scrub setting */
		SMC_RET2(handle, 0, 1);
		break;
	case MEM_FUNC_DRAM_SCRUB_SET:
		/* TODO: DDR scrub setting */
		SMC_RET1(handle, 0);
		break;
	case MEM_FUNC_DIMM_GET_INFO:
		/* Lower 32 bit is index value */
		idx = (uint32_t) x2;
		/* Upper 32 bit is version number */
		version = (uint32_t) (x2 >> 32);
		buf = (uint8_t *) x3;
		length = (uint32_t) x4;
		/* Currently only accept version 1 */
		if (idx >= dimmlist->num_slot || version != 1 ||
				length < sizeof(struct xgene_svc_dimm_infoV1))
			SMC_RET1(handle, -EINVAL);
		memcpy(buf,
			&(dimmlist->dimm[idx].info),
			sizeof(struct xgene_svc_dimm_infoV1));
		flush_dcache_range((uint64_t) buf,
			sizeof(struct xgene_svc_dimm_infoV1));
		SMC_RET1(handle, 0);
		break;
	case MEM_FUNC_DIMM_GET_SPD_DATA:
		/* Lower 32 bit is index value */
		idx = (uint32_t) x2;
		/* Upper 32 bit is version number */
		version = (uint32_t) (x2 >> 32);
		buf = (uint8_t *) x3;
		length = (uint32_t) x4;
		/* Currently only accept version 1 */
		if (idx >= dimmlist->num_slot || version != 1 ||
			length < sizeof(struct xgene_mem_spd_dataV1) ||
			dimmlist->dimm[idx].info.dimm_status != DIMM_INSTALLED)
			SMC_RET1(handle, -EINVAL);
		memcpy(buf,
			&(dimmlist->dimm[idx].spd_data),
			sizeof(struct xgene_mem_spd_dataV1));
		flush_dcache_range((uint64_t) buf,
			sizeof(struct xgene_mem_spd_dataV1));
		SMC_RET1(handle, 0);
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}

void mem_svc_register(void)
{
	int ret;

	ret = smc64_oem_svc_register(SMC_MEM, &mem_svc_handler);
	if (!ret)
		INFO("Memory service registered\n");
}
