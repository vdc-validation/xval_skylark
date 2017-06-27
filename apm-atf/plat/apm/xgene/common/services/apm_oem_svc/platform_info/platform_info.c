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
#include <apm_oem_svc.h>
#include <clk.h>
#include <debug.h>
#include <errno.h>
#include <l3c.h>
#include <mmio.h>
#include <platform_info.h>
#include <string.h>
#include <xgene_nvparam.h>

#if XGENE_VHP
#define SKYLARK_CPU_COUNT			4
#endif

#define SOC_EFUSE_SHADOW0			SOC_EFUSE_SHADOW_BASE
#define SOC_EFUSE_SHADOWn(n) \
	(SOC_EFUSE_SHADOW_BASE + (n) * 4)

/* DVFS enable bit is in EFUSE2 register */
#define EFUSE_DVFS_ENABLE(efuse)		(((efuse) & 0x04000000) >> 26)
/* Turbo enable bit is in EFUSE7 register */
#define EFUSE_TURBO_ENABLE(efuse)		(((efuse) & 0x00020000) >> 17)
/* Turbo frequency bit is in EFUSE8 register */
#define EFUSE_TURBO_FREQ(efuse)			(((efuse) & 0x00000FC0) >> 6)

#define MIDR_EL1_REV_MASK			0x0000000F
#define MIDR_EL1_VARIANT_MASK			0x00F00000
#define PIRANHA_VARIANT_ID			0x00300000

#define MAX_NAME_STRING				50
#define CPU_PIRANHA_INFO_STR			"APM ARM 64-bit Piranha"
#define CPU_VER_STR_A0				"A0"

#define TURBO_DEFAULT_FREQ			3300000000	/* 3.3 GHz */

#define SIZE_32KB (32 * 1024)
#define SIZE_256KB (256 * 1024)

enum {
	PLATFORM_INFO_FUNC_PCP_CLK = 0,
	PLATFORM_INFO_FUNC_PMD_CLK,
	PLATFORM_INFO_FUNC_SOC_CLK,
	PLATFORM_INFO_FUNC_AHB_CLK,
	PLATFORM_INFO_FUNC_AXI_CLK,
	PLATFORM_INFO_FUNC_IOBAXI_CLK,
	PLATFORM_INFO_FUNC_CPU_INFO,
	PLATFORM_INFO_FUNC_CPU_VER,
	PLATFORM_INFO_FUNC_CPU_ID,
	PLATFORM_INFO_FUNC_CPU_COUNT,
	PLATFORM_INFO_FUNC_PMD_IS_ENABLED,
	PLATFORM_INFO_FUNC_TURBO_FREQ,
	PLATFORM_INFO_FUNC_DVFS_IS_SUPPORTED,
	PLATFORM_INFO_FUNC_APB_CLK,
};

static void platform_info_get_cpu_info_str(uint8_t *buf, uint32_t len)
{
	uint32_t val, tmp_len;

	if (len == 0)
		return;

	buf[0] = '\0';

	val = read_midr();
	switch (val & MIDR_EL1_VARIANT_MASK) {
	case PIRANHA_VARIANT_ID:
		if (len < strlen(CPU_PIRANHA_INFO_STR) + 1)
			tmp_len = len;
		else
			tmp_len = strlen(CPU_PIRANHA_INFO_STR);
		memcpy((void *) buf, (void *) CPU_PIRANHA_INFO_STR, tmp_len);
		buf[tmp_len] = '\0';
		break;
	}
}

static void platform_info_get_cpu_ver_str(uint8_t *buf, uint32_t len)
{
	uint32_t val, tmp_len;

	if (len == 0)
		return;

	buf[0] = '\0';

	val = read_midr();
	switch (val & MIDR_EL1_VARIANT_MASK) {
	case PIRANHA_VARIANT_ID:
		if ((val & MIDR_EL1_REV_MASK) == 1) {
			/* A0 */
			if (len < strlen(CPU_VER_STR_A0) + 1)
				tmp_len = len;
			else
				tmp_len = strlen(CPU_VER_STR_A0);
			memcpy((void *) buf, (void *) CPU_VER_STR_A0, tmp_len);
			buf[tmp_len] = '\0';
		}
		break;
	}
}

static int platform_pmd_is_enabled(uint32_t pmd)
{
	uint32_t val;
	int ret;

	if (pmd >= PLATFORM_CLUSTER_COUNT)
		return 0;

#if !XGENE_VHP
	if (mmio_read_32(SOC_EFUSE_SHADOW0) & (1 << pmd))
		return 0;

	ret = plat_nvparam_get(NV_PCP_ACTIVEPMD, NV_PERM_ATF, &val);
	if (!ret) {
		if (!(val & (1 << pmd)))
			return 0;
	}
#else
	if (pmd >= SKYLARK_CPU_COUNT / PLATFORM_MAX_CPUS_PER_CLUSTER)
		return 0;
#endif
	/* PMD is enabled */
	return 1;
}

static int platform_get_num_of_cores(void)
{
	uint32_t pmd;
	uint32_t count;

	count = 0;
	for (pmd = 0; pmd < PLATFORM_CLUSTER_COUNT; pmd++) {
		if (platform_pmd_is_enabled(pmd))
			count++;
	}

	return count * PLATFORM_MAX_CPUS_PER_CLUSTER;
}

static uint64_t platform_info_get_turbo_freq(void)
{
#if PLATFORM_EFUSE_CPU
	uint32_t efuse;

	efuse = mmio_read_32(SOC_EFUSE_SHADOWn(7));
	if (EFUSE_TURBO_ENABLE(efuse)) {
		efuse = mmio_read_32(SOC_EFUSE_SHADOWn(8));
		if (EFUSE_TURBO_FREQ(efuse))
			return EFUSE_TURBO_FREQ(efuse) * 100 * 1000000;
		else
			return TURBO_DEFAULT_FREQ;
	}

	return 0;
#else
	return TURBO_DEFAULT_FREQ;
#endif
}

static int platform_info_get_dvfs_support(void)
{
	uint32_t efuse;

	efuse = mmio_read_32(SOC_EFUSE_SHADOWn(2));

	return EFUSE_DVFS_ENABLE(efuse);
}

static uint64_t platform_info_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	uint8_t *buf;
	uint32_t pmd;
	uint32_t length;

	switch (x1) {
	case PLATFORM_INFO_FUNC_PCP_CLK:
		SMC_RET2(handle, 0, get_pcppll_clk());
		break;
	case PLATFORM_INFO_FUNC_PMD_CLK:
		SMC_RET2(handle, 0, get_pmdpll_clk());
		break;
	case PLATFORM_INFO_FUNC_SOC_CLK:
		SMC_RET2(handle, 0, get_socpll_clk());
		break;
	case PLATFORM_INFO_FUNC_AHB_CLK:
		SMC_RET2(handle, 0, get_ahb_clk());
		break;
	case PLATFORM_INFO_FUNC_APB_CLK:
		SMC_RET2(handle, 0, get_apb_clk());
		break;
	case PLATFORM_INFO_FUNC_AXI_CLK:
		SMC_RET2(handle, 0, get_axi_clk());
		break;
	case PLATFORM_INFO_FUNC_IOBAXI_CLK:
		SMC_RET2(handle, 0, get_iobaxi_clk());
		break;
	case PLATFORM_INFO_FUNC_CPU_INFO:
		buf = (uint8_t *) x2;
		length = (uint32_t) x3;
		platform_info_get_cpu_info_str(buf, length);
		flush_dcache_range((uint64_t) buf, length);
		SMC_RET1(handle, 0);
		break;
	case PLATFORM_INFO_FUNC_CPU_VER:
		buf = (uint8_t *) x2;
		length = (uint32_t) x3;
		platform_info_get_cpu_ver_str(buf, length);
		flush_dcache_range((uint64_t) buf, length);
		SMC_RET1(handle, 0);
		break;
	case PLATFORM_INFO_FUNC_CPU_ID:
		SMC_RET2(handle, 0, read_midr());
		break;
	case PLATFORM_INFO_FUNC_CPU_COUNT:
		SMC_RET2(handle, 0, platform_get_num_of_cores());
		break;
	case PLATFORM_INFO_FUNC_PMD_IS_ENABLED:
		pmd = (uint32_t) x2;
		SMC_RET2(handle, 0, platform_pmd_is_enabled(pmd));
		break;
	case PLATFORM_INFO_FUNC_TURBO_FREQ:
		SMC_RET2(handle, 0, platform_info_get_turbo_freq());
		break;
	case PLATFORM_INFO_FUNC_DVFS_IS_SUPPORTED:
		SMC_RET2(handle, 0, platform_info_get_dvfs_support());
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}

void svc_platform_info_register(void)
{
	int ret;

	ret = smc64_oem_svc_register(SMC_PLATFORM_INFO, &platform_info_handler);
	if (!ret)
		INFO("Platform Info service registered\n");
}
