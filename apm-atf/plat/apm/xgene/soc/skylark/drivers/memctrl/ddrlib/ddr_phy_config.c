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

/******************************************************************************
 *     PHY CSR config thru DFI init complete
 *****************************************************************************/
int phy_csr_config(struct apm_mcu *mcu)
{
	unsigned int regd = 0, jj, offset;
	unsigned int mcu_id = mcu->id;

	ddr_info("MCU-PHY[%d]: PHY Registers setup\n", mcu_id);

	config_per_cs_training_index(mcu_id, 0, !mcu->mcu_ud.ud_singlerank_train_mode);

	/* Enable Multicast */
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		offset = jj * PHY_SLICE_OFFSET;
		regd = mcu->phy_rd(mcu_id, (MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset));
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_SET(regd, 1);
		mcu->phy_wr(mcu_id, (MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset), regd);
	}
	set_phy_deskew_pll(mcu_id);
	set_phy_config(mcu);
	set_master_dly(mcu_id);
	set_grp_slave_dly(mcu);
	set_tsel_timing(mcu);
	set_drv_term(mcu);
	set_wrlvl_config(mcu_id);
	set_gtlvl_config(mcu_id);
	set_rdlvl_config(mcu_id);

	/* Disable Multicast if Multi Rank Training Mode Enabled */
	for (jj = 0; jj < CDNPHY_NUM_SLICES; jj++) {
		offset = jj * PHY_SLICE_OFFSET;
		regd = mcu->phy_rd(mcu_id, (MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset));
		regd = FIELD_MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_SET(regd, mcu->mcu_ud.ud_singlerank_train_mode);
		mcu->phy_wr(mcu_id, (MCU_PHY_PER_CS_TRAINING_MULTICAST_EN_0_ADDR + offset), regd);
	}

	/*
	 * De-assert Phy DLL Reset (Mcb Csr)
	 */
	return change_mcu_phy_dll_reset(mcu);
}

/******************************************************************************
 *     PHY after init/train runtime settings
 *****************************************************************************/
void phy_post_train_setup(struct apm_mcu *mcu)
{
	unsigned int regd;

	if (!mcu->enabled)
		return;

	/* PHY_CAL_SAMPLE_WAIT*/
	regd = phy_read_reg(mcu->id, MCU_PHY_CAL_SAMPLE_WAIT_0_ADDR);
	regd = FIELD_MCU_PHY_CAL_SAMPLE_WAIT_0_SET(regd, PHY_CAL_SAMPLE_WAIT);
	phy_write_reg(mcu->id, MCU_PHY_CAL_SAMPLE_WAIT_0_ADDR, regd);

	/* PHY_CAL_CLK_SELECT */
	regd = phy_read_reg(mcu->id, MCU_PHY_CAL_CLK_SELECT_0_ADDR);
	regd = FIELD_MCU_PHY_CAL_CLK_SELECT_0_SET(regd, PHY_CAL_CLK_SELECT);
	phy_write_reg(mcu->id, MCU_PHY_CAL_CLK_SELECT_0_ADDR, regd);

	if (mcu->mcu_ud.ud_phy_cal_mode_on) {
		regd = phy_read_reg(mcu->id, MCU_PHY_CAL_MODE_0_ADDR);
		regd = (regd & 0xF1FFFFFF) | (0x1 << 25) |
			((mcu->mcu_ud.ud_phy_cal_base_interval << 26) & 0xF3FFFFFF);
		ddr_verbose("MCU-PHY[%d]: phy_cal_mode_auto_en=1  phy_cal_mode_base_intvl=%d \n",
			    mcu->id, mcu->mcu_ud.ud_phy_cal_base_interval);
		phy_write_reg(mcu->id, MCU_PHY_CAL_MODE_0_ADDR, regd);

		regd = phy_read_reg(mcu->id, MCU_PHY_CAL_INTERVAL_COUNT_0_ADDR);
		regd = FIELD_MCU_PHY_CAL_INTERVAL_COUNT_0_SET(regd, mcu->mcu_ud.ud_phy_cal_interval_count_0);
		phy_write_reg(mcu->id, MCU_PHY_CAL_INTERVAL_COUNT_0_ADDR, regd);
		ddr_verbose("MCU-PHY[%d]: set phy_cal_interval_count_0 = 32'd%d \n",
			    mcu->id, mcu->mcu_ud.ud_phy_cal_interval_count_0);
	}
}

void set_phy_deskew_pll(unsigned int mcu_id)
{
	unsigned int regd;

	/* PHY_PLL_WAIT */
	regd = phy_read_reg(mcu_id, MCU_PHY_PLL_WAIT_ADDR);
	regd = FIELD_MCU_PHY_PLL_WAIT_SET(regd, PHY_PLL_WAIT);
	phy_write_reg(mcu_id, MCU_PHY_PLL_WAIT_ADDR, regd);

	/* PHY_PLL_CTRL */
	regd = phy_read_reg(mcu_id, MCU_PHY_PLL_CTRL_ADDR);
	regd = FIELD_MCU_PHY_PLL_CTRL_SET(regd, PHY_PLL_CTRL);
	phy_write_reg(mcu_id, MCU_PHY_PLL_CTRL_ADDR, regd);
}

void set_phy_config(struct apm_mcu * mcu)
{
	unsigned int regd, data;
	unsigned int mcu_id = mcu->id;
	unsigned int t_ck_ps = mcu->ddr_info.t_ck_ps;
	unsigned int rddata_en_dly;

	/* PHY_DQS_RATIO_X8 */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_RATIO_X8_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_RATIO_X8_0_SET(regd, mcu->ddr_info.device_type);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_RATIO_X8_0_ADDR, regd);

	/* PHY_DDR4 */
	regd = phy_read_reg(mcu_id, MCU_PHY_DDR4_0_ADDR);
	regd = FIELD_MCU_PHY_DDR4_0_SET(regd, 1);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DDR4_0_ADDR, regd);

	/* PHY_TWO_CYC_PREAMBLE */
	regd = phy_read_reg(mcu_id, MCU_PHY_TWO_CYC_PREAMBLE_0_ADDR);
	data = (mcu->ddr_info.write_preamble - 1) << 1;
	regd = FIELD_MCU_PHY_TWO_CYC_PREAMBLE_0_SET(regd, data | PHY_TWO_CYC_PREAMBLE);
	phy_write_reg(mcu_id, MCU_PHY_TWO_CYC_PREAMBLE_0_ADDR, regd);

	/* PHY_CAL_SAMPLE_WAIT*/
	regd = phy_read_reg(mcu_id, MCU_PHY_CAL_SAMPLE_WAIT_0_ADDR);
	regd = FIELD_MCU_PHY_CAL_SAMPLE_WAIT_0_SET(regd, PHY_CAL_SAMPLE_WAIT);
	phy_write_reg(mcu_id, MCU_PHY_CAL_SAMPLE_WAIT_0_ADDR, regd);

	/* PHY_CAL_CLK_SELECT */
	regd = phy_read_reg(mcu_id, MCU_PHY_CAL_CLK_SELECT_0_ADDR);
	regd = FIELD_MCU_PHY_CAL_CLK_SELECT_0_SET(regd, PHY_CAL_CLK_SELECT);
	phy_write_reg(mcu_id, MCU_PHY_CAL_CLK_SELECT_0_ADDR, regd);

	/* PHY_RX_CAL_START */
	regd = phy_read_reg(mcu_id, MCU_SC_PHY_RX_CAL_START_0_ADDR);
	regd = FIELD_MCU_SC_PHY_RX_CAL_START_0_SET(regd, PHY_RX_CAL_START);
	phy_wr_all_data_slices(mcu_id, MCU_SC_PHY_RX_CAL_START_0_ADDR, regd);

	/* PHY_RX_CAL_SAMPLE_WAIT_0 */
	regd = phy_read_reg(mcu_id, MCU_PHY_RX_CAL_SAMPLE_WAIT_0_ADDR);
	regd = FIELD_MCU_PHY_RX_CAL_SAMPLE_WAIT_0_SET(regd, PHY_RX_CAL_SAMPLE_WAIT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RX_CAL_SAMPLE_WAIT_0_ADDR, regd);

	/* PHY_IE_MODE */
	regd = phy_read_reg(mcu_id, MCU_PHY_IE_MODE_0_ADDR);
	regd = FIELD_MCU_PHY_IE_MODE_0_SET(regd, PHY_IE_MODE);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_IE_MODE_0_ADDR, regd);

	/* PHY_PER_RANK_CS_MAP */
	regd = phy_read_reg(mcu_id, MCU_PHY_PER_RANK_CS_MAP_0_ADDR);
	regd = FIELD_MCU_PHY_PER_RANK_CS_MAP_0_SET(regd, PHY_PER_RANK_CS_MAP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_PER_RANK_CS_MAP_0_ADDR, regd);

	/* PHY_RDDATA_EN_DLY */
	rddata_en_dly = cdiv(IO_INPUT_ENABLE_DELAY * 10, t_ck_ps);
	regd = phy_read_reg(mcu_id, MCU_PHY_RDDATA_EN_DLY_0_ADDR);
	regd = FIELD_MCU_PHY_RDDATA_EN_DLY_0_SET(regd, rddata_en_dly);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDDATA_EN_DLY_0_ADDR, regd);

	/* PHY_RDDATA_EN_IE_DLY */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDDATA_EN_IE_DLY_0_ADDR);
	regd = FIELD_MCU_PHY_RDDATA_EN_IE_DLY_0_SET(regd, PHY_RDDATA_EN_IE_DLY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDDATA_EN_IE_DLY_0_ADDR, regd);

	/* PHY_RDDATA_EN_TSEL_DLY_0 */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDDATA_EN_TSEL_DLY_0_ADDR);
	regd = FIELD_MCU_PHY_RDDATA_EN_TSEL_DLY_0_SET(regd, rddata_en_dly - 1);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDDATA_EN_TSEL_DLY_0_ADDR, regd);

	/* PHY_RPTR_UPDATE */
	regd = phy_read_reg(mcu_id, MCU_PHY_RPTR_UPDATE_0_ADDR);
	regd = FIELD_MCU_PHY_RPTR_UPDATE_0_SET(regd, PHY_RPTR_UPDATE);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RPTR_UPDATE_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_RPTR_UPDATE_0_ADDR);
	regd = FIELD_MCU_PHY_X4_RPTR_UPDATE_0_SET(regd, PHY_RPTR_UPDATE);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_RPTR_UPDATE_0_ADDR, regd);

	/* PHY_PAD_CAL_MODE */
        /* PAD CONTROL DDR3 vs DDR4, control VREF */
	regd = phy_read_reg(mcu_id, MCU_PHY_CAL_MODE_0_ADDR);
        /*
         * Bit[24]: Init Disable, Bit[25]: Auto Enable
         * Bit[27:26]: Base Mode Bit[31:28]: pad_cal_mode
         */
	regd = (regd & 0x0FFFFFFF) | ((PHY_PAD_CAL_MODE << 28) & 0xF0000000);
	phy_write_reg(mcu_id, MCU_PHY_CAL_MODE_0_ADDR, regd);

	/* PHY_CLK_WRDQ0_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ0_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ1_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ1_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ2_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ2_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ3_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ3_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ4_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ4_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ5_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ5_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ6_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ6_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQ7_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDQx_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDQ7_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_CLK_WRDQM_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_CLK_WRDM_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_CLK_WRDM_SLAVE_DELAY_0_SET(regd, PHY_CLK_WRDM_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_CLK_WRDM_SLAVE_DELAY_0_ADDR, regd);
}

void set_grp_slave_dly(struct apm_mcu * mcu)
{
	unsigned int regd;
	unsigned int mcu_id = mcu->id;

	/* MCU_PHY_GRP_SLAVE_DELAY_0 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_0_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_0_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_1 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_1_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_1_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_1_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_2 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_2_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_2_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_2_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_3 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_3_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_3_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_3_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_4 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_4_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_4_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_4_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_5 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_5_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_5_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_5_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_6 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_6_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_6_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_6_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_7 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_7_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_7_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_7_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_8 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_8_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_8_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_8_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_9 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_9_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_9_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_9_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_10 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_10_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_10_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_10_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_11 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_11_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_11_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_11_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_12 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_12_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_12_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_12_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_13 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_13_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_13_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_13_ADDR, regd);

	/* MCU_PHY_GRP_SLAVE_DELAY_14 */
	regd = phy_read_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_14_ADDR);
	regd = FIELD_MCU_PHY_GRP_SLAVE_DELAY_14_SET(regd, mcu->mcu_ud.ud_phy_adctrl_slv_dly);
	phy_write_reg(mcu_id, MCU_PHY_GRP_SLAVE_DELAY_14_ADDR, regd);
}

void set_master_dly(unsigned int mcu_id)
{
	unsigned int regd;

	/* PHY_MASTER_DELAY_START */
	regd = phy_read_reg(mcu_id, MCU_PHY_MASTER_DELAY_START_0_ADDR);
	regd = FIELD_MCU_PHY_MASTER_DELAY_START_0_SET(regd, PHY_MASTER_DELAY_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_MASTER_DELAY_START_0_ADDR, regd);

	/* PHY_MASTER_DELAY_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_MASTER_DELAY_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_MASTER_DELAY_STEP_0_SET(regd, PHY_MASTER_DELAY_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_MASTER_DELAY_STEP_0_ADDR, regd);

	/* PHY_MASTER_DELAY_WAIT */
	regd = phy_read_reg(mcu_id, MCU_PHY_MASTER_DELAY_WAIT_0_ADDR);
	regd = FIELD_MCU_PHY_MASTER_DELAY_WAIT_0_SET(regd, PHY_MASTER_DELAY_WAIT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_MASTER_DELAY_WAIT_0_ADDR, regd);

	/* PHY_ADRCTL_MASTER_DELAY_START */
	regd = phy_read_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_START_ADDR);
	regd = FIELD_MCU_PHY_ADRCTL_MASTER_DELAY_START_SET(regd, PHY_ADRCTL_MASTER_DELAY_START);
	phy_write_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_START_ADDR, regd);

	/* PHY_ADRCTL_MASTER_DELAY_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_STEP_ADDR);
	regd = FIELD_MCU_PHY_ADRCTL_MASTER_DELAY_STEP_SET(regd, PHY_ADRCTL_MASTER_DELAY_STEP);
	phy_write_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_STEP_ADDR, regd);

	/* PHY_ADRCTL_MASTER_DELAY_WAIT */
	regd = phy_read_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_WAIT_ADDR);
	regd = FIELD_MCU_PHY_ADRCTL_MASTER_DELAY_WAIT_SET(regd, PHY_ADRCTL_MASTER_DELAY_WAIT);
	phy_write_reg(mcu_id, MCU_PHY_ADRCTL_MASTER_DELAY_WAIT_ADDR, regd);
}

void set_tsel_timing(struct apm_mcu * mcu)
{
	unsigned int regd;
	unsigned int mcu_id = mcu->id;

	/* PHY_DQ_IE_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_IE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_IE_TIMING_0_SET(regd, PHY_DQ_IE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_IE_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQ_IE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQ_IE_TIMING_0_SET(regd, PHY_DQ_IE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQ_IE_TIMING_0_ADDR, regd);

	/* PHY_DQ_OE_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_OE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_OE_TIMING_0_SET(regd, PHY_DQ_OE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_OE_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQ_OE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQ_OE_TIMING_0_SET(regd, PHY_DQ_OE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQ_OE_TIMING_0_ADDR, regd);

	/* PHY_DQ_TSEL_WR_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_TSEL_WR_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_TSEL_WR_TIMING_0_SET(regd, PHY_DQ_TSEL_WR_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_TSEL_WR_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQ_TSEL_WR_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQ_TSEL_WR_TIMING_0_SET(regd, PHY_DQ_TSEL_WR_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQ_TSEL_WR_TIMING_0_ADDR, regd);

	/* PHY_DQ_TSEL_RD_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_TSEL_RD_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_TSEL_RD_TIMING_0_SET(regd, PHY_DQ_TSEL_RD_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_TSEL_RD_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQ_TSEL_RD_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQ_TSEL_RD_TIMING_0_SET(regd, PHY_DQ_TSEL_RD_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQ_TSEL_RD_TIMING_0_ADDR, regd);

	/* PHY_DQ_TSEL_ENABLE */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_TSEL_ENABLE_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_TSEL_ENABLE_0_SET(regd, mcu->mcu_ud.ud_phy_dq_tsel_enable);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_TSEL_ENABLE_0_ADDR, regd);

	/* PHY_DQ_TSEL_SELECT */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQ_TSEL_SELECT_0_ADDR);
	regd = FIELD_MCU_PHY_DQ_TSEL_SELECT_0_SET(regd, mcu->mcu_ud.ud_phy_dq_tsel_select);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQ_TSEL_SELECT_0_ADDR, regd);

	/* PHY_DQS_IE_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_IE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_IE_TIMING_0_SET(regd, PHY_DQS_IE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_IE_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQS_IE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQS_IE_TIMING_0_SET(regd, PHY_DQS_IE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQS_IE_TIMING_0_ADDR, regd);

	/* PHY_DQS_OE_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_OE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_OE_TIMING_0_SET(regd, PHY_DQS_OE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_OE_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQS_OE_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQS_OE_TIMING_0_SET(regd, PHY_DQS_OE_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQS_OE_TIMING_0_ADDR, regd);

	/* PHY_DQS_TSEL_WR_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_TSEL_WR_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_TSEL_WR_TIMING_0_SET(regd, PHY_DQS_TSEL_WR_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_TSEL_WR_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQS_TSEL_WR_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQS_TSEL_WR_TIMING_0_SET(regd, PHY_DQS_TSEL_WR_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQS_TSEL_WR_TIMING_0_ADDR, regd);

	/* PHY_DQS_TSEL_RD_TIMING */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_TSEL_RD_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_TSEL_RD_TIMING_0_SET(regd, PHY_DQS_TSEL_RD_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_TSEL_RD_TIMING_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_DQS_TSEL_RD_TIMING_0_ADDR);
	regd = FIELD_MCU_PHY_X4_DQS_TSEL_RD_TIMING_0_SET(regd, PHY_DQS_TSEL_RD_TIMING);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_DQS_TSEL_RD_TIMING_0_ADDR, regd);

	/* PHY_DQS_TSEL_ENABLE */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_TSEL_ENABLE_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_TSEL_ENABLE_0_SET(regd, mcu->mcu_ud.ud_phy_dqs_tsel_enable);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_TSEL_ENABLE_0_ADDR, regd);

	/* PHY_DQS_TSEL_SELECT */
	regd = phy_read_reg(mcu_id, MCU_PHY_DQS_TSEL_SELECT_0_ADDR);
	regd = FIELD_MCU_PHY_DQS_TSEL_SELECT_0_SET(regd, mcu->mcu_ud.ud_phy_dqs_tsel_select);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_DQS_TSEL_SELECT_0_ADDR, regd);
}

void set_drv_term(struct apm_mcu * mcu)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	unsigned int regd;
	unsigned int mcu_id = mcu->id;

	/* PHY_PAD_FDBK_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_FDBK_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_FDBK_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_fdbk_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_FDBK_DRIVE_ADDR, regd);

	/* PHY_PAD_X4_FDBK_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_X4_FDBK_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_fdbk_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_DRIVE_ADDR, regd);

	/* PHY_PAD_ADDR_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ADDR_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_ADDR_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_addr_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_ADDR_DRIVE_ADDR, regd);

	/* PHY_PAD_CLK_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_CLK_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_CLK_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_clk_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_CLK_DRIVE_ADDR, regd);

	/* PHY_PAD_PAR_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_PAR_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_PAR_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_par_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_PAR_DRIVE_ADDR, regd);

	/* PHY_PAD_ERR_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ERR_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_ERR_DRIVE_SET(regd, mcu->mcu_ud.ud_phy_pad_err_drive);
	phy_write_reg(mcu_id, MCU_PHY_PAD_ERR_DRIVE_ADDR, regd);

	/* PHY_PAD_VREF_CTRL */
	phy_vref_ctrl(mcu_id, udp->ud_phy_pad_vref_range, udp->ud_phy_pad_vref_value);

	/* PHY_PAD_ATB_CTRL */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ATB_CTRL_ADDR);
	regd = FIELD_MCU_PHY_PAD_ATB_CTRL_SET(regd, mcu->mcu_ud.ud_phy_pad_atb_ctrl);
	phy_write_reg(mcu_id, MCU_PHY_PAD_ATB_CTRL_ADDR, regd);

	/* PHY_PAD_DQS_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DQS_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_DQS_DRIVE_SET(regd, PHY_PAD_DQS_DRIVE);
	phy_write_reg(mcu_id, MCU_PHY_PAD_DQS_DRIVE_ADDR, regd);

	/* PHY_PAD_DATA_DRIVE */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DATA_DRIVE_ADDR);
	regd = FIELD_MCU_PHY_PAD_DATA_DRIVE_SET(regd, PHY_PAD_DATA_DRIVE | (mcu->ddr_info.device_type << 11));
	phy_write_reg(mcu_id, MCU_PHY_PAD_DATA_DRIVE_ADDR, regd);

	/* PHY_PAD_FDBK_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_FDBK_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_FDBK_TERM_SET(regd, PHY_PAD_FDBK_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_FDBK_TERM_ADDR, regd);

	/* PHY_PAD_X4_FDBK_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_X4_FDBK_TERM_SET(regd, PHY_PAD_FDBK_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_X4_FDBK_TERM_ADDR, regd);

	/* PHY_PAD_ADDR_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_ADDR_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_ADDR_TERM_SET(regd, PHY_PAD_ADDR_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_ADDR_TERM_ADDR, regd);

	/* PHY_PAD_CLK_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_CLK_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_CLK_TERM_SET(regd, PHY_PAD_CLK_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_CLK_TERM_ADDR, regd);

	/* PHY_PAD_DATA_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DATA_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_DATA_TERM_SET(regd, PHY_PAD_DATA_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_DATA_TERM_ADDR, regd);

	/* PHY_PAD_DQS_TERM */
	regd = phy_read_reg(mcu_id, MCU_PHY_PAD_DQS_TERM_ADDR);
	regd = FIELD_MCU_PHY_PAD_DQS_TERM_SET(regd, PHY_PAD_DQS_TERM);
	phy_write_reg(mcu_id, MCU_PHY_PAD_DQS_TERM_ADDR, regd);
}

void set_wrlvl_config(unsigned int mcu_id)
{
	unsigned int regd;

	/* PHY_WRLVL_DLY_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_DLY_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_DLY_STEP_0_SET(regd, PHY_WRLVL_DLY_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_DLY_STEP_0_ADDR, regd);

	/* PHY_WRLVL_CAPTURE_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_CAPTURE_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_CAPTURE_CNT_0_SET(regd, PHY_WRLVL_CAPTURE_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_CAPTURE_CNT_0_ADDR, regd);

	/* PHY_WRLVL_RESP_WAIT_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_RESP_WAIT_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_RESP_WAIT_CNT_0_SET(regd, PHY_WRLVL_RESP_WAIT_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_RESP_WAIT_CNT_0_ADDR, regd);

	/* PHY_WRLVL_UPDT_WAIT_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_UPDT_WAIT_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_UPDT_WAIT_CNT_0_SET(regd, PHY_WRLVL_UPDT_WAIT_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_UPDT_WAIT_CNT_0_ADDR, regd);

	/* PHY_WRLVL_DELAY_EARLY_THRESHOLD */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_DELAY_EARLY_THRESHOLD_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_DELAY_EARLY_THRESHOLD_0_SET(regd, PHY_WRLVL_DELAY_EARLY_THRESHOLD);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_DELAY_EARLY_THRESHOLD_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRLVL_DELAY_EARLY_THRESHOLD_0_ADDR);
	regd = FIELD_MCU_PHY_X4_WRLVL_DELAY_EARLY_THRESHOLD_0_SET(regd, PHY_WRLVL_DELAY_EARLY_THRESHOLD);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_WRLVL_DELAY_EARLY_THRESHOLD_0_ADDR, regd);

	/* PHY_WRLVL_EARLY_FORCE_ZERO */
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_EARLY_FORCE_0_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_EARLY_FORCE_0_0_SET(regd, PHY_WRLVL_EARLY_FORCE_ZERO);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_EARLY_FORCE_0_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRLVL_EARLY_FORCE_0_0_ADDR);
	regd = FIELD_MCU_PHY_X4_WRLVL_EARLY_FORCE_0_0_SET(regd, PHY_WRLVL_EARLY_FORCE_ZERO);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_WRLVL_EARLY_FORCE_0_0_ADDR, regd);

	/*  PHY_WRLVL_DELAY_PERIOD_THRESHOLD*/
	regd = phy_read_reg(mcu_id, MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR);
	regd = FIELD_MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_SET(regd, PHY_WRLVL_DELAY_PERIOD_THRESHOLD);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR);
	regd = FIELD_MCU_PHY_X4_WRLVL_DELAY_PERIOD_THRESHOLD_0_SET(regd, PHY_WRLVL_DELAY_PERIOD_THRESHOLD);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_WRLVL_DELAY_PERIOD_THRESHOLD_0_ADDR, regd);
}

void set_gtlvl_config(unsigned int mcu_id)
{
	unsigned int regd;

	/* PHY_GTLVL_CAPTURE_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_CAPTURE_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_CAPTURE_CNT_0_SET(regd, PHY_GTLVL_CAPTURE_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_CAPTURE_CNT_0_ADDR, regd);

	/* PHY_GTLVL_UPDT_WAIT_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_UPDT_WAIT_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_UPDT_WAIT_CNT_0_SET(regd, PHY_GTLVL_UPDT_WAIT_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_UPDT_WAIT_CNT_0_ADDR, regd);

	/* PHY_GTLVL_RESP_WAIT_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_RESP_WAIT_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_RESP_WAIT_CNT_0_SET(regd, PHY_GTLVL_RESP_WAIT_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_RESP_WAIT_CNT_0_ADDR, regd);

	/* PHY_GTLVL_DLY_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_DLY_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_DLY_STEP_0_SET(regd, PHY_GTLVL_DLY_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_DLY_STEP_0_ADDR, regd);

	/* PHY_GTLVL_FINAL_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_FINAL_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_FINAL_STEP_0_SET(regd, PHY_GTLVL_FINAL_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_FINAL_STEP_0_ADDR, regd);

	/* PHY_GTLVL_BACK_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_BACK_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_BACK_STEP_0_SET(regd, PHY_GTLVL_BACK_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_BACK_STEP_0_ADDR, regd);

	/* PHY_GTLVL_LAT_ADJ_START */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_LAT_ADJ_START_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_LAT_ADJ_START_0_SET(regd, PHY_GTLVL_LAT_ADJ_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_LAT_ADJ_START_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_GTLVL_LAT_ADJ_START_0_ADDR);
	regd = FIELD_MCU_PHY_X4_GTLVL_LAT_ADJ_START_0_SET(regd, PHY_GTLVL_LAT_ADJ_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_GTLVL_LAT_ADJ_START_0_ADDR, regd);

	/* PHY_GTLVL_RDDQS_SLV_DLY_START */
	regd = phy_read_reg(mcu_id, MCU_PHY_GTLVL_RDDQS_SLV_DLY_START_0_ADDR);
	regd = FIELD_MCU_PHY_GTLVL_RDDQS_SLV_DLY_START_0_SET(regd, PHY_GTLVL_RDDQS_SLV_DLY_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GTLVL_RDDQS_SLV_DLY_START_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_GTLVL_RDDQS_SLV_DLY_START_0_ADDR);
	regd = FIELD_MCU_PHY_X4_GTLVL_RDDQS_SLV_DLY_START_0_SET(regd, PHY_GTLVL_RDDQS_SLV_DLY_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_GTLVL_RDDQS_SLV_DLY_START_0_ADDR, regd);
}

void set_rdlvl_config(unsigned int mcu_id)
{
	unsigned int regd;

	/* PHY_RDLVL_CAPTURE_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_CAPTURE_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_CAPTURE_CNT_0_SET(regd, PHY_RDLVL_CAPTURE_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_CAPTURE_CNT_0_ADDR, regd);

	/* PHY_RDLVL_UPDT_WAIT_CNT */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_UPDT_WAIT_CNT_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_UPDT_WAIT_CNT_0_SET(regd, PHY_RDLVL_UPDT_WAIT_CNT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_UPDT_WAIT_CNT_0_ADDR, regd);

	/* PHY_RDLVL_OP_MODE */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_OP_MODE_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_OP_MODE_0_SET(regd, PHY_RDLVL_OP_MODE);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_OP_MODE_0_ADDR, regd);

	/* PHY_RDLVL_DLY_STEP */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_DLY_STEP_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_DLY_STEP_0_SET(regd, PHY_RDLVL_DLY_STEP);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_DLY_STEP_0_ADDR, regd);

	/* PHY_RDLVL_DATA_MASK */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_DATA_MASK_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_DATA_MASK_0_SET(regd, PHY_RDLVL_DATA_MASK);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_DATA_MASK_0_ADDR, regd);

	/* PHY_RDLVL_RDDQS_DQ_SLV_DLY_START */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_SLV_DLY_START_0_ADDR);
	regd = FIELD_MCU_PHY_RDLVL_RDDQS_DQ_SLV_DLY_START_0_SET(regd, PHY_RDLVL_RDDQS_DQ_SLV_DLY_START);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDLVL_RDDQS_DQ_SLV_DLY_START_0_ADDR, regd);

	/* PHY_RDDQS_GATE_SLAVE_DELAY */
	regd = phy_read_reg(mcu_id, MCU_PHY_RDDQS_GATE_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_RDDQS_GATE_SLAVE_DELAY_0_SET(regd, PHY_RDDQS_GATE_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_RDDQS_GATE_SLAVE_DELAY_0_ADDR, regd);

	regd = phy_read_reg(mcu_id, MCU_PHY_X4_RDDQS_GATE_SLAVE_DELAY_0_ADDR);
	regd = FIELD_MCU_PHY_X4_RDDQS_GATE_SLAVE_DELAY_0_SET(regd, PHY_RDDQS_GATE_SLAVE_DELAY);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_X4_RDDQS_GATE_SLAVE_DELAY_0_ADDR, regd);

	/* PHY_GATE_ERROR_DELAY_SELECT */
	regd = phy_read_reg(mcu_id, MCU_PHY_GATE_ERROR_DELAY_SELECT_0_ADDR);
	regd = FIELD_MCU_PHY_GATE_ERROR_DELAY_SELECT_0_SET(regd, PHY_GATE_ERROR_DELAY_SELECT);
	phy_wr_all_data_slices(mcu_id, MCU_PHY_GATE_ERROR_DELAY_SELECT_0_ADDR, regd);
}
