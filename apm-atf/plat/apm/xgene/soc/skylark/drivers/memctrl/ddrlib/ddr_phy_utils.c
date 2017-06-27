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

unsigned long long phy_addr(unsigned int mcu_id, unsigned int reg)
{
	unsigned int phyl_rb_base, phyh_rb_base;
	unsigned long long sys_addr;

	phyl_rb_base = PCP_RB_MCU0_PHYL_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;
	phyh_rb_base = PCP_RB_MCU0_PHYH_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	if (reg < 0x1000)
		sys_addr = PCP_RB_BASE + (phyl_rb_base << 16) + reg;
	else
		sys_addr = PCP_RB_BASE + (phyh_rb_base << 16) + (reg & 0xFFF);

	return sys_addr;
}

unsigned int phy_read_reg(unsigned int mcu_id, unsigned int reg)
{
	unsigned int data;
	unsigned int phyl_rb_base, phyh_rb_base;
	unsigned long long sys_addr;

	phyl_rb_base = PCP_RB_MCU0_PHYL_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;
	phyh_rb_base = PCP_RB_MCU0_PHYH_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	if (reg < 0x1000)
		sys_addr = PCP_RB_BASE + (phyl_rb_base << 16) + reg;
	else
		sys_addr = PCP_RB_BASE + (phyh_rb_base << 16) + (reg & 0xFFF);

	data = le32toh(*(volatile unsigned int *)(sys_addr));
	ddr_debug("MCU-PHY[%d]: read 0x%llX value 0x%08X\n",mcu_id, sys_addr, data);
	return data;
}

void phy_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value)
{
	unsigned long long sys_addr;
	unsigned int phyl_rb_base, phyh_rb_base;

	phyl_rb_base = PCP_RB_MCU0_PHYL_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;
	phyh_rb_base = PCP_RB_MCU0_PHYH_PAGE + mcu_id * PCP_RB_MCUX_OFFSET;

	if (reg < 0x1000)
		sys_addr = PCP_RB_BASE + (phyl_rb_base << 16) + reg;
	else
		sys_addr = PCP_RB_BASE + (phyh_rb_base << 16) + (reg & 0xFFF);

	ddr_debug("MCU-PHY[%d]: write 0x%llX value 0x%08X\n",mcu_id, sys_addr, value);

	*(volatile unsigned int *)(sys_addr) = htole32(value);
}

int phy_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value, unsigned int mask)
{
	unsigned int rd_val;
	unsigned int polling_on;
	unsigned int err = 0;
	unsigned int timeout = 0x1000;

	ddr_debug("MCU-PHY[%d]: Polling for 0x%x\n", mcu_id, value);

	polling_on = 1;
	while (polling_on) {
		rd_val = phy_read_reg(mcu_id, reg);
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
#if XGENE_VHP
		polling_on = 0;
#endif
	}
	return err;
}

/* Disable/Enable Periodic IO Calibration */
void control_periodic_io_cal(unsigned int mcu_id, int set)
{
	unsigned int regd;

	/* PHY_CAL_MODE_ON */
	regd = phy_read_reg(mcu_id, MCU_PHY_CAL_MODE_0_ADDR);
	regd = (regd & 0xFDFFFFFF) | ((set & 0x1) << 25);
	phy_write_reg(mcu_id, MCU_PHY_CAL_MODE_0_ADDR, regd);
}

/* TODO: to be implemented */
void mcu_phy_rdfifo_reset(unsigned int mcu_id)
{
}

void phy_vref_ctrl(unsigned int mcu_id, unsigned int vref_range, unsigned int vref_value)
{
	unsigned int regd, vref_ctrl;

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_VREF_CTRL_ADDR);
	/* VREF CTRL PARAMs:
	 * [12:9]= DDR3/4 mode + range
	 * [8] = Enable Vref
	 * [5:0] = vref value
	 */
	vref_ctrl = ((0x4 + vref_range) << 9) | (0x1 << 8) | vref_value;
	regd = FIELD_MCU_PHY_PAD_VREF_CTRL_SET(regd, vref_ctrl);
	phy_write_reg(mcu_id, MCU_PHY_PAD_VREF_CTRL_ADDR, regd);
}

/******************************************************************************
 *     Phy Training Status functions
 *****************************************************************************/
/* Phy Training Status functions */
int mcu_check_wrlvl_obs(unsigned int mcu_id, unsigned int rank)
{
	int unsigned slice, offset;
	int err = 0;
	unsigned int regd[4];
	ddr_verbose("MCU-PHY[%d]: INFO	Mcu PEVM Wr Level status rank=%1d\n", mcu_id, rank);
	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		offset = slice * PHY_SLICE_OFFSET;

		regd[0] = phy_read_reg(mcu_id, MCU_PHY_WRLVL_HARD0_DELAY_OBS_0_ADDR + offset);
		regd[1] = phy_read_reg(mcu_id, MCU_PHY_WRLVL_HARD1_DELAY_OBS_0_ADDR + offset);
		regd[2] = phy_read_reg(mcu_id, MCU_PHY_WRLVL_STATUS_OBS_0_ADDR + offset);
		regd[3] = phy_read_reg(mcu_id, MCU_PHY_WRLVL_ERROR_OBS_0_ADDR + offset);

		ddr_verbose("Wrlvl lane=%01d hard-0=0x%03x hard-1=0x%03x status=0x%04x err=0x%08x\n",
			    slice, FIELD_MCU_PHY_WRLVL_HARD0_DELAY_OBS_0_RD(regd[0]),
			    FIELD_MCU_PHY_WRLVL_HARD1_DELAY_OBS_0_RD(regd[1]),
			    FIELD_MCU_PHY_WRLVL_STATUS_OBS_0_RD(regd[2]),
			    FIELD_MCU_PHY_WRLVL_ERROR_OBS_0_RD(regd[3]));

		if (regd[3] != 0)
			err = -1;
	}
	return err;
}

int mcu_check_rdgate_obs(unsigned int mcu_id, unsigned int rank)
{
	int unsigned slice;
	int err = 0;
	unsigned int regd[3], offset;
	ddr_verbose("MCU-PHY[%d]: INFO	Mcu PEVM Rd Gate status rank=%1d\n", mcu_id, rank);
	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		offset = slice * PHY_SLICE_OFFSET;
		regd[0] = phy_read_reg(mcu_id, MCU_PHY_GTLVL_HARD0_DELAY_OBS_0_ADDR + offset);
		regd[1] = phy_read_reg(mcu_id, MCU_PHY_GTLVL_HARD1_DELAY_OBS_0_ADDR + offset);
		regd[2] = phy_read_reg(mcu_id, MCU_PHY_GTLVL_STATUS_OBS_0_ADDR + offset);

		ddr_verbose("RdGt lane=%01d hard-0=0x%03x hard-1=0x%03x status=0x%04x\n",
			     slice, FIELD_MCU_PHY_GTLVL_HARD0_DELAY_OBS_0_RD(regd[0]),
			     FIELD_MCU_PHY_GTLVL_HARD1_DELAY_OBS_0_RD(regd[1]),
			     FIELD_MCU_PHY_GTLVL_STATUS_OBS_0_RD(regd[2]));

		/*
 		 * Bit[10:9] - gtlvl_error for upper nibble
 		 * Bit[7:6] - gtlvl_error for lower nibble
 		 */
		if (((regd[2] & 0xC0) != 0) || ((regd[2] & 0x600) != 0))
			err = -1;
	}
	return err;
}

int mcu_check_rdlvl_obs(unsigned int mcu_id, unsigned int rank)
{
	int unsigned slice, dq_bit;
	int err = 0;
	unsigned int regd[3], offset, data;
	ddr_verbose("MCU-PHY[%d]: INFO	Mcu PEVM Rd Lvl status rank=%1d\n",
		   mcu_id, rank);
	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		offset = slice * PHY_SLICE_OFFSET;

		for (dq_bit = 0; dq_bit < 8; dq_bit++) {
			data = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_OBS_SELECT_0_ADDR + offset);
			data = FIELD_MCU_PHY_RDLVL_RDDQS_DQ_OBS_SELECT_0_SET(data, dq_bit);
			phy_write_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_OBS_SELECT_0_ADDR + offset, data);

			regd[0] = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_LE_DLY_OBS_0_ADDR + offset);
			regd[1] = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_TE_DLY_OBS_0_ADDR + offset);
			regd[2] = phy_read_reg(mcu_id, MCU_PHY_RDLVL_STATUS_OBS_0_ADDR + offset);

			ddr_verbose("RdLvl lane=%01d DQ[%01d] lead=0x%03x trail=0x%03x status=0x%04x\n",
				    slice, dq_bit, FIELD_MCU_PHY_RDLVL_RDDQS_DQ_LE_DLY_OBS_0_RD(regd[0]),
				    FIELD_MCU_PHY_RDLVL_RDDQS_DQ_TE_DLY_OBS_0_RD(regd[1]),
				    FIELD_MCU_PHY_RDLVL_STATUS_OBS_0_RD(regd[2]));
		}
			/*
		 	 * Bits [23:16] = DQ [7:0] fail flag Bit[15:8] - rdlvl_dqZ_te_found
		 	 * Bit[7:0] - rdlvl_dqZ_le_found
		 	 */
			if (((regd[2] & 0xFF) != 0xFF) || ((regd[2] & 0xFF00) != 0xFF00) ||
		   	    ((regd[2] & 0xFF0000) != 0))
				err = -1;
	}
	return err;
}

/* Phy training utility functions */
/* Configure Per CS Training Index */
void config_per_cs_training_index(unsigned int mcu_id, unsigned int rank,
				  unsigned int per_cs_training_en)
{
	unsigned int jj, offset, regd;

	/* Set Target Rank PHY CSRs per slice */
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		offset = jj * PHY_SLICE_OFFSET;

		/* Update Per CS Training En */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_EN_0_ADDR + offset);
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_EN_0_SET(regd, per_cs_training_en);
		phy_write_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_EN_0_ADDR + offset, regd);

		/* Training Index */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_INDEX_0_ADDR + offset);
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_INDEX_0_SET(regd, rank);
		phy_write_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_INDEX_0_ADDR + offset, regd);

		/* Disable MultiCast */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset);
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_SET(regd, 0);
		phy_write_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset, regd);
	}
}

/* Update Per CS Training Index */
void update_per_cs_training_index(unsigned int mcu_id, unsigned int rank)
{
	/* Single rank mode training (pick target rank - number 0 thru 7) */
	/* Setup Phy Per CS Training Index */
	unsigned int jj, regd, offset;

	/* Set Target Rank PHY CSRs per slice */
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		offset = jj * PHY_SLICE_OFFSET;

		/* Training Index */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_INDEX_0_ADDR + offset);
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_INDEX_0_SET(regd, rank);
		phy_write_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_INDEX_0_ADDR + offset, regd);

		/* Disable MultiCast */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset);
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_SET(regd, 0);
		phy_write_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset, regd);
	}
}

unsigned int get_per_cs_training_en(unsigned int mcu_id)
{
	unsigned int jj, offset, regd, enable = 1;

	/* Set Target Rank PHY CSRs per slice */
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		offset = jj * PHY_SLICE_OFFSET;

		/* Update Per CS Training En */
		regd = phy_read_reg(mcu_id, MCU_PHY_PER_CS_TRAINING_EN_0_ADDR + offset);
		enable &= FIELD_MCU_PHY_PER_CS_TRAINING_EN_0_RD(regd);
	}
	return enable;
}

/* Train results & Obs */

/* Drive Strength / Slew rate / Termination Control (int-ODT)*/
void mcu_phy_peek_pad_drv_term(unsigned int mcu_id, phy_pad_drv_term_t * datast)
{
	unsigned int regd;
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_FDBK_DRIVE_ADDR);
	datast->phy_pad_fdbk_drive = FIELD_MCU_PHY_PAD_FDBK_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_DRIVE_ADDR);
	datast->phy_pad_x4_fdbk_drive = FIELD_MCU_PHY_PAD_X4_FDBK_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DATA_DRIVE_ADDR);
	datast->phy_pad_data_drive = FIELD_MCU_PHY_PAD_DATA_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DQS_DRIVE_ADDR);
	datast->phy_pad_dqs_drive = FIELD_MCU_PHY_PAD_DQS_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ADDR_DRIVE_ADDR);
	datast->phy_pad_addr_drive = FIELD_MCU_PHY_PAD_ADDR_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_CLK_DRIVE_ADDR);
	datast->phy_pad_clk_drive = FIELD_MCU_PHY_PAD_CLK_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DATA_TERM_ADDR);
	datast->phy_pad_data_term = FIELD_MCU_PHY_PAD_DATA_TERM_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DQS_TERM_ADDR);
	datast->phy_pad_dqs_term = FIELD_MCU_PHY_PAD_DQS_TERM_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ADDR_TERM_ADDR);
	datast->phy_pad_addr_term = FIELD_MCU_PHY_PAD_ADDR_TERM_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_CLK_TERM_ADDR);
	datast->phy_pad_clk_term = FIELD_MCU_PHY_PAD_CLK_TERM_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_PAR_DRIVE_ADDR);
	datast->phy_pad_par_drive = FIELD_MCU_PHY_PAD_PAR_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ERR_DRIVE_ADDR);
	datast->phy_pad_err_drive = FIELD_MCU_PHY_PAD_ERR_DRIVE_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ATB_CTRL_ADDR);
	datast->phy_pad_atb_ctrl = FIELD_MCU_PHY_PAD_ATB_CTRL_RD(regd);
}

/* Write Level */
void mcu_phy_peek_wrlvl_obs_sl(unsigned int mcu_id, phy_slice_train_obs_t * datast,
			       unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_WRDQS_BASE_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_wrdqs_base_slv_dly_enc_obs = FIELD_MCU_PHY_WRDQS_BASE_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRDQ_BASE_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_wrdq_base_slv_dly_enc_obs = FIELD_MCU_PHY_WRDQ_BASE_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WR_ADDER_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_wr_adder_slv_dly_enc_obs = FIELD_MCU_PHY_WR_ADDER_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_HARD0_DELAY_OBS_0_ADDR + offset);
	datast->phy_wrlvl_hard0_delay_obs = FIELD_MCU_PHY_WRLVL_HARD0_DELAY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_HARD1_DELAY_OBS_0_ADDR + offset);
	datast->phy_wrlvl_hard1_delay_obs = FIELD_MCU_PHY_WRLVL_HARD1_DELAY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_STATUS_OBS_0_ADDR + offset);
	datast->phy_wrlvl_status_obs = FIELD_MCU_PHY_WRLVL_STATUS_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_ERROR_OBS_0_ADDR + offset);
	datast->phy_wrlvl_err_obs = FIELD_MCU_PHY_WRLVL_ERROR_OBS_0_RD(regd);
}

void mcu_phy_peek_wrlvl_obs_rnk(unsigned int mcu_id, unsigned int rank,
				phy_rnk_train_obs_t * datast)
{
	unsigned int jj;
	phy_slice_train_obs_t * psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_obs_t *)&datast->sliced[jj];
		mcu_phy_peek_wrlvl_obs_sl(mcu_id,psub,jj);
	}
}


/* Results Wr Lvl */
void mcu_phy_peek_wrlvl_res_sl(unsigned int mcu_id, unsigned int device_type,
			       phy_slice_train_res_t * datast, unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[0] = FIELD_MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[1] = FIELD_MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[2] = FIELD_MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[3] = FIELD_MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[4] = FIELD_MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[5] = FIELD_MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[6] = FIELD_MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqZ_slave_delay[7] = FIELD_MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDM_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdm_slave_delay = FIELD_MCU_PHY_CLK_WRDM_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQS_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_clk_wrdqs_slave_delay = FIELD_MCU_PHY_CLK_WRDQS_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRITE_PATH_LAT_ADD_0_ADDR + offset);
	datast->phy_write_path_lat_add = FIELD_MCU_PHY_WRITE_PATH_LAT_ADD_0_RD(regd);

	if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4) {
		regd = phy_read_reg(mcu_id, MCU_PHY_X4_CLK_WRDQS_SLAVE_DELAY_0_ADDR + offset);
		datast->phy_x4_clk_wrdqs_slave_delay =
			FIELD_MCU_PHY_X4_CLK_WRDQS_SLAVE_DELAY_0_RD(regd);

		regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_ADDR + offset);
		datast->phy_x4_write_path_lat_add = FIELD_MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_RD(regd);
	}
}

void mcu_phy_peek_wrlvl_res_rnk(unsigned int mcu_id, unsigned int rank,
				unsigned int device_type,
				phy_rnk_train_res_t * datast)
{
	unsigned int jj;
	phy_slice_train_res_t *psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_res_t *)&datast->sliced[jj];
		mcu_phy_peek_wrlvl_res_sl(mcu_id, device_type, psub, jj);
	}
}

/* WrLvl Start Peek */
void mcu_phy_peek_wrlvl_start_sl(unsigned int mcu_id, unsigned int device_type,
				 phy_slice_train_res_t * datast, unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_WRITE_PATH_LAT_ADD_0_ADDR + offset);
	datast->phy_write_path_lat_add = FIELD_MCU_PHY_WRITE_PATH_LAT_ADD_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR + offset);
	datast->phy_wrlvl_delay_early_threshold =
		FIELD_MCU_PHY_WRLVL_DELAY_EARLY_THRESHOLD_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR + offset);
	datast->phy_wrlvl_early_force = FIELD_MCU_PHY_WRLVL_EARLY_FORCE_0_0_RD(regd);
	datast->phy_wrlvl_delay_period_threshold =
		FIELD_MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_RD(regd);

	if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4) {
		regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_ADDR + offset);
		datast->phy_x4_write_path_lat_add = FIELD_MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_RD(regd);
		datast->phy_x4_wrlvl_delay_early_threshold =
			FIELD_MCU_PHY_X4_WRLVL_DELAY_EARLY_THRESHOLD_0_RD(regd);

		regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRLVL_EARLY_FORCE_0_0_ADDR + offset);
		datast->phy_x4_wrlvl_early_force = FIELD_MCU_PHY_X4_WRLVL_EARLY_FORCE_0_0_RD(regd);
		datast->phy_x4_wrlvl_delay_period_threshold =
			FIELD_MCU_PHY_X4_WRLVL_DELAY_PERIOD_THRESHOLD_0_RD(regd);
	}
}

void mcu_phy_peek_wrlvl_start_rnk(unsigned int mcu_id, unsigned int rank,
				unsigned int device_type,
				phy_rnk_train_res_t * datast)
{
	unsigned int jj;
	phy_slice_train_res_t *psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_res_t *)&datast->sliced[jj];
		mcu_phy_peek_wrlvl_start_sl(mcu_id, device_type, psub, jj);
	}
}

/* Rd Gate */
void mcu_phy_peek_rdgate_obs_sl(unsigned int mcu_id, phy_slice_train_obs_t * datast,
				unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_HARD0_DELAY_OBS_0_ADDR + offset);
	datast->phy_gtlvl_hard0_delay_obs = FIELD_MCU_PHY_GTLVL_HARD0_DELAY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_HARD1_DELAY_OBS_0_ADDR + offset);
	datast->phy_gtlvl_hard1_delay_obs = FIELD_MCU_PHY_GTLVL_HARD1_DELAY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_STATUS_OBS_0_ADDR + offset);
	datast->phy_gtlvl_status_obs = FIELD_MCU_PHY_GTLVL_STATUS_OBS_0_RD(regd);
}

void mcu_phy_peek_rdgate_obs_rnk(unsigned int mcu_id, unsigned int rank,
				phy_rnk_train_obs_t * datast)
{
	unsigned int jj;
	phy_slice_train_obs_t * psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_obs_t *)&datast->sliced[jj];
		mcu_phy_peek_rdgate_obs_sl(mcu_id,psub,jj);
	}
}


/* Results Peek */
void mcu_phy_peek_rdgate_res_sl(unsigned int mcu_id, unsigned int device_type,
				phy_slice_train_res_t * datast, unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ0_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[0] = FIELD_MCU_PHY_RDDQ0_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ1_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[1] = FIELD_MCU_PHY_RDDQ1_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ2_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[2] = FIELD_MCU_PHY_RDDQ2_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ3_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[3] = FIELD_MCU_PHY_RDDQ3_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ4_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[4] = FIELD_MCU_PHY_RDDQ4_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ5_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[5] = FIELD_MCU_PHY_RDDQ5_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ6_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[6] = FIELD_MCU_PHY_RDDQ6_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ7_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqZ_slave_delay[7] = FIELD_MCU_PHY_RDDQ7_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_GATE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_gate_slave_delay = FIELD_MCU_PHY_RDDQS_GATE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_LATENCY_ADJUST_0_ADDR + offset);
	datast->phy_rddqs_latency_adjust = FIELD_MCU_PHY_RDDQS_LATENCY_ADJUST_0_RD(regd);

	if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4) {
		regd = phy_read_reg(mcu_id, MCU_PHY_X4_RDDQS_GATE_SLAVE_DELAY_0_ADDR + offset);
		datast->phy_x4_rddqs_gate_slave_delay =
			FIELD_MCU_PHY_X4_RDDQS_GATE_SLAVE_DELAY_0_RD(regd);

		regd = phy_read_reg(mcu_id, MCU_PHY_X4_RDDQS_LATENCY_ADJUST_0_ADDR + offset);
		datast->phy_x4_rddqs_latency_adjust =
			FIELD_MCU_PHY_X4_RDDQS_LATENCY_ADJUST_0_RD(regd);
	}
}

void mcu_phy_peek_rdgate_res_rnk(unsigned int mcu_id, unsigned int rank,
				unsigned int device_type,
				phy_rnk_train_res_t * datast)
{
	unsigned int jj;
	phy_slice_train_res_t * psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_res_t *)&datast->sliced[jj];
		mcu_phy_peek_rdgate_res_sl(mcu_id, device_type, psub, jj);
	}
}

/* OBS */
void mcu_phy_peek_rdeye_obs_sl(unsigned int mcu_id, phy_slice_train_obs_t * datast,
			       unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQ_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_rddq_slv_dly_enc_obs = FIELD_MCU_PHY_RDDQ_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_BASE_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_rddqs_base_slv_dly_enc_obs = FIELD_MCU_PHY_RDDQS_BASE_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_GATE_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_rddqs_gate_slv_dly_enc_obs = FIELD_MCU_PHY_RDDQS_GATE_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ_RISE_ADDER_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_rddqs_dq_rise_adder_slv_dly_enc_obs =
		FIELD_MCU_PHY_RDDQS_DQ_RISE_ADDER_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ_FALL_ADDER_SLV_DLY_ENC_OBS_0_ADDR + offset);
	datast->phy_rddqs_dq_fall_adder_slv_dly_enc_obs =
		FIELD_MCU_PHY_RDDQS_DQ_FALL_ADDER_SLV_DLY_ENC_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_LE_DLY_OBS_0_ADDR + offset);
	datast->phy_rdlvl_rddqs_dq_le_dly_obs = FIELD_MCU_PHY_RDLVL_RDDQS_DQ_LE_DLY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_TE_DLY_OBS_0_ADDR + offset);
	datast->phy_rdlvl_rddqs_dq_te_dly_obs = FIELD_MCU_PHY_RDLVL_RDDQS_DQ_TE_DLY_OBS_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_STATUS_OBS_0_ADDR + offset);
	datast->phy_rdlvl_status_obs = FIELD_MCU_PHY_RDLVL_STATUS_OBS_0_RD(regd);
}	/* mcu_phy_peek_rdeye_obs_sl */

void mcu_phy_peek_rdeye_obs_rnk(unsigned int mcu_id, unsigned int rank,
				phy_rnk_train_obs_t * datast)
{
	unsigned int jj;
	phy_slice_train_obs_t * psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_obs_t *)&datast->sliced[jj];
		mcu_phy_peek_rdeye_obs_sl(mcu_id,psub,jj);
	}
}


/* Results Peek */
void mcu_phy_peek_rdeye_res_sl(unsigned int mcu_id, phy_slice_train_res_t * datast,
			       unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DM_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dm_fall_slave_delay = FIELD_MCU_PHY_RDDQS_DM_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DM_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dm_rise_slave_delay = FIELD_MCU_PHY_RDDQS_DM_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ0_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[0] =
		FIELD_MCU_PHY_RDDQS_DQ0_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ0_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[0] =
		FIELD_MCU_PHY_RDDQS_DQ0_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ1_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[1] =
		FIELD_MCU_PHY_RDDQS_DQ1_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ1_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[1] =
		FIELD_MCU_PHY_RDDQS_DQ1_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ2_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[2] =
		FIELD_MCU_PHY_RDDQS_DQ2_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ2_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[2] =
		FIELD_MCU_PHY_RDDQS_DQ2_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ3_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[3] =
		FIELD_MCU_PHY_RDDQS_DQ3_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ3_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[3] =
		FIELD_MCU_PHY_RDDQS_DQ3_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ4_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[4] =
		FIELD_MCU_PHY_RDDQS_DQ4_RISE_SLAVE_DELAY_0_RD(regd);
	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ4_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[4] =
		FIELD_MCU_PHY_RDDQS_DQ4_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ5_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[5] =
		FIELD_MCU_PHY_RDDQS_DQ5_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ5_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[5] =
		FIELD_MCU_PHY_RDDQS_DQ5_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ6_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[6] =
		FIELD_MCU_PHY_RDDQS_DQ6_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ6_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[6] =
		FIELD_MCU_PHY_RDDQS_DQ6_FALL_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ7_RISE_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_rise_slave_delay[7] =
		FIELD_MCU_PHY_RDDQS_DQ7_RISE_SLAVE_DELAY_0_RD(regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_DQ7_FALL_SLAVE_DELAY_0_ADDR + offset);
	datast->phy_rddqs_dqZ_fall_slave_delay[7] =
		FIELD_MCU_PHY_RDDQS_DQ7_FALL_SLAVE_DELAY_0_RD(regd);
}

void mcu_phy_peek_rdeye_res_rnk(unsigned int mcu_id, unsigned int rank,
				phy_rnk_train_res_t * datast)
{
	unsigned int jj;
	phy_slice_train_res_t * psub;

	/* Update CS Training Index as per target Rank */
	update_per_cs_training_index(mcu_id, rank);
	datast->ranknum = rank;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_slice_train_res_t *)&datast->sliced[jj];
		mcu_phy_peek_rdeye_res_sl(mcu_id,psub,jj);
	}
}

/******************************************************************************
 *     dq/dqs ie timing
 *****************************************************************************/
/* DQ/DQS IE Timing */
void mcu_phy_peek_dq_ie_timing_sl(unsigned int mcu_id, phy_dq_dqs_ie_oe_slice_t * datast,
				  unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_IE_TIMING_0_ADDR + offset);
	regd = FIELD_MCU_PHY_DQ_IE_TIMING_0_RD(regd);
	datast->dq_start_half_cycle = regd & 0x1;
	datast->dq_start_full_cycle = (regd >> 1) & 0x7;
	datast->dq_end_half_cycle = (regd >> 4) & 0x1;
	datast->dq_end_full_cycle = (regd >> 5) & 0x7;
}	/* mcu_phy_peek_dq_ie_timing_sl */

void mcu_phy_peek_dq_ie_timing_all(unsigned int mcu_id, phy_dq_dqs_ie_oe_timing_all_t * datast)
{
	unsigned int jj;
	phy_dq_dqs_ie_oe_slice_t * psub;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_dq_dqs_ie_oe_slice_t *)&datast->sliced[jj];
		mcu_phy_peek_dq_ie_timing_sl(mcu_id,psub,jj);
	}
}	/* mcu_phy_peek_dq_ie_timing__all */

/* DQS IE Timing */
void mcu_phy_peek_dqs_ie_timing_sl(unsigned int mcu_id, phy_dq_dqs_ie_oe_slice_t * datast,
				   unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_IE_TIMING_0_ADDR + offset);
	regd = FIELD_MCU_PHY_DQS_IE_TIMING_0_RD(regd);
	datast->dqs_start_half_cycle = regd & 0x1;
	datast->dqs_start_full_cycle = (regd >> 1) & 0x7;
	datast->dqs_end_half_cycle = (regd >> 4) & 0x1;
	datast->dqs_end_full_cycle = (regd >> 5) & 0x7;
}	/* mcu_phy_peek_dq_ie_timing_sl */

void mcu_phy_peek_dqs_ie_timing_all(unsigned int mcu_id, phy_dq_dqs_ie_oe_timing_all_t * datast)
{
	unsigned int jj;
	phy_dq_dqs_ie_oe_slice_t * psub;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_dq_dqs_ie_oe_slice_t *)&datast->sliced[jj];
		mcu_phy_peek_dqs_ie_timing_sl(mcu_id,psub,jj);
	}
}

/* DQ/DQS OE Timing */
void mcu_phy_peek_dq_oe_timing_sl(unsigned int mcu_id, phy_dq_dqs_ie_oe_slice_t * datast,
				  unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_OE_TIMING_0_ADDR + offset);
	regd = FIELD_MCU_PHY_DQ_OE_TIMING_0_RD(regd);
	datast->dq_start_half_cycle = regd & 0x1;
	datast->dq_start_full_cycle = (regd >> 1) & 0x7;
	datast->dq_end_half_cycle = (regd >> 4) & 0x1;
	datast->dq_end_full_cycle = (regd >> 5) & 0x7;
}	/* mcu_phy_peek_dq_oe_timing_sl */

void mcu_phy_peek_dq_oe_timing_all(unsigned int mcu_id, phy_dq_dqs_ie_oe_timing_all_t * datast)
{
	unsigned int jj;
	phy_dq_dqs_ie_oe_slice_t * psub;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_dq_dqs_ie_oe_slice_t *)&datast->sliced[jj];
		mcu_phy_peek_dq_oe_timing_sl(mcu_id,psub,jj);
	}
}

/*     DQS	*/
void mcu_phy_peek_dqs_oe_timing_sl(unsigned int mcu_id, phy_dq_dqs_ie_oe_slice_t * datast,
				   unsigned int slice)
{
	unsigned int regd, offset;

	offset = slice * PHY_SLICE_OFFSET;
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_OE_TIMING_0_ADDR + offset);
	regd = FIELD_MCU_PHY_DQS_OE_TIMING_0_RD(regd);
	datast->dqs_start_half_cycle = regd & 0x1;
	datast->dqs_start_full_cycle = (regd >> 1) & 0x7;
	datast->dqs_end_half_cycle = (regd >> 4) & 0x1;
	datast->dqs_end_full_cycle = (regd >> 5) & 0x7;
}

void mcu_phy_peek_dqs_oe_timing_all(unsigned int mcu_id, phy_dq_dqs_ie_oe_timing_all_t * datast)
{
	unsigned int jj;
	phy_dq_dqs_ie_oe_slice_t * psub;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_dq_dqs_ie_oe_slice_t *)&datast->sliced[jj];
		mcu_phy_peek_dqs_oe_timing_sl(mcu_id,psub,jj);
	}
}

/* Master_Delay_Lock (Read Only) */
void mcu_phy_peek_master_delay_lock_sl(unsigned int mcu_id, phy_master_delay_lock_vals_t * datast,
				       unsigned int slice)
{
	unsigned int regd = 0, regk, kkc, offset;

	offset = slice * PHY_SLICE_OFFSET;
	regk = phy_read_reg(mcu_id, MCU_PHY_MASTER_DLY_LOCK_OBS_SELECT_0_ADDR + offset);
	for (kkc = 0; kkc < 7 ; kkc++) {
		regk = FIELD_MCU_PHY_MASTER_DLY_LOCK_OBS_SELECT_0_SET(regd, kkc & 0x7);
		phy_write_reg(mcu_id, MCU_PHY_MASTER_DLY_LOCK_OBS_SELECT_0_ADDR + offset, regk);

		regd = phy_read_reg(mcu_id, MCU_PHY_MASTER_DLY_LOCK_OBS_0_ADDR + offset);
		regd = FIELD_MCU_PHY_MASTER_DLY_LOCK_OBS_0_RD(regd);
		datast->phy_qtr_d[kkc]	= regd & 0x3;
		datast->phy_full_d[kkc] = (regd >> 2) & 0xFF;
	}
}

void mcu_phy_peek_master_delay_lock_all(unsigned int mcu_id, phy_all_mstr_dly_lock_t * datast)
{
	unsigned int jj;
	phy_master_delay_lock_vals_t * psub;
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		psub = (phy_master_delay_lock_vals_t *)&datast->sliced[jj];
		mcu_phy_peek_master_delay_lock_sl(mcu_id, psub, jj);
	}
}

unsigned int calc_partial_RDDQS_start(unsigned int cperiod, unsigned int remndr)
{
	/* Add up the Clock delays (incl cumulative per slice fly by delay) */
	/* and slices DQ delays for the slot */
	unsigned int npc;
	unsigned int fract_part;
	fract_part = (remndr * 100) / cperiod;
	if (fract_part < 20)
		npc = 0;
	else if (fract_part < 50)
		npc = 0x80;
	else if (fract_part < 70)
		npc = 0x100;
	else if (fract_part < 90)
		npc = 0x140;
	else
		npc = 0x160;
	return npc;
}

/* Write to all Slices */
void phy_wr_all_data_slices(unsigned int mcu_id, unsigned int addr,  unsigned int regd)
{
	unsigned int slice;

	for (slice = 0; slice < 9; slice++)
		phy_write_reg(mcu_id, addr + slice * PHY_SLICE_OFFSET, regd);
}


/* Software based RX Calibration Setup */
void phy_sw_rx_cal_setup(unsigned int mcu_id, unsigned int offset)
{
	unsigned int regd;

	/* Set Calibration override PHY_RX_CAL_OVERRIDE_X = 1'b1 */
	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OVERRIDE_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_OVERRIDE_0_SET(regd, 1);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_OVERRIDE_0_ADDR + offset, regd);

	/* Set I/O Pad input enable PHY_IE_MODE_X = 2’b01 */
	regd = phy_read_reg(mcu_id, MCU_PHY_IE_MODE_0_ADDR + offset);
	regd = FIELD_MCU_PHY_IE_MODE_0_SET(regd, 1);
	phy_write_reg(mcu_id, MCU_PHY_IE_MODE_0_ADDR + offset, regd);

	/* DSB, ISB for RX CAL OVERRIDE update */
	DSB_SY_CALL;
	ISB;

	/* Set calibration code PHY_RX_CAL_DQx/DQS/DM/FDBK_X[11:0] = 12’b11111_111111 */
	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ0_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ1_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ2_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ3_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ4_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ5_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ6_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQ7_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DM_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_DQS_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_FDBK_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR + offset, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_X4_FDBK_0_SET(regd, PHY_RX_CAL_START_VALUE);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR + offset, regd);

	DSB_SY_CALL;
        ISB;
}

/* Read RX Cal Value */
unsigned int get_rx_cal(unsigned int index, unsigned int regd)
{
	unsigned int rx_cal = 0;

	switch(index) {
	case PHY_RX_CAL_OBS_DQ0_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ0_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ1_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ1_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ2_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ2_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ3_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ3_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ4_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ4_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ5_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ5_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ6_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ6_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQ7_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ7_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DM_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DM_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_DQS_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQS_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_FDBK_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_FDBK_0_RD(regd);
		break;
	case PHY_RX_CAL_OBS_X4_FDBK_BIT:
		rx_cal = FIELD_MCU_PHY_RX_CAL_X4_FDBK_0_RD(regd);
		break;
	}
	return rx_cal;
}

/* Write RX Cal Value */
unsigned int set_rx_cal(unsigned int index, unsigned int regd, unsigned int rx_cal)
{
	switch(index) {
	case PHY_RX_CAL_OBS_DQ0_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ0_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ1_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ1_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ2_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ2_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ3_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ3_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ4_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ4_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ5_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ5_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ6_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ6_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQ7_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQ7_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DM_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DM_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_DQS_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_DQS_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_FDBK_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_FDBK_0_SET(regd, rx_cal);
		break;
	case PHY_RX_CAL_OBS_X4_FDBK_BIT :
		rx_cal = FIELD_MCU_PHY_RX_CAL_X4_FDBK_0_SET(regd, rx_cal);
		break;
	}
	return rx_cal;
}

/* Software based RX Calibration */
int phy_sw_rx_cal_up_code(unsigned int mcu_id, unsigned int addr,
			  unsigned int index, unsigned int slice)
{
	unsigned int rx_obs, rx_cal, rx_cal_result;
	unsigned int cal_up, cal_down, cal_value, offset, cal_result;

	offset = slice * PHY_SLICE_OFFSET;
	/*
	 * Increment RX_CAL_CODE_UP, Decrement RX_CAL_CODE_DOWN
	 * Set new calibration code PHY_RX_CAL_DQx/DQS/DM/FDBK_X[11:0]
	 * Examine result PHY_RX_CAL_OBS_X[11:0] for appropriate DQx/DQS/DM/FDBK pad
	 * Result = 1’b1, go back to sw_rx_cal_up_code, else done
	 */
	rx_cal = phy_read_reg(mcu_id, addr + offset);
	cal_value = get_rx_cal(index, rx_cal);

	cal_down = cal_value & 0x3F;
	cal_up = (cal_value >> 6) & 0x3F;

	for (int i = PHY_RX_CAL_MIN_VALUE; i < PHY_RX_CAL_MAX_VALUE; i++) {
		/* Index for appropriate bit in OBS Reg */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << index);

		rx_cal = phy_read_reg(mcu_id, addr + offset);
		cal_value = get_rx_cal(index, rx_cal);

		cal_down = cal_value & 0x3F;
		cal_up = (cal_value >> 6) & 0x3F;

		if ((cal_up == 0x3E) && (cal_down == 0)) {
			ddr_verbose("MCU-PHY[%d]: Slice: %d Addr: 0x%X \
				     RX Cal Up Saturate: cal_up: 0x%X cal_down: 0x%X\n",
				    mcu_id, slice, addr, cal_up, cal_down);
			return rx_cal;
		} else if (rx_cal_result) {
			cal_up = (cal_up + 1) & 0x3F;
			cal_down = (cal_down - 1) & 0x3F;

			cal_result = cal_down | (cal_up << 6);
			rx_cal = set_rx_cal(index, rx_cal, cal_result);
			phy_write_reg(mcu_id, addr + offset, rx_cal);
			DSB_SY_CALL;
			ISB;
		} else {
			ddr_verbose("MCU-PHY[%d]: Slice: %d Addr: 0x%X \
				     RX Cal Up: cal_up: 0x%X cal_down: 0x%X\n",
				    mcu_id, slice, addr, cal_up, cal_down);
			return rx_cal;
		}
	}
	ddr_err("MCU-PHY[%d]: Couldn't find RX Up Calibration Code Slice:\
		%d Addr: 0x%X cal_up: 0x%X cal_down: 0x%X\n",
		mcu_id, slice, addr, cal_up, cal_down);
	return -1;
}


int phy_sw_rx_cal_down_code(unsigned int mcu_id, unsigned int addr,
			    unsigned int index, unsigned int slice)
{
	unsigned int rx_obs, rx_cal, rx_cal_result;
	unsigned int cal_up, cal_down, cal_value, offset, cal_result;

	offset = slice * PHY_SLICE_OFFSET;
	/*
	 * Decrement RX_CAL_CODE_UP, Increment RX_CAL_CODE_DOWN,
	 * Set new calibration code PHY_RX_CAL_DQx/DQS/DM/FDBK_X[11:0]
	 * Examine result PHY_RX_CAL_OBS_X[11:0] for appropriate DQx/DQS/DM/FDBK pad
	 * Result = 1’b0, go back to sw_rx_cal_down_code, else done
	 */
	rx_cal = phy_read_reg(mcu_id, addr + offset);
	cal_value = get_rx_cal(index, rx_cal);

	cal_down = cal_value & 0x3F;
	cal_up = (cal_value >> 6) & 0x3F;

	for (int i = PHY_RX_CAL_MIN_VALUE; i < PHY_RX_CAL_MAX_VALUE; i++) {
		/* Index for appropriate bit in OBS Reg */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << index);

		rx_cal = phy_read_reg(mcu_id, addr + offset);
		cal_value = get_rx_cal(index, rx_cal);

		cal_down = cal_value & 0x3F;
		cal_up = (cal_value >> 6) & 0x3F;

		if ((cal_up == 0x0) && (cal_down == 0x3E)) {
			ddr_verbose("MCU-PHY[%d]: Slice: %d Addr: 0x%X \
				     RX Cal Down Saturate: cal_up: 0x%X cal_down: 0x%X\n",
				    mcu_id, slice, addr, cal_up, cal_down);
			return rx_cal;
		} else if (rx_cal_result == 0) {
			cal_up = (cal_up - 1) & 0x3F;
			cal_down = (cal_down + 1) & 0x3F;

			cal_result = cal_down | (cal_up << 6);
			rx_cal = set_rx_cal(index, rx_cal, cal_result);
			phy_write_reg(mcu_id, addr + offset, rx_cal);
			DSB_SY_CALL;
			ISB;
		} else {
			ddr_verbose("MCU-PHY[%d]: Slice: %d Addr: 0x%X \
				    RX Cal Down: cal_up: 0x%X cal_down: 0x%X\n",
				    mcu_id, slice, addr, cal_up, cal_down);
			return rx_cal;
		}
	}
	ddr_err("MCU-PHY[%d]: Couldn't find RX Down Calibration Code Slice:\
		%d Addr: 0x%X cal_up: 0x%X cal_down: 0x%X\n",
		mcu_id, slice, addr, cal_up, cal_down);
	return -1;
}

void phy_sw_rx_cal_stop(unsigned int mcu_id, unsigned int offset)
{
	unsigned int regd;

	/* Clear calibration override PHY_RX_CAL_OVERRIDE_X = 1’b0 */
	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OVERRIDE_0_ADDR + offset);
	regd = FIELD_MCU_PHY_RX_CAL_OVERRIDE_0_SET(regd, 0);
	phy_write_reg(mcu_id, MCU_PHY_RX_CAL_OVERRIDE_0_ADDR + offset, regd);

	/* Clear I/O Pad input enable PHY_IE_MODE_X = 2’b00 */
	regd = phy_read_reg(mcu_id, MCU_PHY_IE_MODE_0_ADDR + offset);
	regd = FIELD_MCU_PHY_IE_MODE_0_SET(regd, 0);
	phy_write_reg(mcu_id, MCU_PHY_IE_MODE_0_ADDR + offset, regd);

	/* Set DQS manual clear SC_PHY_MANUAL_CLEAR_X = 5’b00011 Bug 59047 */
	regd = phy_read_reg(mcu_id, MCU_SC_PHY_MANUAL_CLEAR_0_ADDR + offset);
	regd = FIELD_MCU_SC_PHY_MANUAL_CLEAR_0_SET(regd, 3);
	phy_write_reg(mcu_id, MCU_SC_PHY_MANUAL_CLEAR_0_ADDR + offset, regd);

	/* Clear DQS manual clear SC_PHY_MANUAL_CLEAR_X = 5’b00000 */
	regd = phy_read_reg(mcu_id, MCU_SC_PHY_MANUAL_CLEAR_0_ADDR + offset);
	regd = FIELD_MCU_SC_PHY_MANUAL_CLEAR_0_SET(regd, 0);
	phy_write_reg(mcu_id, MCU_SC_PHY_MANUAL_CLEAR_0_ADDR + offset, regd);

	DSB_SY_CALL;
	ISB;
}

/* RX Calibration Software Bug 58664 */
int phy_sw_rx_calibration(unsigned int mcu_id, unsigned int device_type, int cal_en)
{
	unsigned int rx_cal_result, rx_obs, result;
        unsigned int offset;
	unsigned int regd, rx_code[CDNPHY_NUM_SLICES][12];

	/*
	 * Run RX Calibration software routine
	 * Examine OBS Registers if result = 1, run phy_sw_rx_cal_up_code
	 * else, run phy_sw_rx_cal_down_code
	 */
	for (int slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {

		offset = slice * PHY_SLICE_OFFSET;
		/* Setup Software Calibration */
		phy_sw_rx_cal_setup(mcu_id, offset);

		/* Check if SW Calibration Enabled */
		if (!cal_en) {
			/* Slice programmed with hardcoded values */
			phy_sw_rx_cal_stop(mcu_id, offset);
			continue;
		}

		/* DQ0 */
                rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ0_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR,
						       PHY_RX_CAL_OBS_DQ0_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR,
							 PHY_RX_CAL_OBS_DQ0_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ0_BIT] = result;
		else
			return result;

		/* DQ1 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ1_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR,
						       PHY_RX_CAL_OBS_DQ1_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR,
							 PHY_RX_CAL_OBS_DQ1_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ1_BIT] = result;
		else
			return result;

		/* DQ2 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ2_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR,
						       PHY_RX_CAL_OBS_DQ2_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR,
							 PHY_RX_CAL_OBS_DQ2_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ2_BIT] = result;
		else
			return result;

		/* DQ3 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ3_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR,
						       PHY_RX_CAL_OBS_DQ3_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR,
							 PHY_RX_CAL_OBS_DQ3_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ3_BIT] = result;
		else
			return result;

		/* DQ4 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ4_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR,
						       PHY_RX_CAL_OBS_DQ4_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR,
							 PHY_RX_CAL_OBS_DQ4_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ4_BIT] = result;
		else
			return result;

		/* DQ5 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ5_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR,
						       PHY_RX_CAL_OBS_DQ5_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR,
							 PHY_RX_CAL_OBS_DQ5_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ5_BIT] = result;
		else
			return result;

		/* DQ6 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ6_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR,
						       PHY_RX_CAL_OBS_DQ6_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR,
							 PHY_RX_CAL_OBS_DQ6_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ6_BIT] = result;
		else
			return result;

		/* DQ7 */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQ7_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR,
						       PHY_RX_CAL_OBS_DQ7_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR,
							 PHY_RX_CAL_OBS_DQ7_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQ7_BIT] = result;
		else
			return result;

		/* DM */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DM_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR,
                                                       PHY_RX_CAL_OBS_DM_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR,
							 PHY_RX_CAL_OBS_DM_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DM_BIT] = result;
		else
			return result;

		/* DQS */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_DQS_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR,
						       PHY_RX_CAL_OBS_DQS_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR,
							 PHY_RX_CAL_OBS_DQS_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_DQS_BIT] = result;
		else
			return result;

		/* FDBK */
		rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
		rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
		rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_FDBK_BIT);

		if (rx_cal_result)
			result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR,
						       PHY_RX_CAL_OBS_FDBK_BIT, slice);
		else
			result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR,
							 PHY_RX_CAL_OBS_FDBK_BIT, slice);

		if (result != -1)
			rx_code[slice][PHY_RX_CAL_OBS_FDBK_BIT] = result;
		else
			return result;

		/* X4_FDBK */
		if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4 && !is_skylark_A0()) {
			rx_obs = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_OBS_0_ADDR + offset);
			rx_obs = FIELD_MCU_PHY_RX_CAL_OBS_0_RD(rx_obs);
			rx_cal_result = rx_obs & (1 << PHY_RX_CAL_OBS_X4_FDBK_BIT);

			if (rx_cal_result)
				result = phy_sw_rx_cal_up_code(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR,
								PHY_RX_CAL_OBS_X4_FDBK_BIT, slice);
			else
				result = phy_sw_rx_cal_down_code(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR,
								PHY_RX_CAL_OBS_X4_FDBK_BIT, slice);

			if (result != -1)
				rx_code[slice][PHY_RX_CAL_OBS_X4_FDBK_BIT] = result;
			else
				return result;
		}
		/* slice done, Stop */
		phy_sw_rx_cal_stop(mcu_id, offset);
	}

	/*
	 * As per Cadence, SW needs to insure that values found in step 12 are kept in
	 * the appropriate registers, parallel calibration for multiple I/O pads may have
	 * left wrong values in pads that finished calibration earlier (reached step 12)
	 * before others in the parallel execution
	 */
	for (int slice = CDNPHY_NUM_SLICES; slice < CDNPHY_NUM_SLICES; slice++) {
		offset = slice * PHY_SLICE_OFFSET;

		/* Update DQ0[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ0_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ0_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ0_0_ADDR + offset, regd);

		/* Update DQ1[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR + offset);
                regd =  FIELD_MCU_PHY_RX_CAL_DQ1_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ1_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ1_0_ADDR + offset, regd);

		/* Update DQ2[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ2_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ2_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ2_0_ADDR + offset, regd);

		/* Update DQ3[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ3_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ3_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ3_0_ADDR + offset, regd);

		/* Update DQ4[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ4_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ4_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ4_0_ADDR + offset, regd);

		/* Update DQ5[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ5_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ5_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ5_0_ADDR + offset, regd);

		/* Update DQ6[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ6_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ6_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ6_0_ADDR + offset, regd);

		/* Update DQ7[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQ7_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQ7_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQ7_0_ADDR + offset, regd);

		/* Update DM[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DM_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DM_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DM_0_ADDR + offset, regd);

		/* Update DQS[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_DQS_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_DQS_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_DQS_0_ADDR + offset, regd);

		/* Update FDBK[11:0] */
		regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR + offset);
		regd =  FIELD_MCU_PHY_RX_CAL_FDBK_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_FDBK_BIT]);
		phy_write_reg(mcu_id, MCU_PHY_RX_CAL_FDBK_0_ADDR + offset, regd);

		/* Update X4_FDBK[11:0] */
		if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4 && !is_skylark_A0()) {
			regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR + offset);
			regd =  FIELD_MCU_PHY_RX_CAL_X4_FDBK_0_SET(regd, rx_code[slice][PHY_RX_CAL_OBS_X4_FDBK_BIT]);
			phy_write_reg(mcu_id, MCU_PHY_RX_CAL_X4_FDBK_0_ADDR + offset, regd);
		}
	}
	return 0;
}

/******************************************************************************
 *     Write DSKW Register Offset Function
 *****************************************************************************/

unsigned int wrdskw_get_reg_offset(unsigned int bit)
{
	unsigned int reg_offset = 0;
	switch(bit) {
	case 0:
		reg_offset = MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_ADDR;
		break;
	case 1:
		reg_offset = MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_ADDR;
		break;
	case 2:
		reg_offset = MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_ADDR;
		break;
	case 3:
		reg_offset = MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_ADDR;
		break;
	case 4:
		reg_offset = MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_ADDR;
		break;
	case 5:
		reg_offset = MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_ADDR;
		break;
	case 6:
		reg_offset = MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_ADDR;
		break;
	case 7:
		reg_offset = MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_ADDR;
		break;
	}
	return reg_offset;
}

unsigned int wrdskw_set_delay_value(unsigned int bit, unsigned int regd,
                                    unsigned int delay)
{
	switch(bit) {
	case 0:
		regd = FIELD_MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 1:
		regd = FIELD_MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 2:
		regd = FIELD_MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 3:
		regd = FIELD_MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 4:
		regd = FIELD_MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 5:
		regd = FIELD_MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 6:
		regd = FIELD_MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 7:
		regd = FIELD_MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_SET(regd, delay);
		break;
	}
	return regd;
}

/******************************************************************************
 *     Read Rise DSKW Register Offset Function
 *****************************************************************************/

unsigned int rddskw_rise_get_reg_offset(unsigned int bit)
{
	unsigned int reg_offset = 0;
	switch(bit) {
	case 0:
		reg_offset = MCU_PHY_RDDQS_DQ0_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 1:
		reg_offset = MCU_PHY_RDDQS_DQ1_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 2:
		reg_offset = MCU_PHY_RDDQS_DQ2_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 3:
		reg_offset = MCU_PHY_RDDQS_DQ3_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 4:
		reg_offset = MCU_PHY_RDDQS_DQ4_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 5:
		reg_offset = MCU_PHY_RDDQS_DQ5_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 6:
		reg_offset = MCU_PHY_RDDQS_DQ6_RISE_SLAVE_DELAY_0_ADDR;
		break;
	case 7:
		reg_offset = MCU_PHY_RDDQS_DQ7_RISE_SLAVE_DELAY_0_ADDR;
		break;
	}
	return reg_offset;
}

unsigned int rddskw_rise_set_delay_value(unsigned int bit, unsigned int regd,
                                         unsigned int delay)
{
	switch(bit) {
	case 0:
		regd = FIELD_MCU_PHY_RDDQS_DQ0_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 1:
		regd = FIELD_MCU_PHY_RDDQS_DQ1_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 2:
		regd = FIELD_MCU_PHY_RDDQS_DQ2_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 3:
		regd = FIELD_MCU_PHY_RDDQS_DQ3_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 4:
		regd = FIELD_MCU_PHY_RDDQS_DQ4_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 5:
		regd = FIELD_MCU_PHY_RDDQS_DQ5_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 6:
		regd = FIELD_MCU_PHY_RDDQS_DQ6_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 7:
		regd = FIELD_MCU_PHY_RDDQS_DQ7_RISE_SLAVE_DELAY_0_SET(regd, delay);
		break;
	}
	return regd;
}

/******************************************************************************
 *     Read Fall DSKW Register Offset Function
 *****************************************************************************/

unsigned int rddskw_fall_get_reg_offset(unsigned int bit)
{
	unsigned int reg_offset = 0;
	switch(bit) {
	case 0:
		reg_offset = MCU_PHY_RDDQS_DQ0_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 1:
		reg_offset = MCU_PHY_RDDQS_DQ1_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 2:
		reg_offset = MCU_PHY_RDDQS_DQ2_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 3:
		reg_offset = MCU_PHY_RDDQS_DQ3_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 4:
		reg_offset = MCU_PHY_RDDQS_DQ4_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 5:
		reg_offset = MCU_PHY_RDDQS_DQ5_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 6:
		reg_offset = MCU_PHY_RDDQS_DQ6_FALL_SLAVE_DELAY_0_ADDR;
		break;
	case 7:
		reg_offset = MCU_PHY_RDDQS_DQ7_FALL_SLAVE_DELAY_0_ADDR;
		break;
	}
	return reg_offset;
}

unsigned int rddskw_fall_set_delay_value(unsigned int bit, unsigned int regd,
                                         unsigned int delay)
{
	switch(bit) {
	case 0:
		regd = FIELD_MCU_PHY_RDDQS_DQ0_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 1:
		regd = FIELD_MCU_PHY_RDDQS_DQ1_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 2:
		regd = FIELD_MCU_PHY_RDDQS_DQ2_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 3:
		regd = FIELD_MCU_PHY_RDDQS_DQ3_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 4:
		regd = FIELD_MCU_PHY_RDDQS_DQ4_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 5:
		regd = FIELD_MCU_PHY_RDDQS_DQ5_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 6:
		regd = FIELD_MCU_PHY_RDDQS_DQ6_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	case 7:
		regd = FIELD_MCU_PHY_RDDQS_DQ7_FALL_SLAVE_DELAY_0_SET(regd, delay);
		break;
	}
	return regd;
}


/* Update X4_FDBK_TERM based of FDBK_TERM: Bug 59696 */
void update_x4_fdbk_term_pvt(struct apm_mcu * mcu)
{
	unsigned int regd;
	unsigned int mcu_id = mcu->id;

	/* PHY_PAD_FDBK_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_FDBK_TERM_ADDR);
	/* PHY_PAD_X4_FDBK_TERM */
	phy_write_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_TERM_ADDR, regd);
}
