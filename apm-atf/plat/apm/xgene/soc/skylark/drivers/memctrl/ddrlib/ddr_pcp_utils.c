/*
 * Copyright (c) 2016 Applied Micro Circuits Corporation.
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED.
 */

#include "ddr_lib.h"

unsigned int rdiv(unsigned long long a, unsigned long long b)
{
    return (((a*10)+5)/(b*10));
}

unsigned int cdiv(unsigned long long a, unsigned long long b)
{
    return (a/b + ((a%b)!=0));
}

unsigned int ilog2(unsigned long x)
{
	int i;

	if (x == 0)
		return 0;

	for (i = 0; x != 0; i++)
		x >>= 1;

	return i - 1;
}

unsigned long long rb_page_translate(unsigned int page_addr,
				     unsigned int offset)
{
	unsigned long long sys_addr;

	sys_addr  = page_addr << 16;
	sys_addr += (offset << 2);
	sys_addr += PCP_RB_BASE;
	return sys_addr;
}

/* RB Address Read */
unsigned int rb_addr_read(unsigned long long sys_addr)
{
	return le32toh(*(volatile unsigned int *)sys_addr);
}

/* RB Address Write */
void rb_addr_write(unsigned long long sys_addr, unsigned int wr_data)
{
	*(volatile unsigned int *)sys_addr = htole32(wr_data);
}

/* RB Reg Read */
unsigned int rb_read(unsigned int page_addr, unsigned int offset)
{
	unsigned long long sys_addr;
	sys_addr = rb_page_translate(page_addr, offset);
	return le32toh(*(volatile unsigned int *)sys_addr);
}

/* RB Reg Write */
void rb_write(unsigned int page_addr, unsigned int offset, unsigned int wr_data)
{
	unsigned long long sys_addr;

	sys_addr = rb_page_translate(page_addr, offset);
	*(volatile unsigned int *)sys_addr = htole32(wr_data);
}

/* MCB Reg Read */
unsigned int mcb_read_reg(int mcb_id, unsigned int reg)
{
	unsigned int data;
	unsigned long long sys_addr, page_addr = mcb_id ?
	    PCP_RB_MCB1_PAGE : PCP_RB_MCB0_PAGE;

	sys_addr = rb_page_translate(page_addr, reg);

	data = le32toh(*(volatile unsigned int *)(sys_addr));
	ddr_debug("MCB[%d]: read reg 0x%llX value 0x%X\n",
		    mcb_id, sys_addr, data);
	DSB_SY_CALL;
	return data;
}

/* MCB Reg Write */
void mcb_write_reg(int mcb_id, unsigned int reg, unsigned int value)
{
	unsigned long long sys_addr;
	unsigned long page_addr;

	page_addr = mcb_id ?
		PCP_RB_MCB1_PAGE : PCP_RB_MCB0_PAGE;

	sys_addr = rb_page_translate(page_addr, reg);

	ddr_debug("MCB[%d]: write reg 0x%llX value 0x%08X\n",
		    mcb_id, sys_addr, value);
	*(volatile unsigned int *)sys_addr = htole32(value);
	DSB_SY_CALL;
}

/* MCB Poll Reg */
int mcb_poll_reg(int mcb_id, unsigned int reg, unsigned int value,
		 unsigned int mask)
{
	unsigned int rd_val;
	unsigned int polling_on;
	unsigned int err = 0;
	unsigned int timeout = 0x1000;

	polling_on = 1;
	while (polling_on) {
		rd_val = mcb_read_reg(mcb_id, reg);
		if ((value & mask) == (rd_val & mask)) {
			polling_on = 0;
		}
		if (polling_on) {
			if (timeout) {
				timeout--;
				DELAY(10);
			} else {
				polling_on = 0;
				err = -1;
			}
		}
#if (XGENE_VHP || XGENE_EMU)
		polling_on = 0;
#endif
	}
	// TDB timeout & error routine
	return err;
}

/* CSW Reg Read */
unsigned int csw_read_reg(unsigned int reg)
{
	unsigned int data;
	unsigned long long csw_reg, sys_addr;

	sys_addr = rb_page_translate(PCP_RB_CSW_PAGE, reg);

	csw_reg = (unsigned long long)(sys_addr);
	data = le32toh(*(volatile unsigned int *)csw_reg);
	ddr_debug("CSW: read reg 0x%llX value 0x%08X\n", csw_reg, data);
	DSB_SY_CALL;
	return data;
}

/* CSW Reg Write */
void csw_write_reg(unsigned int reg, unsigned int value)
{
	unsigned long long csw_reg, sys_addr;

	sys_addr = rb_page_translate(PCP_RB_CSW_PAGE, reg);

	csw_reg = (unsigned long long)(sys_addr);

	ddr_debug("CSW: write reg 0x%llX value 0x%08X\n", csw_reg, value);
	*(volatile unsigned int *)csw_reg = htole32(value);
	DSB_SY_CALL;
}


/* IOB Reg Read */
unsigned int iob_read_reg(unsigned int page_offset, unsigned int reg)
{
	unsigned int data;
	unsigned long long iob_reg, sys_addr;

	sys_addr = rb_page_translate(page_offset, reg);

	iob_reg = (unsigned long long)(sys_addr);
	data = le32toh(*(volatile unsigned int *)iob_reg);
	ddr_debug("IOB: read 0x%llX value 0x%08X\n",iob_reg, data);
	DSB_SY_CALL;
	return data;
}

/* IOB Reg Write */
void iob_write_reg(unsigned int page_offset, unsigned int reg, unsigned int value)
{
	unsigned long long iob_reg, sys_addr;

	sys_addr = rb_page_translate(page_offset, reg);

	iob_reg = (unsigned long long)(sys_addr);
	ddr_debug("IOB: write reg 0x%llX value 0x%08X\n", iob_reg, value);
	*(volatile unsigned int *)iob_reg = htole32(value);
	DSB_SY_CALL;
}

/* Update SCU_SRST.MCU_RESET (AsyncResetMcu), the register is in SMPro and requires a message transfer */
int update_async_reset_mcu(struct apm_memc *memc, unsigned int val)
{
	int err;
	unsigned read_data;

	err = memc->p_smpro_read(SCU_MCU_RST_ADDR, &read_data, 1);
	read_data = (read_data & ~0x80U) | (val & 0x80U);
	err |= memc->p_smpro_write(SCU_MCU_RST_ADDR, read_data, 1);

#if !XGENE_VHP
	err |= memc->p_smpro_read(SCU_MCU_RST_ADDR, &read_data, 1);
	while ((read_data & 0x80U) != (val & 0x80U)) {
		err |= memc->p_smpro_read(SCU_MCU_RST_ADDR, &read_data, 1);
	}
#else
	err = 0;
#endif
	return err;
}

void mcu_board_specific_settings(struct apm_memc *memc)
{
	/* Modify the rtw timing as the depend on speed */
	struct apm_mcu *mcu;
	const unsigned int mclk_ps = memc->mcu[0].ddr_info.t_ck_ps;
	unsigned int iia, add_cycles = 0;

	if (mclk_ps >= 1499)		/* 1333 */
		add_cycles = 0;
	else if (mclk_ps >= 1250)	/* 1600 */
		add_cycles = 0;
	else if (mclk_ps >= 1071)	/* 1866 */
		add_cycles = 1;
	else if (mclk_ps >= 937)	/* 2133 */
		add_cycles = 1;
	else if (mclk_ps >= 833)	/* 2400 */
		add_cycles = 2;
	else if (mclk_ps >= 750)	/* 2666 */
		add_cycles = 2;
	else if (mclk_ps >= 625)	/* 3200 */
		add_cycles = 3;
	else
		add_cycles = 0;

	for_each_mcu(iia) {
		mcu = (struct apm_mcu *) &memc->mcu[iia];

		mcu->mcu_ud.ud_rtw_s_margin  += add_cycles;
		mcu->mcu_ud.ud_rtw_l_margin  += add_cycles;
		mcu->mcu_ud.ud_rtw_cs_margin += add_cycles;
	}
}

/* PCP addressing mode configuration */
int pcp_addressing_mode(struct apm_memc *ddr)
{
	unsigned int mcb0_routing = 0, mcb1_routing = 0;
	unsigned int addr_map_mode, apm_addr_map_mode, dimm_count;
	unsigned int iia;

	/* PMPro Configures CSWCR0 register for interleaving and hashing */
	for_each_mcu(iia) {
		if (ddr->mcu[iia].enabled) {
			mcb0_routing += (iia < 4U);
			mcb1_routing += (iia > 3U);
		}
	}

	/*
	 * HashEn = 0
	 * Bit[2:0]=3b000 addr = request.addr  One or three SNs or DMCs
	 * Bit[2:0]=3b001 addr = {request.addr[MSB:8],request.addr[6:0]}  Two SNs or DMCs
	 * Bit[2:0]=3b010 addr = {request.addr[MSB:9],request.addr[6:0]}  Four SNs or DMCs
	 * Bit[2:0]=3b011 addr = {request.addr[MSB:10],request.addr[6:0]} Eight SNs or DMCs
	 * HashEn = 1
	 * Bit[2:0]=3b000 addr = request.addr
	 * Bit[2:0]=3b001 addr = {request.addr[MSB:31],request.addr[29:0]}		    Single MCB, Two  MCU
	 * Bit[2:0]=3b010 addr = {request.addr[MSB:30],request.addr[28:0]}		    Dual   MCB, One  MCU
	 * Bit[2:0]=3b011 addr = {request.addr[MSB:31],request.addr[29],request.addr[27:0]} Single MCB, Four MCU
	 * Bit[2:0]=3b100 addr = {request.addr[MSB:31],request.addr[28:0]}		    Dual   MCB, Two  MCU
	 * Bit[2:0]=3b101 addr = {request.addr[MSB:31],request.addr[27:0]}		    Dual   MCB, Four MCU
	 */
	if (ddr->memc_ud.ud_hash_en) {
		/* System Mapping Enabled */
		addr_map_mode = 4U;
		/* APM Address map mode */
		if ((mcb0_routing == 1) & (mcb1_routing == 0)) {
			apm_addr_map_mode = 0U;
		} else if ((mcb0_routing == 2) & (mcb1_routing == 0)) {
			apm_addr_map_mode = 1U;
		} else if ((mcb0_routing == 1) & (mcb1_routing == 1)) {
			apm_addr_map_mode = 2U;
		} else if ((mcb0_routing == 4) & (mcb1_routing == 0)) {
			apm_addr_map_mode = 3U;
		} else if ((mcb0_routing == 2) & (mcb1_routing == 2)) {
			apm_addr_map_mode = 4U;
		} else if ((mcb0_routing == 4) & (mcb1_routing == 4)) {
			apm_addr_map_mode = 5U;
		} else {
			ddr_err("Addressing configuration doesn't match DIMM layout\n");
			return -1;
		}

	} else if (ddr->memc_ud.ud_interleaving_en) {
		/* APM Address map mode */
		apm_addr_map_mode = 0U;
		dimm_count = mcb0_routing + mcb1_routing;
		/* Address map mode */
		if (dimm_count == 1) {
			addr_map_mode = 0U;
		} else if (dimm_count == 2) {
			addr_map_mode = 1U;
		} else if (dimm_count == 4) {
			addr_map_mode = 2U;
		} else if (dimm_count == 8) {
			addr_map_mode = 3U;
		} else {
			ddr_err("Addressing configuration doesn't match DIMM layout\n");
			return -1;
		}
	} else {
		apm_addr_map_mode = 0U;
		addr_map_mode = 0U;
	}

	for_each_mcu(iia) {
		if (ddr->mcu[iia].enabled) {
			ddr->mcu[iia].ddr_info.addr_map_mode = (addr_map_mode | 0x4U);
			ddr->mcu[iia].ddr_info.apm_addr_map_mode = apm_addr_map_mode;
		}
	}
	return 0;
}
/*  PCP Configuration check  */
int pcp_config_check(struct apm_memc *ddr)
{
	unsigned int mcb0_routing, mcb1_routing;
	unsigned int mcb0_channel = 0, mcb1_channel = 0;
	unsigned int iia, data;

	/* PMPro Configures CSWCR0 register for interleaving and hashing */
	for_each_mcu(iia) {
		if (ddr->mcu[iia].enabled) {
			mcb0_channel += (iia < 4U);
			mcb1_channel += (iia > 3U);
		}
	}

	switch (mcb0_channel) {
	case 0:
		mcb0_routing = 0;
		break;
	case 1:
		mcb0_routing = 0;
		break;
	case 2:
		mcb0_routing = 1;
		break;
	case 4:
		mcb0_routing = 2;
		break;
	default:
		ddr_err("Invalid Channel count for MCB0\n");
		return -1;
	}

	switch (mcb1_channel) {
	case 0:
		mcb1_routing = 0;
		break;
	case 1:
		mcb1_routing = 0;
		break;
	case 2:
		mcb1_routing = 1;
		break;
	case 4:
		mcb1_routing = 2;
		break;
	default:
		ddr_err("Invalid Channel count for MCB1\n");
		return -1;
	}

	data = csw_read_reg(PCP_RB_CSW_CSWCR0_PAGE_OFFSET);

	/* Check for Dual Mcb */
	if (PCP_RB_CSW_CSWCR0_DUALMCB_RD(data) != (mcb1_channel != 0)) {
		ddr_err("CSWCR0 DualMcb Mode mismatch\n");
		return -1;
	}

	/* Check for Mcb0 Routing */
	if (PCP_RB_CSW_CSWCR0_MCB0ROUTINGMODE_RD(data) != mcb0_routing) {
		ddr_err("MCB0 Routing Mode mismatch\n");
		return -1;
	}

	/* Check for Mcb1 Routing */
	if (PCP_RB_CSW_CSWCR0_MCB1ROUTINGMODE_RD(data) != mcb1_routing) {
		ddr_err("MCB1 Routing Mode mismatch\n");
		return -1;
	}

	return 0;
}

/* RB Access functions */
unsigned int get_mcu_pllcr0_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLCR0_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLCR0_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLCR0_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLCR0_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_pllcr1_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLCR1_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLCR1_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLCR1_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLCR1_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_pllcr2_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLCR2_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLCR2_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLCR2_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLCR2_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_pllcr3_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLCR3_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLCR3_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLCR3_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLCR3_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_pllcr4_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLCR4_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLCR4_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLCR4_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLCR4_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_pllsr_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0PLLSR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1PLLSR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2PLLSR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3PLLSR_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_ccr_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0CCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1CCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2CCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3CCR_PAGE_OFFSET;
	}
	return 0;
}

unsigned int get_mcu_rcr_page_offset(unsigned int index)
{
	index = index % 4;
	switch(index) {
	case PCP_RB_MCBX_MCU0:
		return PCP_RB_MCBX_MCU0RCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU1:
		return PCP_RB_MCBX_MCU1RCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU2:
		return PCP_RB_MCBX_MCU2RCR_PAGE_OFFSET;
	case PCP_RB_MCBX_MCU3:
		return PCP_RB_MCBX_MCU3RCR_PAGE_OFFSET;
	}
	return 0;
}

int is_skylark_A0(void)
{
	return 1;
}
