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

int dmc_wrlvl_routine(struct apm_mcu *mcu, unsigned int rank)
{
	/* DMC driven Write leveling routine*/
	int err = 0;
	unsigned int mcu_id = mcu->id;
	unsigned int mrs_mr2, mrs_mr1;
	unsigned int non_active_ranks;

	if (!mcu->enabled)
		return 0;

	non_active_ranks = mcu->ddr_info.physical_ranks & (~(1 << rank));

	mrs_mr2 = config_mr2_value(mcu->ddr_info.cw_latency, mcu->mcu_ud.ud_rtt_wr, crc_enable(mcu->id));
	mrs_mr1 = config_mr1_value(mcu->mcu_ud.ud_rtt_nom_s0, mcu->mcu_ud.ud_mr1_dic, 0, 1);

	ddr_verbose("MCU[%d]: start DMC Write leveling routine for rank=%01d\n",
		   mcu_id, rank);

	/* Disable Output Buffer for non active ranks MR1*/
	if (non_active_ranks != 0) {
		dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_mr1 | 0x1000);
		dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, non_active_ranks, 1, 0, 0, 0);
	}

	/*
	 * Dynamic ODT not allowed(RTT_WR==0) if write leveling is enabled
	 * Only allowed settings are RTT_NOM and RTT_PARK
	 * 4.7.1 JESD79-4A_r2
	 */
	if (mcu->mcu_ud.ud_rtt_wr != 0) {
		dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_mr2 & 0xfffff1ff);
		dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_MRS, rank, 2, 0, 0, 0);
	}

	/* Initiate the write leveling using DCI command */
	/* Issue NOP */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_NOP, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Issue Write Leveling command */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 3);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_TRAIN, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Poll for Write leveling completion*/
	err = dmc_poll_reg(mcu->id, MEMC_STATUS_ADDR, REGV_MGR_ACTIVE_IDLE << FIELD_DMC520_MGR_ACTIVE_SHIFT_MASK,
                           FIELD_DMC520_MGR_ACTIVE_MASK);
	if (err) {
		dmc_config_state(mcu_id);
		ddr_err("MCU[%d]: *** Write Leveling DMC status doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	err = dmc_poll_reg(mcu->id, CHANNEL_STATUS_ADDR, REGV_M0_IDLE_HIGH << FIELD_DMC520_M0_IDLE_SHIFT_MASK,
                           FIELD_DMC520_M0_IDLE_MASK);
	if (err) {
		dmc_config_state(mcu_id);
		ddr_err("MCU[%d]: *** Write Leveling DMC channel doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	/* Look for training errors & copy delay values */
	err = mcu_check_wrlvl_obs(mcu_id, rank);
	if (err) {
		ddr_err("MCU[%d]: *** Write Leveling ERROR: %d ***\n",
			mcu_id, err);
		return err;
	}

	/* Restore Rtt_nom */
	if (mcu->mcu_ud.ud_rtt_wr != 0) {
		dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_mr2);
       		dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_MRS, rank, 2, 0, 0, 0);
	}

	/* Restore MR1 which should enable Output Buffer for non active ranks*/
	if (non_active_ranks != 0) {
		dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_mr1);
		dmc_cmd_multiple(mcu->id, REGV_DIRECT_CMD_MRS, non_active_ranks, 1, 0, 0, 0);
        }


	return err;
}

int dmc_rdgate_routine(struct apm_mcu *mcu, unsigned int rank)
{
	/* DMC driven Read Gate training for all lanes and ranks */

	int err = 0;
	unsigned int mcu_id = mcu->id;

	if (!mcu->enabled) {
		return 0;
	}

	/* Initiate the Read Gate Training using DCI command */
	/* Issue NOP */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_NOP, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Issue Read Gate command */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 2);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_TRAIN, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Poll for Read Gate completion*/
	err = dmc_poll_reg(mcu->id, MEMC_STATUS_ADDR, REGV_MGR_ACTIVE_IDLE << FIELD_DMC520_MGR_ACTIVE_SHIFT_MASK,
                           FIELD_DMC520_MGR_ACTIVE_MASK);
	if (err) {
		ddr_err("MCU[%d]: *** Read Gate Training DMC status doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	err = dmc_poll_reg(mcu->id, CHANNEL_STATUS_ADDR, REGV_M0_IDLE_HIGH << FIELD_DMC520_M0_IDLE_SHIFT_MASK,
                           FIELD_DMC520_M0_IDLE_MASK);
	if (err) {
		ddr_err("MCU[%d]: *** Read Gate Training DMC status doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	/* Look for training errors */
	err = mcu_check_rdgate_obs(mcu_id, rank);

	/* FIFO Reset */
	mcu_phy_rdfifo_reset(mcu_id);

	if (err) {
		ddr_err("MCU[%d]: *** Read Gate Training ERROR: %d ***\n",
			mcu_id, err);
		return err;
	}

	return err;
}

int dmc_rdlvl_routine(struct apm_mcu *mcu, unsigned int rank)
{
	/* DMC driven Read Level training for all lanes and ranks */

	int err = 0;
	unsigned int mcu_id = mcu->id;

	if (!mcu->enabled)
		return 0;

	/* Initiate the Read Eye Leveling using DCI command */
	/* Issue NOP */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 0);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_NOP, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Issue Read Leveling command */
	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, 1);
	dmc_cmd_single(mcu->id, REGV_DIRECT_CMD_TRAIN, rank, 0, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Poll for Read leveling completion*/
	err = dmc_poll_reg(mcu->id, MEMC_STATUS_ADDR, REGV_MGR_ACTIVE_IDLE << FIELD_DMC520_MGR_ACTIVE_SHIFT_MASK,
                           FIELD_DMC520_MGR_ACTIVE_MASK);
	if (err) {
		ddr_err("MCU[%d]: *** Read Eye Leveling DMC status doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	err = dmc_poll_reg(mcu->id, CHANNEL_STATUS_ADDR, REGV_M0_IDLE_HIGH << FIELD_DMC520_M0_IDLE_SHIFT_MASK,
                           FIELD_DMC520_M0_IDLE_MASK);
	if (err) {
		ddr_err("MCU[%d]: *** Read Eye Leveling DMC status doesn't go idle: %d ***\n",
			mcu_id, err);
		return err;
	}

	/* Look for training errors & possibly copy delay values before WrCal & deskew */
	err = mcu_check_rdlvl_obs(mcu_id, rank);

	if (err) {
		ddr_err("MCU[%d]: *** Read Eye leveling ERROR: %d ***\n",
			mcu_id, err);
		return err;
	}

	return err;
}

void dmc_rdlvl_pattern(struct apm_mcu *mcu, unsigned int rank)
{
	/* DMC Read Level Pattern modification */
	unsigned int lvl_pattern, mcu_id = mcu->id;
	unsigned int mrs_mr3, mrs_mr3_inverted;
	unsigned int bank, muxed_rank;

	if (!mcu->enabled)
		return;

	/* Non-UDIMMs have A-side and B-side */
	if (mcu->ddr_info.package_type == UDIMM)
		return;

	/* Issue DCI MRS command to enable MPR on DRAMs */
	mrs_mr3 = config_mr3_value(mcu->ddr_info.t_ck_ps,
		  mcu->mcu_ud.ud_refresh_granularity, 0, 0, 0);

	/* Inverted MR3 */
	mrs_mr3_inverted = mrs_inversion(mrs_mr3);

	/* Addressing mirroring enabled */
	if ((rank % 2 == 1) && (mcu->ddr_info.addr_mirror)) {
		mrs_mr3 = address_mirroring(mrs_mr3);
		mrs_mr3_inverted = address_mirroring(mrs_mr3_inverted);
	}

	muxed_rank = get_rank_mux(mcu_id, rank, mcu->ddr_info.two_dpc_enable);
	/*
	 * Here we need to specify the DIMM rank, work for general scenario,
	 * but for 3DS we will have to remap the ranks. Issue DCI Control Word
	 * command to enable MPR Page 2 on side A (this is a READ_ONLY page)
	 */
	mrs_mr3 = (mrs_mr3 & ~0x7) | 0x6;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, mrs_mr3);
	dmc_cmd_single(mcu_id, REGV_DIRECT_CMD_CONTROL_WORD, muxed_rank,
		       0x3, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/*
	 * Issue DCI Control Word  command to enable MPR Page 0 on  Side B
	 * (this would be the MPR #  you would like to use for RD leveling sequence)
	 */
	mrs_mr3_inverted = (mrs_mr3_inverted & ~0x7) | 0x4;
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, mrs_mr3_inverted);
	/* 0xC is for MRS inversion being enabled */
	dmc_cmd_single(mcu_id, REGV_DIRECT_CMD_CONTROL_WORD, muxed_rank,
		       0xC, 0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Read level bank */
	bank = dmc_read_reg(mcu_id, DMC520_RDLVL_COMMAND_BA1_0_NOW_ADDR);
	bank = FIELD_DMC520_RDLVL_COMMAND_BA1_0_NOW_RD(bank);

	/*
	 * Issue DCI Write command to overwrite to the MPR # you have chosen on Side B,
	 * Page 0 and make sure its data matches  the opposite MPR# on Side A
	 * This will issue an MPR Write to Side A Page 2 (READ_ONLY),
	 * however DDR4 JEDEC spec ,under section 4.10.5 MPR Read Data Format , it states:
	 * When MPR Write command is issued to any read-only pages (page 1,2, or 3),
	 * the command is ignored by DRAM. Overwrite the pattern
	 */
	switch (bank) {
	case 1:
		lvl_pattern = 0xCB;
		break;
	case 2:
		lvl_pattern = 0xF7;
		break;
	case 3:
		lvl_pattern = 0xF8;
		break;
	default:
		/* bank = 0 (max 4 banks) */
		lvl_pattern = 0xAD;
		break;
	}

	/*
	 * Bank has to be mirrored if needed,
	 * currently we don't need it as only bank 0 is accessed
	 */
	dmc_write_reg(mcu_id, DIRECT_ADDR_ADDR, lvl_pattern);
	dmc_cmd_single(mcu_id, REGV_DIRECT_CMD_WRITE, rank, (bank & 0x3) << 2,
		       0, 0, REGV_REPLAY_OVERRIDE_REPLAY_TYPE);

	/* Issue DCI MRS command to disable MPR on DRAMs */
	mrs_mr3 = config_mr3_value(mcu->ddr_info.t_ck_ps,
				   mcu->mcu_ud.ud_refresh_granularity, 0, 0, 0);

	dmc_write_reg(mcu->id, DIRECT_ADDR_ADDR, mrs_mr3);
	dmc_cmd_single(mcu_id, REGV_DIRECT_CMD_MRS, rank, 3, 0, 0,
		       REGV_REPLAY_OVERRIDE_REPLAY_TYPE);
}
