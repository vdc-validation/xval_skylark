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

unsigned long long mcu_addr(unsigned int mcu_id, unsigned int reg)
{
	unsigned int mcu_rb_base;
	unsigned long long sys_addr;

	mcu_rb_base = PCP_RB_MCU0_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	sys_addr = rb_page_translate(mcu_rb_base, reg);

	return sys_addr;
}

unsigned int mcu_read_reg(unsigned int mcu_id, unsigned int reg)
{
	unsigned int data, mcu_rb_base;
	unsigned long long sys_addr;

	mcu_rb_base = PCP_RB_MCU0_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	sys_addr = rb_page_translate(mcu_rb_base, reg);

	data = le32toh(*(volatile unsigned int *)sys_addr);
	ddr_debug("MCU[%d]: read 0x%llX value 0x%08X\n", mcu_id, sys_addr, data);
	return data;
}

void pmu_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value)
{
	unsigned int mcu_rb_base;
	unsigned long long sys_addr;

	mcu_rb_base = PCP_RB_MCU0_PMU_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	sys_addr = rb_page_translate(mcu_rb_base, reg);

	ddr_debug("MCU[%d]: write 0x%llX value 0x%08X\n", mcu_id, sys_addr, value);
	*(volatile unsigned int *)(sys_addr) = htole32(value);
}

void mcu_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value)
{
	unsigned int mcu_rb_base;
	unsigned long long sys_addr;

	mcu_rb_base = PCP_RB_MCU0_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	sys_addr = rb_page_translate(mcu_rb_base, reg);

	ddr_debug("MCU[%d]: write 0x%llX value 0x%08X\n",
		    mcu_id, sys_addr, value);
	*(volatile unsigned int *)(sys_addr) = htole32(value);
}

int mcu_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value, unsigned int mask,
		unsigned long long cycles)
{
	unsigned int rd_val;
	unsigned int polling_on;
	int err = 0;
	unsigned int timeout = (cycles + 1) * 10000;

	ddr_debug("MCU[%d]: Polling for 0x%x\n", mcu_id, value);

	polling_on = 1;
	while (polling_on) {
		rd_val = mcu_read_reg(mcu_id, reg);
		if ((value & mask) == (rd_val & mask))
			polling_on = 0;
		if (polling_on) {
			if (timeout) {
				timeout--;
				/* To be reduced if required */
				DELAY(10);
			} else {
				polling_on = 0;
				err = -1;
			}
		}
#if XGENE_VHP
		polling_on = 0;
#endif
	}
	return err;
}

void mcu_set_pll_params(struct mcu_pllctl *mcu_pllctl, apm_mcu_udparam_t ud_param, unsigned int t_ck_ps)
{
	unsigned int speed_grade = cdiv(2000000, t_ck_ps);

	if (speed_grade <= 800) {
		mcu_pllctl->pllctl_fbdivc   = 32U;
		mcu_pllctl->pllctl_outdiv2  = 0x3U;  /* div2=1 div1=1  (1=>/2, 0=>/1) */
		mcu_pllctl->pllctl_outdiv3  = 0U;   /* div3=0  (1=>/3 0=>/2) */
	} else if (speed_grade <= 1066) {
		mcu_pllctl->pllctl_fbdivc   = 32U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 1U;
	} else if (speed_grade <= 1250) {
		mcu_pllctl->pllctl_fbdivc   = 50U;
		mcu_pllctl->pllctl_outdiv2  = 0x3U;
		mcu_pllctl->pllctl_outdiv3  = 0U;
	} else if (speed_grade <= 1333) {
		mcu_pllctl->pllctl_fbdivc   = 40U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 1U;
	} else if (speed_grade <= 1600) {
		mcu_pllctl->pllctl_fbdivc   = 32U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 0U;
	} else if (speed_grade <= 1866) {
		mcu_pllctl->pllctl_fbdivc   = 56U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 1U;
	} else if (speed_grade <= 2133) {
		mcu_pllctl->pllctl_fbdivc   = 64U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 1U;
	} else if (speed_grade <= 2400) {
		mcu_pllctl->pllctl_fbdivc   = 48U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 0U;
	} else if (speed_grade <= 2667) {
		mcu_pllctl->pllctl_fbdivc   = 40U;
		mcu_pllctl->pllctl_outdiv2  = 0x0U;
		mcu_pllctl->pllctl_outdiv3  = 1U;
	} else if (speed_grade <= 3200) {
		mcu_pllctl->pllctl_fbdivc   = 64U;
		mcu_pllctl->pllctl_outdiv2  = 0x1U;
		mcu_pllctl->pllctl_outdiv3  = 0U;
	} else {
		ddr_err("Unknown Speed Grade: %d", speed_grade);
	}

	if (ud_param.ud_pll_force_en) {
		mcu_pllctl->pllctl_fbdivc  = ud_param.ud_pllctl_fbdivc;
		mcu_pllctl->pllctl_outdiv2 = ud_param.ud_pllctl_outdiv2;
		mcu_pllctl->pllctl_outdiv3 = ud_param.ud_pllctl_outdiv3;
	}
}

/* Update PMU registers */
void update_pmu_registers(struct apm_mcu *mcu)
{
	/* Update PMU_PMAUXR */
	pmu_write_reg(mcu->id, PCP_RB_MCUX_PMAUXR0_PAGE_OFFSET, 0xFFFFFF00);
}

/* Update BIST Addressing mode */
void update_bist_addressing(struct apm_mcu *mcu)
{
	/* Update AMCR */
	mcu_write_reg(mcu->id, PCP_RB_MCUX_MCUAMCR_PAGE_OFFSET, mcu->ddr_info.apm_addr_map_mode);
}

/* Update BIST Timing registers based of DMC */
void update_bist_timing_registers(unsigned int mcu_id)
{
	unsigned int dmc_data, mcu_data, data;
	ddr_info("MCU[%d] - MCU BIST Registers setup \n", mcu_id);

	/* ODT Timing control */
	dmc_data = dmc_read_reg(mcu_id, ODT_TIMING_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTODTCTL_PAGE_OFFSET, dmc_data);

	/* ODT WR control */
	mcu_data = 0U;

	/* ODT WR for Logical Rank0..3 */
	dmc_data = dmc_read_reg(mcu_id, ODT_WR_CONTROL_31_00_NOW_ADDR);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 4) & 0x3) << 2) | (dmc_data & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK0ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 12) & 0x3) << 2) | ((dmc_data >> 8) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK1ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 20) & 0x3) << 2) | ((dmc_data >> 16) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK2ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 28) & 0x3) << 2) | ((dmc_data >> 24) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK3ODTWR_SET(mcu_data, data);

	/* ODT WR for Logical Rank4..7 */
	dmc_data = dmc_read_reg(mcu_id, ODT_WR_CONTROL_63_32_NOW_ADDR);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 4) & 0x3) << 2) | (dmc_data & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK4ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 12) & 0x3) << 2) | ((dmc_data >> 8) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK5ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 20) & 0x3) << 2) | ((dmc_data >> 16) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK6ODTWR_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 28) & 0x3) << 2) | ((dmc_data >> 24) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTWR_RANK7ODTWR_SET(mcu_data, data);

	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTODTWR_PAGE_OFFSET, mcu_data);

	/* ODT RD control */
	mcu_data = 0U;

	/* ODT RD for Logical Rank0..3 */
	dmc_data = dmc_read_reg(mcu_id, ODT_RD_CONTROL_31_00_NOW_ADDR);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 4) & 0x3) << 2) | (dmc_data & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK0ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 12) & 0x3) << 2) | ((dmc_data >> 8) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK1ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 20) & 0x3) << 2) | ((dmc_data >> 16) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK2ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 28) & 0x3) << 2) | ((dmc_data >> 24) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK3ODTRD_SET(mcu_data, data);

	/* ODT RD for Logical Rank4..7 */
	dmc_data = dmc_read_reg(mcu_id, ODT_RD_CONTROL_63_32_NOW_ADDR);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 4) & 0x3) << 2) | (dmc_data & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK4ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 12) & 0x3) << 2) | ((dmc_data >> 8) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK5ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 20) & 0x3) << 2) | ((dmc_data >> 16) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK6ODTRD_SET(mcu_data, data);

	/* BIST uses Physical Ranks and only supports 4 ODT lines */
	data = (((dmc_data >> 28) & 0x3) << 2) | ((dmc_data >> 24) & 0x3);
	mcu_data = PCP_RB_MCUX_MCUBISTODTRD_RANK7ODTRD_SET(mcu_data, data);

	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTODTRD_PAGE_OFFSET, mcu_data);

	/* PHY WRLAT */
	dmc_data = dmc_read_reg(mcu_id, T_PHYWRLAT_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTWRLAT_PAGE_OFFSET, dmc_data);

	/* PHY RDDATA_EN */
	dmc_data = dmc_read_reg(mcu_id, T_RDDATA_EN_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTRDEN_PAGE_OFFSET, dmc_data);

	/* RTR */
	dmc_data = dmc_read_reg(mcu_id, T_RTR_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTRTR_PAGE_OFFSET, dmc_data);

	/* WTW */
	dmc_data = dmc_read_reg(mcu_id, T_WTW_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTWTW_PAGE_OFFSET, dmc_data);

	/* WTR */
	dmc_data = dmc_read_reg(mcu_id, T_WTR_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTWTR_PAGE_OFFSET, dmc_data);

	/* RTW */
	dmc_data = dmc_read_reg(mcu_id, T_RTW_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTRTW_PAGE_OFFSET, dmc_data);

	/* PARITY_IN */
	dmc_data = dmc_read_reg(mcu_id, T_PARITY_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTPAR_PAGE_OFFSET, FIELD_DMC520_T_PARIN_LAT_NOW_RD(dmc_data));

	/* T_RFC */
	dmc_data = dmc_read_reg(mcu_id, T_RFC_NOW_ADDR);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTRFC_PAGE_OFFSET, FIELD_DMC520_T_RFC_NOW_RD(dmc_data));
}

/* Enable RdPoisonDisable, WrPoisonDisable */
void mcu_poison_disable_set(unsigned int mcu_id)
{
	unsigned int regd;

	regd = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUECR_PAGE_OFFSET);
	regd = PCP_RB_MCUX_MCUECR_RDPOISONDISABLE_SET(regd, 1);
	regd = PCP_RB_MCUX_MCUECR_WRPOISONDISABLE_SET(regd, 1);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUECR_PAGE_OFFSET, regd);
}

/* Clear RdPoisonDisable, WrPoisonDisable */
void mcu_poison_disable_clear(unsigned int mcu_id)
{
	unsigned int regd;

	regd = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUECR_PAGE_OFFSET);
	regd = PCP_RB_MCUX_MCUECR_RDPOISONDISABLE_SET(regd, 0);
	regd = PCP_RB_MCUX_MCUECR_WRPOISONDISABLE_SET(regd, 0);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUECR_PAGE_OFFSET, regd);
}

/* This function is used for Speed Grade display */
unsigned int ddr_speed_display(unsigned int t_ck_ps)
{
	unsigned int speed_grade;

	speed_grade = cdiv(2000000, t_ck_ps);

	/* Since, DDR 2400 Speed Grade display is not accurate */
	if (speed_grade < 2410 && speed_grade > 2390)
		return 2400;
	else
		return speed_grade;
}
