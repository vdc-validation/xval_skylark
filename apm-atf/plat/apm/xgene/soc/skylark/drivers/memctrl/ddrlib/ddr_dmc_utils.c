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

unsigned long long dmc_addr(unsigned int mcu_id, unsigned int reg)
{
	unsigned int dmcl_rb_base, dmch_rb_base;
	unsigned long long sys_addr;

	dmcl_rb_base = PCP_RB_MCU0_DMCL_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;
	dmch_rb_base = PCP_RB_MCU0_DMCH_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	if (reg < 0x1000)
		sys_addr = PCP_RB_BASE + (dmcl_rb_base << 16) + reg;
	else
		sys_addr = PCP_RB_BASE + (dmch_rb_base << 16) + (reg & 0xFFF);

	return sys_addr;
}

unsigned int dmc_read_reg(unsigned int mcu_id, unsigned int reg)
{
	unsigned int data;
	unsigned long long sys_addr;

	sys_addr = dmc_addr(mcu_id, reg);

	data = le32toh(*(volatile unsigned int *)(sys_addr));
	ddr_debug("MCU-DMC[%d]: read 0x%llX value 0x%08X\n",
		    mcu_id, sys_addr, data);
	return data;
}

void dmc_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value)
{
	unsigned long long sys_addr;

	sys_addr = dmc_addr(mcu_id, reg);

	ddr_debug("MCU-DMC[%d]: write 0x%llX value %08X\n",
		    mcu_id, sys_addr, value);
	*(volatile unsigned int *)(sys_addr) = htole32(value);
}

#if XGENE_VHP
int dmc_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value,
		 unsigned int mask)
{
	return 0;
}
#else
int dmc_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value,
		 unsigned int mask)
{
	unsigned int rd_val;
	unsigned int timeout = 2000;

	ddr_debug("MCU-DMC[%d]: Polling for 0x%x\n", mcu_id, value);

	while (timeout > 0) {
		rd_val = dmc_read_reg(mcu_id, reg);
		if ((value & mask) == (rd_val & mask))
			return 0;
		timeout--;
		DELAY(10);
	}
	return -1;
}
#endif

/* DMC Multiple DCI Command */
void dmc_cmd_multiple(unsigned int mcu_id, unsigned int direct_cmd, unsigned int rank_sel,
		      unsigned int bank, unsigned int expected_pslverr,
		      unsigned int abort, unsigned int override_replay)
{
	unsigned int direct_cmd_data;

	direct_cmd_data = 0;
	/* populate direct_cmd fields */
	direct_cmd_data = FIELD_DMC520_DIRECT_CMD_SET(direct_cmd_data, direct_cmd);
	direct_cmd_data = FIELD_DMC520_RANK_ADDR_SET(direct_cmd_data, rank_sel);
	direct_cmd_data = FIELD_DMC520_DIRECT_BA_SET(direct_cmd_data, bank);

	if (abort == 1 && (override_replay == REGV_REPLAY_OVERRIDE_REPLAY_TYPE))
		direct_cmd_data = FIELD_DMC520_REPLAY_OVERRIDE_SET(direct_cmd_data, REGV_REPLAY_OVERRIDE_ABORT);
	else
		direct_cmd_data = FIELD_DMC520_REPLAY_OVERRIDE_SET(direct_cmd_data, override_replay);

	ddr_debug("MCU-DMC[%d]: DIRECT CMD: write offset 0x%08X value 0x%08X\n",
		  mcu_id, DMC520_DIRECT_CMD_ADDR, direct_cmd_data);
	dmc_write_reg(mcu_id, DMC520_DIRECT_CMD_ADDR, direct_cmd_data);
}


/* DMC Single DCI Command */
void dmc_cmd_single(unsigned int mcu_id, unsigned int direct_cmd, unsigned int rank_bin,
		    unsigned int bank, unsigned int expected_pslverr,
		    unsigned int abort, unsigned int override_replay)
{

	unsigned int rank_sel = 0;
	rank_sel = 1 << rank_bin;

	dmc_cmd_multiple(mcu_id, direct_cmd, rank_sel, bank, expected_pslverr, abort, override_replay);
}


/* Put DMC in Ready State */
int dmc_ready_state(unsigned int mcu_id)
{
	int err = 0;

	/* Generate a CONFIG Command */
	dmc_write_reg(mcu_id, MEMC_CMD_ADDR, REGV_MEMC_CMD_GO);
	/* Poll Memc Status */
	err = dmc_poll_reg(mcu_id, MEMC_STATUS_ADDR, REGV_MEMC_STATUS_READY << FIELD_DMC520_MEMC_STATUS_SHIFT_MASK, FIELD_DMC520_MEMC_STATUS_MASK);

	return err;
}


/* Put DMC in Config State */
int dmc_config_state(unsigned int mcu_id)
{
	int err = 0;

	/* Generate a CONFIG Command */
	dmc_write_reg(mcu_id, MEMC_CMD_ADDR, REGV_MEMC_CMD_CONFIGURE);
	/* Poll Memc Status */
	err = dmc_poll_reg(mcu_id, MEMC_STATUS_ADDR, REGV_MEMC_STATUS_CONFIG << FIELD_DMC520_MEMC_STATUS_SHIFT_MASK, FIELD_DMC520_MEMC_STATUS_MASK);

	return err;
}

/* DMC issues Execute Drain command */
int dmc_execute_drain(unsigned int mcu_id)
{
	int err;

	/* Generate a EXECUTE_DRAIN Command */
	dmc_write_reg(mcu_id, MEMC_CMD_ADDR, REGV_MEMC_CMD_EXECUTE_DRAIN);
	/* Poll Channel Idle Status */
	err = dmc_poll_reg(mcu_id, DMC520_MGR_ACTIVE_ADDR,
			   REGV_MGR_ACTIVE_IDLE << FIELD_DMC520_MGR_ACTIVE_SHIFT_MASK,
			   FIELD_DMC520_MGR_ACTIVE_MASK);

	return err;
}

/* Update DMC, copy next reg -> now reg */
void dmc_update(unsigned int mcu_id)
{
	/* Generate a UPDATE Command */
	dmc_write_reg(mcu_id, DIRECT_CMD_ADDR, REGV_DIRECT_CMD_UPDATE);
}

static char *dmc_state_decoder(int dmc_current_state)
{
	static char state[30];
	switch (dmc_current_state) {
	case REGV_MEMC_STATUS_CONFIG:
		sprintf(state, "%s", "CONFIG State");
		return state;
	case REGV_MEMC_STATUS_LOW_POWER:
		sprintf(state, "%s", "LOW POWER State");
		return state;
	case REGV_MEMC_STATUS_PAUSED:
		sprintf(state, "%s", "PAUSED State");
		return state;
	case REGV_MEMC_STATUS_READY:
		sprintf(state, "%s", "READY State");
		return state;
	case REGV_MEMC_STATUS_ABORTED:
		sprintf(state, "%s", "ABORTED State");
		return state;
	case REGV_MEMC_STATUS_RECOVER:
		sprintf(state, "%s", "RECOVER State");
		return state;
	default:
		sprintf(state, "%s", "UNKNOWN State");
		return state;
	}
}

/* MC-initiated update request to reset FIFOs in read path, and upate PHY delays. */
int phy_update(unsigned int mcu_id)
{
	int err = 0, dmc_data;
	unsigned int dmc_saved_state;

	/* Read Current State of the DMC */
	dmc_data = dmc_read_reg(mcu_id, MEMC_STATUS_ADDR);
	dmc_data = FIELD_DMC520_MEMC_STATUS_RD(dmc_data);
	dmc_saved_state = dmc_data;

	/* Change to Config State, if Current State is not Config State */
	if (dmc_data != REGV_MEMC_STATUS_CONFIG) {
		err = dmc_config_state(mcu_id);
		if (err)
			return err;
	}

	/* Check for DMC State */
	dmc_data = dmc_read_reg(mcu_id, MEMC_STATUS_ADDR);
	dmc_data = FIELD_DMC520_MEMC_STATUS_RD(dmc_data);

	if (dmc_data != REGV_MEMC_STATUS_CONFIG) {
		ddr_err("MCU-DMC[%d]: DMC STATE = 0x%03X [%s]\n", mcu_id, dmc_data,
				dmc_state_decoder(dmc_data));
		return -1;
	}

	/* Issue PHY update */
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_single(mcu_id, REGV_DIRECT_CMD_TRAIN, 0, 0, 0, 0,
				REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Poll for PHY update completion*/
	err = dmc_poll_reg(mcu_id, MEMC_STATUS_ADDR,
				REGV_MGR_ACTIVE_IDLE << FIELD_DMC520_MGR_ACTIVE_SHIFT_MASK,
				FIELD_DMC520_MGR_ACTIVE_MASK);
	if (err)
		return err;

	err = dmc_poll_reg(mcu_id, CHANNEL_STATUS_ADDR,
				REGV_M0_IDLE_HIGH << FIELD_DMC520_M0_IDLE_SHIFT_MASK,
				FIELD_DMC520_M0_IDLE_MASK);
	if (err)
		return err;

	/* Revert Back to Previous State */
	dmc_write_reg(mcu_id, MEMC_CMD_ADDR, dmc_saved_state);
	err = dmc_poll_reg(mcu_id, MEMC_STATUS_ADDR,
				dmc_saved_state << FIELD_DMC520_MEMC_STATUS_SHIFT_MASK,
				FIELD_DMC520_MEMC_STATUS_MASK);
	return err;
}


/* Configure PHY Low Power parameters controlled by DMC */
void config_phy_low_power_parameters(unsigned int mcu_id)
{
	/*
	 * t_lpresp
	 * Wait for CPHY LP Ack
	 * The DMC waits a minimum t_lpresp cycles after asserting a PHY low power request before deasserting
	 * the request and resuming other commands. Zero means wait for dfi_lp_ack. This value
	 * must be programmed in DFI_PHY clock cycles. Program it with respect to DFI-freq-ratio. The
	 * supported range for this bitfield is 0-36. Zero is not supported by DMC
	 */
	dmc_write_reg(mcu_id, T_LPRESP_NEXT_ADDR, 0); /* TODO */

	/* phy_power_control */
	dmc_write_reg(mcu_id, PHY_POWER_CONTROL_NEXT_ADDR, 0); /* TODO */
}


/* Config MR0 */
unsigned int config_mr0_value(unsigned int t_wr, unsigned int cas_latency)
{
	unsigned int mrs_mr0 = 0;

#ifdef DDR3_MODE
	/*
	 * Set CAS Latency value. Note: -4 is because the encoding for the
	 * CAS latency value is 1 for CL of 5, 2 for CL of 6 etc
	 */
	mrs_mr0 |= ((((cas_latency-4) >> 3) & 0x1) << 2);
	mrs_mr0 |= ((((cas_latency-4) >> 0) & 0x7) << 4);

	/* DLL Reset to 1 */
	mrs_mr0 |= (1 << 8);

	/* Set write recovery for precharge */
	if (t_wr == 5)
		mrs_mr0 |= (1 << 9);
	if (t_wr == 6)
		mrs_mr0 |= (2 << 9);
	if (t_wr == 7)
		mrs_mr0 |= (3 << 9);
	if (t_wr == 8)
		mrs_mr0 |= (4 << 9);
	if (t_wr == 10)
		mrs_mr0 |= (5 << 9);
	if (t_wr == 12)
		mrs_mr0 |= (6 << 9);
	if (t_wr == 14)
		mrs_mr0 |= (7 << 9);
	if (t_wr == 16)
		mrs_mr0 |= (0 << 9);
#else
	/* Set CAS Latency value */
	if (cas_latency == 9)
		mrs_mr0 |= 0x0000;
	if (cas_latency == 10)
		mrs_mr0 |= 0x0004;
	if (cas_latency == 11)
		mrs_mr0 |= 0x0010;
	if (cas_latency == 12)
		mrs_mr0 |= 0x0014;
	if (cas_latency == 13)
		mrs_mr0 |= 0x0020;
	if (cas_latency == 14)
		mrs_mr0 |= 0x0024;
	if (cas_latency == 15)
		mrs_mr0 |= 0x0030;
	if (cas_latency == 16)
		mrs_mr0 |= 0x0034;
	if (cas_latency == 18)
		mrs_mr0 |= 0x0040;
	if (cas_latency == 20)
		mrs_mr0 |= 0x0044;
	if (cas_latency == 22)
		mrs_mr0 |= 0x0050;
	if (cas_latency == 24)
		mrs_mr0 |= 0x0054;
	if (cas_latency == 23)
		mrs_mr0 |= 0x0060;
	if (cas_latency == 17)
		mrs_mr0 |= 0x0064;
	if (cas_latency == 19)
		mrs_mr0 |= 0x0070;
	if (cas_latency == 21)
		mrs_mr0 |= 0x0074;
	if (cas_latency == 25)
		mrs_mr0 |= 0x1000;
	if (cas_latency == 26)
		mrs_mr0 |= 0x1004;
	if (cas_latency == 27)
		mrs_mr0 |= 0x1010;
	if (cas_latency == 28)
		mrs_mr0 |= 0x1014;
	if (cas_latency == 30)
		mrs_mr0 |= 0x1024;
	if (cas_latency == 32)
		mrs_mr0 |= 0x1034;

	/* DLL Reset to 1 if DLL is on */
	mrs_mr0 |= (1 << 8);

	/* Set write recovery and read to precharge */
	if (t_wr == 10)
		mrs_mr0 |= (0 << 9);
	if (t_wr == 12)
		mrs_mr0 |= (1 << 9);
	if (t_wr == 14)
		mrs_mr0 |= (2 << 9);
	if (t_wr == 16)
		mrs_mr0 |= (3 << 9);
	if (t_wr == 18)
		mrs_mr0 |= (4 << 9);
	if (t_wr == 20)
		mrs_mr0 |= (5 << 9);
	if (t_wr == 24)
		mrs_mr0 |= (6 << 9);
#endif
	return mrs_mr0;
}

/* Config MR1 */
unsigned int config_mr1_value(unsigned int rtt_nom, unsigned int driver_impedance,
			      unsigned int write_lvl, unsigned int output_buffer_en)
{
	unsigned int mrs_mr1 = 0;

	/* {A10, A9, A8} to enable Rtt_Nom - DDR4 Spec, section 3.5 */
	mrs_mr1 |= (rtt_nom << 8);

	/* DLL Enable */
	mrs_mr1 |= 1;

	/* TDQS Enable is always disabled
	   Output Driver Impedance */
	mrs_mr1 |= ((driver_impedance & 0x3) << 1);

	/* Write Leveling */
	mrs_mr1 |= (write_lvl << 7);

	/* Output Buffer 0: Enable, 1: Disable */
	mrs_mr1 |= ((!output_buffer_en) << 12);

	return mrs_mr1;
}

/* Config MR2 */
unsigned int config_mr2_value(unsigned int cw_latency, unsigned int rtt_wr, unsigned int crc_enable)
{
	unsigned int mrs_mr2 = 0;

#ifdef DDR3_MODE
	/*
	 * Decide on a CAS write latency depending on the tck range. Note: -5 is
	 * because the encoding for the CAS write latency is 0 for CWL 5, 1 for CWL
	 * of 6 etc
	 */
	mrs_mr2 |= ((cw_latency-5) << 3);
#else
	/* Decide on a CAS write latency depending on the tck range */
	if (cw_latency == 9)
		mrs_mr2 |= (0 << 3);
	if (cw_latency == 10)
		mrs_mr2 |= (1 << 3);
	if (cw_latency == 11)
		mrs_mr2 |= (2 << 3);
	if (cw_latency == 12)
		mrs_mr2 |= (3 << 3);
	if (cw_latency == 14)
		mrs_mr2 |= (4 << 3);
	if (cw_latency == 16)
		mrs_mr2 |= (5 << 3);
	if (cw_latency == 18)
		mrs_mr2 |= (6 << 3);
	if (cw_latency == 20)
		mrs_mr2 |= (7 << 3);

	/* CRC Enable */
	mrs_mr2 |= (crc_enable << 12);
#endif
	/* {A10, A9} to enable Rtt_wr - DDR4 Spec, section 3.5 */
	mrs_mr2 |= (rtt_wr << 9);

	return mrs_mr2;
}

/* Config MR3 */
unsigned int config_mr3_value(unsigned int t_ck_ps, unsigned int refresh_granularity,
                              unsigned int mpr_mode, unsigned int mpr_page, unsigned int pda_mode)
{
	unsigned int mrs_mr3 = 0;
	unsigned int crc_dm = 0;

#ifdef DDR3_MODE
	mrs_mr3 = 0;
#else
	/* CRC_DM is based of Speed Grade */
	if (t_ck_ps <= 1250)
		crc_dm = 4;
	else if (t_ck_ps <= 833)
		crc_dm = 5;
	else if (t_ck_ps <= 750)
		crc_dm = 6;
	else
		crc_dm = 7;

	/* Write Command Latency when CRC and DM are both enabled */
	if (crc_dm == 4)
		mrs_mr3 |= (0 << 9);
	if (crc_dm == 5)
		mrs_mr3 |= (1 << 9);
	if (crc_dm == 6)
		mrs_mr3 |= (2 << 9);

	/* OTF Refresh support in DDR4 Denali */
	if (refresh_granularity == REGV_REFRESH_GRANULARITY_2X)
		mrs_mr3 |= (1 << 6); /* fixed 1x/2x */
	else if (refresh_granularity == REGV_REFRESH_GRANULARITY_4X)
		mrs_mr3 |= (2 << 6); /* fixed 1x/4x */

	/*
	 * Always enable the temperature readout option (the actual polling is
	 * programmed by enabling or disabling in the DMC)
	 */
	mrs_mr3 |= (1 << 5);

	/* PDA mode */
	mrs_mr3 |= ((pda_mode & 0x1) << 4);

	/* MPR Operation */
	mrs_mr3 |= ((mpr_mode & 0x1) << 2);

	/* MPR Page Selection */
	mrs_mr3 |= (mpr_page & 0x3);
#endif
	return mrs_mr3;
}
#ifndef DDR3_MODE

/* Config MR4 */
unsigned int config_mr4_value(unsigned int t_cal, unsigned int rd_preamble, unsigned wr_preamble)
{
	unsigned int mrs_mr4 = 0;

	/*
	 * TODO
	 * Temp controlled Refresh support in DDR4 Denali
	 * if (bitfields["temp_controlled_refresh_mode"]) mrs_mr4 |= (1 << 3);
	 * if (bitfields["temp_controlled_refresh_range"]) mrs_mr4 |= (1 << 2);
	 */

	/* Set the read and write preamble */
	if (rd_preamble == 1)
		mrs_mr4 |= (0 << 11);
	if (rd_preamble == 2)
		mrs_mr4 |= (1 << 11);

	if (wr_preamble == 1)
		mrs_mr4 |= (0 << 12);
	if (wr_preamble == 2)
		mrs_mr4 |= (1 << 12);

	/* Set the command address latency */
	if (t_cal == 0)
		mrs_mr4 |= (0 << 6);
	if (t_cal == 3)
		mrs_mr4 |= (1 << 6);
	if (t_cal == 4)
		mrs_mr4 |= (2 << 6);
	if (t_cal == 5)
		mrs_mr4 |= (3 << 6);
	if (t_cal == 6)
		mrs_mr4 |= (4 << 6);
	if (t_cal == 8)
		mrs_mr4 |= (5 << 6);
	if (t_cal == 10)
		mrs_mr4 |= (6 << 6);

	return mrs_mr4;
}

/* Config MR5 */
unsigned int config_mr5_value(unsigned int rtt_park, unsigned int read_dbi, unsigned int write_dbi, unsigned int parity_latency)
{
	unsigned int mrs_mr5 = 0;

	/* {A8, A7, A6} to enable Rtt_Park - DDR4 Spec, section 3.5 */
	mrs_mr5 |= (rtt_park << 6);

	/* Persistent Parity Error Mode Enable */
	mrs_mr5 |= (1 << 9);

	/* Write DBI Enable */
	mrs_mr5 |= (write_dbi << 11);

	/* Read DBI Enable */
	mrs_mr5 |= (read_dbi << 12);

	/* Parity latency */
	if (parity_latency == 4)
		mrs_mr5 |= 1;
	if (parity_latency == 5)
		mrs_mr5 |= 2;
	if (parity_latency == 6)
		mrs_mr5 |= 3;
	if (parity_latency == 8)
		mrs_mr5 |= 4;

	return mrs_mr5;
}

/* Config MR6 */
unsigned int config_mr6_value(unsigned int vref_mode, unsigned int vref_training,
			      unsigned int t_ck_ps)
{
	unsigned int mrs_mr6 = 0;

	/* tCCD_L from DDR4 Spec., Page 22, Table 13 */
	if (t_ck_ps >= 2500)		/* 800 */
		mrs_mr6 |= (1 << 10);
	else if (t_ck_ps >= 1875)	/* 1066 */
		mrs_mr6 |= (1 << 10);
	else if (t_ck_ps >= 1499)	/* 1333 */
		mrs_mr6 |= (1 << 10);
	else if (t_ck_ps >= 1250)	/* 1600 */
		mrs_mr6 |= (1 << 10);
	else if (t_ck_ps >= 1071)	/* 1866 */
		mrs_mr6 |= (1 << 10);
	else if (t_ck_ps >= 937)	/* 2133 */
		mrs_mr6 |= (2 << 10);
	else if (t_ck_ps >= 833)	/* 2400 */
		mrs_mr6 |= (2 << 10);
	else if (t_ck_ps >= 750)	/* 2666 */
		mrs_mr6 |= (3 << 10);
	else if (t_ck_ps >= 625)	/* 3200 */
		mrs_mr6 |= (4 << 10);
	else
		mrs_mr6 |= (0 << 10);

	/* 0- Normal mode, 1- Training mode */
	mrs_mr6 |= vref_training & 0x7F;

	/* Vref Range Ref JEDEC DDR4 Spec Table 16 */
	mrs_mr6 |= (vref_mode & 0x1) << 7;

	return mrs_mr6;
}

void mr6_vreftrain(unsigned int mcu_id, unsigned int target_rank, unsigned int vref_mode,
		unsigned int vref_range, unsigned int vref_value, unsigned int t_ck_ps,
		unsigned int pda_component)
{
	unsigned int mrs_value, vref_train, val;

	/* MRS6: Set New VREF-DQ */
	/* config_mr6_value PARAMx
	 * vref_mode, vref_training, ccd_l
	 */
	vref_train = (vref_range << 6) | vref_value;
	mrs_value = config_mr6_value(vref_mode, vref_train, t_ck_ps);
	/* Maintaining Component value, in non-pda mode this is dont care */
	val = (pda_component & 0x1F) << 24;
	mrs_value = (mrs_value & 0x0FFFFFF) | val;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, target_rank, 6, 0, 0, 0);

	/* Flush the MR6 command */
	dmc_flush_seq(mcu_id);
}
void mr3_pda_access(unsigned int mcu_id, unsigned int target_rank, unsigned int pda_mode,
		unsigned int component, unsigned int t_ck_ps, unsigned int refresh_granularity)
{
	unsigned int mrs_value;

	/* config_mr3_value PARAMs
	 * t_ck_ps, refresh_granularity, mpr_mode, mpr_page, pda_mode
	 */
	mrs_value = config_mr3_value(t_ck_ps, refresh_granularity, 0, 0, pda_mode);
	mrs_value |= ((component & 0x1F) << 24);
	mrs_value |= (0x1 << 29); /* All components targeted */
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, target_rank, 3, 0, 0, 0);

	/* Flush the MR3 command */
	dmc_flush_seq(mcu_id);
}

#endif

/* DMC read ECC Enable */
unsigned int ecc_enable(unsigned int mcu_id)
{
	unsigned int data;

	/* Generate a UPDATE Command */
	data = dmc_read_reg(mcu_id, DMC520_ECC_ENABLE_ADDR);
	return FIELD_DMC520_ECC_ENABLE_RD(data);
}

/* DMC read CRC Enable */
unsigned int crc_enable(unsigned int mcu_id)
{
	unsigned int data;

	/* Generate a UPDATE Command */
	data = dmc_read_reg(mcu_id, DMC520_CRC_ENABLE_NOW_ADDR);
	return FIELD_DMC520_CRC_ENABLE_NOW_RD(data);
}

/* DMC read t_wr */
unsigned int cal_t_wr(unsigned int mcu_id, unsigned int t_ck_ps,
		       unsigned int write_preamble)
{
	unsigned int data;

	data = cdiv(15000, t_ck_ps);

	if (write_preamble == 2)
		data = data + 2;
	return data;
}

/* DMC read t_cal */
unsigned int read_t_cal(unsigned int mcu_id)
{
	unsigned int data;

	/* Generate a UPDATE Command */
	data = dmc_read_reg(mcu_id, T_CMD_NOW_ADDR);
	return FIELD_DMC520_T_CAL_NOW_RD(data);
}

/* DMC read dbi enable */
unsigned int read_dbi_enable(unsigned int mcu_id)
{
	unsigned int data;

	/* Generate a UPDATE Command */
	data = dmc_read_reg(mcu_id, DMC520_READ_DBI_ENABLE_NOW_ADDR);
	return FIELD_DMC520_READ_DBI_ENABLE_NOW_RD(data);
}

/* DMC write dbi enable */
unsigned int write_dbi_enable(unsigned int mcu_id)
{
	unsigned int data;

	/* Generate a UPDATE Command */
	data = dmc_read_reg(mcu_id, DMC520_WRITE_DBI_ENABLE_NOW_ADDR);
	return FIELD_DMC520_WRITE_DBI_ENABLE_NOW_RD(data);
}

/* DMC DRAM init */
void dram_init(struct apm_mcu *mcu)
{
	unsigned int reset_cycle_count, counter = 0, rank_sel = 0;
	unsigned int t_ck_ps = 0, ddr_type = 0;
	package_type_e package_type = 0;
	unsigned int dllk_cycle_count = 0, stable_cycle_count = 0;

	rank_sel     = mcu->ddr_info.active_ranks;
	t_ck_ps      = mcu->ddr_info.t_ck_ps;
	package_type = mcu->ddr_info.package_type;
	ddr_type     = mcu->ddr_info.ddr_type;

	/*  Start clock before reset */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_POWERDOWN_ENTRY, rank_sel, 0, 0, 0, 0);

	/* Invalidate DCB entries */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_INVALIDATE_RESET, 0, 0, 0, 0, 0);

	/* DRAM assert Reset */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 1);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_INVALIDATE_RESET, rank_sel, 0, 0, 0, 0);

#ifdef DDR3_MODE
	reset_cycle_count = 512;
#else
	/*  Wait until tPW_RESET is satisfied (min. 1us DDR4-JEDEC-4A Page 189 Table 90) */
	/*  1600 cycles required (min t_ck is 0.625ns for DDR4-3200) */
	reset_cycle_count = (1000000)/t_ck_ps;
#endif
	while (counter < reset_cycle_count) {
		dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x003E8);
		dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_WAIT, 0, 0, 0, 0, 0);
		counter = counter + 0x003E8;
	}

	/* DRAM Deassert Reset */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x10001);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_INVALIDATE_RESET, rank_sel, 0, 0, 0, 0);

	if ((ddr_type == SPD_MEMTYPE_DDR4) & (package_type != UDIMM)) {
		stable_cycle_count = (5*1000000)/t_ck_ps;
		counter = 0;

		while (counter < stable_cycle_count) {
			dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x3E8);
			dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_WAIT, 0, 0, 0, 0, 0);
			counter = counter + 0x3E8;
		}
	}

	if ((ddr_type == SPD_MEMTYPE_DDR4) & (package_type != UDIMM)) {
		dllk_cycle_count = 768; /* Denali model requirement */
		counter = 0;

		/* DDR4 RCD requirement DR4RCD01 JC_40.4 0304.2122.3 */
		/* Wait for tdllk cycles before asserting CKE */
		while (counter < dllk_cycle_count) {
			dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x3E8);
			dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_WAIT, 0, 0, 0, 0, 0);
			counter = counter + 0x3E8;
		}
	}

	/*  Assert dfi_cke to ALL INITIALISED RANKS */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_NOP, rank_sel, 0, 0, 0, 0);
}


/*  DRAM MRS Programming */
void dram_mrs_program(struct apm_mcu *mcu)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	unsigned int rank_sel, mrs_value, ccd_l, val;

	rank_sel = mcu->ddr_info.physical_ranks;

	/* MRS Write for DIMM initialization */
#ifdef DDR3_MODE
	/* MRS2 */
	mrs_value = config_mr2_value(mcu->ddr_info.cw_latency, mcu->mcu_ud.ud_rtt_wr, crc_enable(mcu));

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 2, 0, 0, 0);

	/* MRS3 */
	mrs_value = config_mr3_value(mcu->ddr_info.t_ck_ps, 0, 0, 0, 0); /* MRS3 DDR3 is zero */

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 3, 0, 0, 0);

	/* MRS1 */
	mrs_value = config_mr1_value(mcu->mcu_ud.ud_rtt_nom_s0, mcu->mcu_ud.ud_mr1_dic, 0, 1);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 1, 0, 0, 0);

	/* MRS0 */
	val = cal_t_wr(mcu->id, mcu->ddr_info.t_ck_ps, mcu->ddr_info.write_preamble);
	mrs_value = config_mr0_value(val, mcu->ddr_info.cas_latency);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 0, 0, 0, 0);
#else
	/* MRS3 */
	mrs_value = config_mr3_value(mcu->ddr_info.t_ck_ps, mcu->mcu_ud.ud_refresh_granularity, 0, 0, 0);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 3, 0, 0, 0);

	/* MRS6 */
	ccd_l = dmc_read_reg(mcu->id, T_RTR_NOW_ADDR);
	ccd_l = FIELD_DMC520_T_RTR_L_NOW_RD(ccd_l);

	/* Set DIMM VREFDQ: Enter train mode */
	mr6_vreftrain(mcu->id, rank_sel, 1, udp->ud_dram_vref_range,
			mcu->mcu_ud.ud_dram_vref_value, ccd_l, 0);
	DELAY(1);
	/* Set DIMM VREFDQ: Exit train mode*/
	mr6_vreftrain(mcu->id, rank_sel, 0, udp->ud_dram_vref_range,
			mcu->mcu_ud.ud_dram_vref_value, ccd_l, 0);

	/* MRS5 */
	mrs_value = config_mr5_value(mcu->mcu_ud.ud_rtt_park_s0, read_dbi_enable(mcu->id),
		write_dbi_enable(mcu->id), mcu->ddr_info.parity_latency);
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 5, 0, 0, 0);

	/* MRS4 */
	mrs_value = config_mr4_value(read_t_cal(mcu->id), mcu->ddr_info.read_preamble,
		mcu->ddr_info.write_preamble);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 4, 0, 0, 0);

	/* MRS2 */
	mrs_value = config_mr2_value(mcu->ddr_info.cw_latency, mcu->mcu_ud.ud_rtt_wr, crc_enable(mcu->id));

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 2, 0, 0, 0);

	/* MRS1 */
	mrs_value = config_mr1_value(mcu->mcu_ud.ud_rtt_nom_s0, mcu->mcu_ud.ud_mr1_dic, 0, 1);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 1, 0, 0, 0);

	/* MRS0 */
	val = cal_t_wr(mcu->id, mcu->ddr_info.t_ck_ps, mcu->ddr_info.write_preamble);
	mrs_value = config_mr0_value(val, mcu->ddr_info.cas_latency);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_value);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, rank_sel, 0, 0, 0, 0);
#endif
}

/* ZQCL Wait */
void zqcl_wait(struct apm_mcu *mcu, unsigned int zqcl_init)
{
	/* ZQC Long Wait routine */
	unsigned int cycle_count;

	/* First ZQCL waits for tZQinit i.e DDR4: 1024 cycles, DDR3: max(512nCK, 640ns) */
	/* Subsequent ZQCL wait for tZQoper i.e DDR4: 512 cycles DDR3: max(256nCK, 320ns) */
	if (zqcl_init == 0) {
		if (mcu->ddr_info.ddr_type == SPD_MEMTYPE_DDR4) {
			/* 1024 cycles */
			cycle_count = 1023;
		} else {
			/* 512 cycles */
			cycle_count = mcu->ddr_info.t_ck_ps;
			if (cycle_count < 512) {
				cycle_count = 512;
			}
		}
	} else {
		if (mcu->ddr_info.ddr_type == SPD_MEMTYPE_DDR4) {
			/* 512 cycles */
			cycle_count = 512;
		} else {
			/* 256 cycles */
			cycle_count = (320*1000)/mcu->ddr_info.t_ck_ps;
			if (cycle_count < 256) {
				cycle_count = 256;
			}
		}
	}
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, cycle_count);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_WAIT, 0, 0, 0, 0, 0);
}

/*  ZQCL Command */
void dram_zqcl(struct apm_mcu *mcu)
{
	unsigned rank_sel = 0;

	rank_sel = mcu->ddr_info.physical_ranks;

	/* DLL Lock Wait for 512 cycles */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x00200);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_WAIT, 0, 0, 0, 0, 0);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0x00000400);
	dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_ZQC, rank_sel, 0, 0, 0, 0);
	/* ZQCL Wait */
	zqcl_wait(mcu, 1);
}

/* software based ZQCS */
int dram_zqcs(unsigned int mcu_id)
{
	unsigned int rank_sel;

	rank_sel = dmc_read_reg(mcu_id, DMC520_PHYSICAL_RANK_ADDR);
	rank_sel = FIELD_DMC520_PHYSICAL_RANK_RD(rank_sel);

	/*
	 * Precharge the ranks
	 * Software can't make any assumptions of page being closed.
	 */
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_PRECHARGEALL, rank_sel, 0, 0, 0, 0);

	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, 1);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_ZQC, rank_sel, 0, 0, 0, 0);
	/* Execute Drain */
	return dmc_execute_drain(mcu_id);
}

/* DMC Flush Routine */
int dmc_flush_seq(unsigned int mcu_id)
{
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_INVALIDATE_RESET, 0, 0, 0, 0, 0);
	/* Execute Drain */
	return dmc_execute_drain(mcu_id);
}

/* Clear ECC Correctable Error counts */
void dmc_ecc_errc_clear(unsigned int mcu_id)
{
	dmc_write_reg(mcu_id, ECC_ERRC_COUNT_31_00_ADDR, 0);
	dmc_write_reg(mcu_id, ECC_ERRC_COUNT_63_32_ADDR, 0);
}

/* Clear ECC Uncorrectable Error counts */
void dmc_ecc_errd_clear(unsigned int mcu_id)
{
	dmc_write_reg(mcu_id, ECC_ERRD_COUNT_31_00_ADDR, 0);
	dmc_write_reg(mcu_id, ECC_ERRD_COUNT_63_32_ADDR, 0);
}

/* Clear RAM Error counts */
void dmc_ram_err_clear(unsigned int mcu_id)
{
	dmc_write_reg(mcu_id, RAM_ERR_COUNT_ADDR, 0);
	dmc_write_reg(mcu_id, RAM_ERR_COUNT_ADDR, 0);
}

/* Clear Link Error counts */
void dmc_link_err_clear(unsigned int mcu_id)
{
	dmc_write_reg(mcu_id, LINK_ERR_COUNT_ADDR, 0);
}

/* Setup ECC interrupt bits */
void dmc_ecc_interrupt_ctrl(unsigned int mcu_id, unsigned int ram_errc, unsigned int ram_errd,
			    unsigned int dram_errc, unsigned int dram_errd)
{
	unsigned int regd;

	regd = dmc_read_reg(mcu_id, INTERRUPT_CONTROL_ADDR);
	/* Disable RAM Correctable Error */
	regd = FIELD_DMC520_RAM_ECC_ERRC_INT_EN_SET(regd, ram_errc);
	/* Disable RAM Uncorrectable Error */
	regd = FIELD_DMC520_RAM_ECC_ERRD_INT_EN_SET(regd, ram_errd);
	/* Disable DRAM Correctable Error */
	regd = FIELD_DMC520_DRAM_ECC_ERRC_INT_EN_SET(regd, dram_errc);
	/* Disable DRAM Uncorrectable Error */
	regd = FIELD_DMC520_DRAM_ECC_ERRD_INT_EN_SET(regd, dram_errd);

	dmc_write_reg(mcu_id, INTERRUPT_CONTROL_ADDR, regd);
}

/* MCU Error Status */
unsigned int dmc_error_status(unsigned int mcu_id)
{
	unsigned int regd, errc = 0;

	regd = dmc_read_reg(mcu_id, ECC_ERRD_COUNT_31_00_ADDR);
	errc += FIELD_DMC520_RANK0_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK1_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK2_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK3_ERRD_COUNT_RD(regd);

	regd = dmc_read_reg(mcu_id, ECC_ERRD_COUNT_63_32_ADDR);
	errc += FIELD_DMC520_RANK4_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK5_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK6_ERRD_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK7_ERRD_COUNT_RD(regd);

	regd = dmc_read_reg(mcu_id, ECC_ERRC_COUNT_31_00_ADDR);
	errc += FIELD_DMC520_RANK0_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK1_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK2_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK3_ERRC_COUNT_RD(regd);

	regd = dmc_read_reg(mcu_id, ECC_ERRC_COUNT_63_32_ADDR);
	errc += FIELD_DMC520_RANK4_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK5_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK6_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RANK7_ERRC_COUNT_RD(regd);

	regd = dmc_read_reg(mcu_id, RAM_ERR_COUNT_ADDR);
	errc += FIELD_DMC520_RAM_ERRC_COUNT_RD(regd);
	errc += FIELD_DMC520_RAM_ERRD_COUNT_RD(regd);

	regd = dmc_read_reg(mcu_id, LINK_ERR_COUNT_ADDR);
	errc += FIELD_DMC520_LINK_ERR_COUNT_RD(regd);

	return errc;
}

/* Convert Physical Rank to Logical Rank */
unsigned int get_rank_mux(unsigned int mcu_id, unsigned int rank,
			  unsigned int two_dpc_enable)
{
	unsigned int cke_mux, cs_mux, cid_mask;
	unsigned int regd, rank_mux = 0;

	regd = dmc_read_reg(mcu_id, MUX_CONTROL_NOW_ADDR);

	cid_mask = FIELD_DMC520_CID_MASK_NOW_RD(regd);
	cke_mux  = FIELD_DMC520_CKE_MUX_CONTROL_NOW_RD(regd);
	cs_mux   = FIELD_DMC520_CS_MUX_CONTROL_NOW_RD(regd);


	/* DIMM 0 - 0,1,2,3  DIMM 1 - 4,5,6,7 */
	switch (cid_mask) {
	case 0:
		rank = rank;
		break;
	case 1:
		rank = rank >> 1;
		break;
	case 3:
		rank = rank >> 2;
		break;
	case 7:
		rank = rank >> 3;
		break;
	}

	/*
	 * Check CKE MUX and map ranks appropriately
	 * 2H Stack, 1R 2DPC - Special Case
	 */
	if (two_dpc_enable && (cke_mux == 1) && (cs_mux == 1)) {
		switch (rank) {
		case 0:
			rank_mux = 0;
			break;
		case 1:
			rank_mux = 1;
			break;
		case 2:
			rank_mux = 4;
			break;
		case 3:
			rank_mux = 5;
			break;
		}
	} else if (two_dpc_enable && (cke_mux == 2) && (cs_mux == 3)) {
		/* 4H Stack, 1R 2DPC - special case */
		rank_mux = rank << 2;
	} else {
		rank_mux = rank;
	}

	/* In 3DS redo the mapping */
	if (cid_mask != 0) {
		/* For 2H case */
		switch (rank_mux) {
		case 0:
			rank_mux = 0;
			break;
		case 1:
			rank_mux = 1;
			break;
		case 2:
			rank_mux = 4;
			break;
		case 3:
			rank_mux = 5;
			break;
		}
		/* TODO: For 4H case map 1 -> 4 if 2dpc_enabled */
	}
	return rank_mux;
}

/* MRS inversion */
unsigned int mrs_inversion(unsigned int mrs_mr)
{
	unsigned int mask, mask_inverted;
	unsigned int mrs_inverted;

	/* Mask for non-inverted bits */
	mask = 0x1D407;
	/* Mask for inverted bits */
	mask_inverted = ~mask & 0x3FFFF;

	mrs_inverted = ~(mrs_mr & mask_inverted) & 0x3FFFF;
	mrs_mr = mrs_mr & mask;

	return (mrs_mr | mrs_inverted);
}

/* Address Mirroring */
unsigned int address_mirroring(unsigned int address)
{
	unsigned int mirror_address = 0;

	/* Mirroring: mr3[17:14],mr3[11],mr3[12],mr3[13],mr3[10:9],mr3[7],mr3[8],mr3[5],mr3[6],mr3[3],mr3[4],mr3[2:0] */
	mirror_address = (address & 0x7);
	mirror_address |= ((address >> 4) & 0x1) << 3;
	mirror_address |= ((address >> 3) & 0x1) << 4;
	mirror_address |= ((address >> 6) & 0x1) << 5;
	mirror_address |= ((address >> 5) & 0x1) << 6;
	mirror_address |= ((address >> 8) & 0x1) << 7;
	mirror_address |= ((address >> 7) & 0x1) << 8;
	mirror_address |= ((address >> 9) & 0x3) << 9;
	mirror_address |= ((address >> 13) & 0x1) << 11;
	mirror_address |= ((address >> 12) & 0x1) << 12;
	mirror_address |= ((address >> 11) & 0x1) << 13;
	mirror_address |= ((address >> 14) & 0xF) << 14;

	return mirror_address;
}

/* Configure RCD Buffer */
void config_rcd_buffer(struct apm_mcu *mcu)
{
	unsigned int t_ck_ps, da_setting = 0;
	unsigned int rank_sel, mcu_id;

	mcu_id = mcu->id;
	t_ck_ps = mcu->ddr_info.t_ck_ps;

	/* Check for Non-UDIMM */
	if (mcu->ddr_info.package_type == UDIMM)
		return;

	/* Only One Rank per Slot */
	rank_sel = 1 | (mcu->ddr_info.two_dpc_enable << 4);

	/* MRS7 is used to program RCD Buffer */
	/* F0RC00 */
	da_setting = 0;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, (0 << 4) | da_setting);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, rank_sel, 7,
			 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* F0RC02 */
	da_setting = 0;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, (2 << 4) | da_setting);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, rank_sel, 7,
			 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* F0RC09 */
	da_setting = 0xC;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, (9 << 4) | da_setting);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, rank_sel, 7,
			 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* F0RC0A - RDIMM Operating Speed */
	if (t_ck_ps >= 2500)		/* 800 */
		da_setting = 0;
	else if (t_ck_ps >= 1875)	/* 1066 */
		da_setting = 0;
	else if (t_ck_ps >= 1499)	/* 1333 */
		da_setting = 0;
	else if (t_ck_ps >= 1250)	/* 1600 */
		da_setting = 0;
	else if (t_ck_ps >= 1071)	/* 1866 */
		da_setting = 1;
	else if (t_ck_ps >= 937)	/* 2133 */
		da_setting = 2;
	else if (t_ck_ps >= 833)	/* 2400 */
		da_setting = 3;
	else if (t_ck_ps >= 750)	/* 2666 */
		da_setting = 3;
	else if (t_ck_ps >= 625)	/* 3200 */
		da_setting = 4;
	else
		da_setting = 0;

	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, (0xA << 4) | da_setting);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, rank_sel, 7,
			 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* F0RC0D */
	da_setting = 0;
	da_setting |= ((mcu->ddr_info.package_type == RDIMM) << 2);
	da_setting |= (mcu->ddr_info.addr_mirror << 3);

	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, (0xD << 4) | da_setting);
	dmc_cmd_multiple(mcu_id, REGV_DIRECT_CMD_MRS, rank_sel, 7,
			 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);
}
