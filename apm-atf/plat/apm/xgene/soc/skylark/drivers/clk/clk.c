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
#include <config.h>
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include <mmio.h>
#include <skylark_soc.h>
#include <skylark_ahbc_csr.h>
#include <skylark_pcp_csr.h>

#ifndef XGENE_VHP
#define XGENE_VHP	0
#endif

/* PMpro SCU register offsets */
#define SCU_PCPPLL				0x1c0
#define SCU_PMDPLL				0x1c8
#define  FBDIVC_RD(src)				((0x0000007f & (uint32_t)(src)))

/* SMpro SCU register offsets */
#define SCU_SOCPLL				0x120
#define SCU_SOCIOBAXIDIV			0x150
#define  IOBAXI_FREQ_SEL_MASK			0x0000001F
#define  IOBAXI_FREQ_SEL_RD(src)		(IOBAXI_FREQ_SEL_MASK & (uint32_t)(src))
#define  IOBAXI_FREQ_SEL_SET(dst, src) \
	(((uint32_t)(dst) & ~IOBAXI_FREQ_SEL_MASK) | \
		((uint32_t)(src) & IOBAXI_FREQ_SEL_MASK))
#define SCU_SOCAXIDIV				0x160
#define  AXI_FREQ_SEL_MASK			0x0000001F
#define  AXI_FREQ_SEL_RD(src) \
	(AXI_FREQ_SEL_MASK & (uint32_t)(src))
#define  AXI_FREQ_SEL_SET(dst, src) \
	(((uint32_t)(dst) & ~AXI_FREQ_SEL_MASK) | \
		((uint32_t)(src) & AXI_FREQ_SEL_MASK))
#define SCU_SOCAHBDIV				0x164
#define  AHB_FREQ_SEL_MASK			0x0000001F
#define  AHB_FREQ_SEL_RD(src)			(0x0000001F & (uint32_t)(src))
#define  AHB_FREQ_SEL_SET(dst, src) \
	(((uint32_t)(dst) & ~0x0000001F) | ((uint32_t)(src) & 0x0000001F))

#define DIV2_1B_RD(src)			((0x00000600 & (uint32_t)(src)) >> 9)
#define DIV3_3B_RD(src)			((0x00000100 & (uint32_t)(src)) >> 8)
#define BIT_VALUE(val, bit) \
	(((1 << (uint32_t)(bit)) & (uint32_t)(val)) >> (bit))

static uint32_t scu_read_32(uintptr_t addr)
{
	/*
	 * TODO: For Secure Boot, need to use an indirect read.
	 */
	return mmio_read_32(addr);
}

uint64_t get_base_clk(void)
{
	return SYS_CLK_FREQ;
}

static uint32_t get_pll_div(uint32_t div2, uint32_t div3)
{
	/*
	 * Each bit of div2 can divide by 1 or divide by 2.
	 * div3 bit can divide by 2 or divide by 3
	 *
	 * pll_div = (div2.0 + 1) * (div2.1 + 1) * (div3.0 + 2)
	 *
	 * pll_div value is from 2 to 12.
	 */
	return (BIT_VALUE(div2, 0) + 1) * (BIT_VALUE(div2, 1) + 1) * (BIT_VALUE(div3, 0) + 2);
}

static uint64_t get_pll_freq(uintptr_t scu_pll_reg)
{
	uint32_t val, div, fbdivc;

	val = scu_read_32(scu_pll_reg);
	div = get_pll_div(DIV2_1B_RD(val), DIV3_3B_RD(val));
	fbdivc = FBDIVC_RD(val);

	return (get_base_clk()  * fbdivc) / div;
}

uint64_t get_pcppll_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	return get_pll_freq(PMPRO_SCU_CSR_REG_BASE + SCU_PCPPLL);
}

uint64_t get_pmdpll_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	return get_pll_freq(PMPRO_SCU_CSR_REG_BASE + SCU_PMDPLL);
}

uint64_t get_socpll_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	return get_pll_freq(SMPRO_SCU_CSR_REG_BASE + SCU_SOCPLL);
}

uint64_t get_ahb_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	uint64_t socclk = get_socpll_clk();
	uint32_t socahb = scu_read_32(SMPRO_SCU_CSR_REG_BASE + SCU_SOCAHBDIV);

	return (socclk / AHB_FREQ_SEL_RD(socahb));
}

uint64_t get_axi_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	uint64_t socclk = get_socpll_clk();
	uint32_t socaxi = scu_read_32(SMPRO_SCU_CSR_REG_BASE + SCU_SOCAXIDIV);
	uint64_t axiclk = socclk / AXI_FREQ_SEL_RD(socaxi);

	return axiclk;
}

uint64_t get_iobaxi_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	uint64_t socclk = get_socpll_clk();
	uint32_t sociobaxi = scu_read_32(SMPRO_SCU_CSR_REG_BASE + SCU_SOCIOBAXIDIV);
	uint64_t iobaxiclk = socclk / AXI_FREQ_SEL_RD(sociobaxi);

	return iobaxiclk;
}

uint64_t get_apb_clk(void)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	uint64_t ahbclk = get_ahb_clk();
	uint32_t apbcfg = mmio_read_32(AHBC_CLKRST_REG_BASE + AHBC_CLK_CONFIG);

	return ahbclk / CFG_AHB_APB_CLK_RATIO_RD(apbcfg);
}

uint64_t get_i2c_clk(uint32_t bus)
{
	if (XGENE_VHP)
		return XGENE_VHP;

	if (bus == 0 || bus == 1) {
		/* I2C 0 and 1 clock come from SMPMPro clock source */
		return SMPMRPO_CLK_FREQ;
	}

	return get_apb_clk();
}
