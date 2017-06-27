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
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED.
 */

/*
 * This is the main init function file. The init is attached to the
 * apm_memc structure. These are the top level init function used
 * to initialize the system's ddr controllers.
 */
#include "ddr_lib.h"

/*  Mcu  User Defined parameters to known state  */
void ddr_sys_default_params_mcu_setup(struct apm_mcu_udparam *udp)
{
	/* User Defined SPD etc */
	udp->ud_ddr_mimicspd            = DEFAULT_MIMICSPD;
	udp->ud_mimic_activeslots       = DEFAULT_ACTIVE_SLOTS;
	udp->ud_rc11_dimmvdd            = DEFAULT_RC11_DIMMVDD;

	/* Mcu training  User Params */
	udp->ud_singlerank_train_mode   = DEFAULT_PHY_TRAIN_MODE;
	udp->ud_physlice_en             = DEFAULT_PHY_SLICE_ON;

	/* User Defined overrides for PLL params */
	udp->ud_pll_force_en            = DEFAULT_FORCE_PLL;
	udp->ud_pllctl_fbdivc           = DEFAULT_PLL_FBDIVC;
	udp->ud_pllctl_outdiv2          = DEFAULT_PLL_OUTDIV2;
	udp->ud_pllctl_outdiv3          = DEFAULT_PLL_OUTDIV3;
	udp->ud_speed_grade             = DEFAULT_DDR_SPEED;

	/* User Defined overrides for MCU Configuration */
	udp->ud_en2tmode                = DEFAULT_2T_MODE;
	udp->ud_addr_decode             = DEFAULT_ADDR_DECODE;
	udp->ud_stripe_decode           = DEFAULT_STRIPE_DECODE;
	udp->ud_ecc_enable              = DEFAULT_ECC_MODE;
	udp->ud_crc_enable              = DEFAULT_CRC;
	udp->ud_rd_dbi_enable           = DEFAULT_RD_DBI;
	udp->ud_wr_dbi_enable           = DEFAULT_WR_DBI;
	udp->ud_geardown_enable         = DEFAULT_GEARDOWN;
	udp->ud_bank_hash_enable        = DEFAULT_BANK_HASH;
	udp->ud_refresh_granularity     = DEFAULT_REFRESH_GRANULARITY;
	udp->ud_pmu_enable              = DEFAULT_PMU_ENABLE;
	udp->ud_write_preamble_2t       = WR_PREAMBLE_2T_MODE;
	udp->ud_deskew_en               = DEFAULT_DESKEW_EN;
	udp->ud_sw_rx_cal_en            = DEFAULT_SW_RX_CAL_EN;

	/* RTT related controls */
	udp->ud_rtt_wr                  = DEFAULT_RTT_WR;
	udp->ud_rtt_nom_s0              = DEFAULT_RTT_NOM_SLOT0;
	udp->ud_rtt_nom_s1              = DEFAULT_RTT_NOM_SLOT1;
	udp->ud_rtt_park_s0             = DEAFULT_RTT_PARK_SLOT0;
	udp->ud_rtt_park_s1             = DEAFULT_RTT_PARK_SLOT1;
	udp->ud_rtt_nom_wrlvl_tgt       = DEFAULT_RTT_NOM_WRLVL;
	udp->ud_rtt_nom_wrlvl_non_tgt   = DEFAULT_RTT_NOM_WRLVL_NONTGT;
	/* MR1 D.I.C Driver Impedance Control */
	udp->ud_mr1_dic                 = DEAFULT_MR1_DIC;

	/* User Defined margin to Mcu timing params */
	udp->ud_force_cl                = DEFAULT_CL;
	udp->ud_force_cwl               = DEFAULT_CWL;
	udp->ud_rtr_s_margin            = DEFAULT_RTR_S_MARGIN;
	udp->ud_rtr_l_margin            = DEFAULT_RTR_L_MARGIN;
	udp->ud_rtr_cs_margin           = DEFAULT_RTR_CS_MARGIN;
	udp->ud_wtw_s_margin            = DEFAULT_WTW_S_MARGIN;
	udp->ud_wtw_l_margin            = DEFAULT_WTW_L_MARGIN;
	udp->ud_wtw_cs_margin           = DEFAULT_WTW_CS_MARGIN;
	udp->ud_rtw_s_margin            = DEFAULT_RTW_S_MARGIN;
	udp->ud_rtw_l_margin            = DEFAULT_RTW_L_MARGIN;
	udp->ud_rtw_cs_margin           = DEFAULT_RTW_CS_MARGIN;
	udp->ud_wtr_s_margin            = DEFAULT_WTR_S_MARGIN;
	udp->ud_wtr_l_margin            = DEFAULT_WTR_L_MARGIN;
	udp->ud_wtr_cs_margin           = DEFAULT_WTR_CS_MARGIN;

	/* User Defined ODT timing */
	udp->ud_rdodt_off_margin        = DEFAULT_RDOTTOFF_MARGIN;
	udp->ud_wrodt_off_margin        = DEFAULT_WROTTOFF_MARGIN;

	/* User Defined VREF settings */
	udp->ud_dram_vref_range		= DEFAULT_DRAM_VREFDQ_RANGE;
	udp->ud_dram_vref_value		= DEFAULT_DRAM_VREFDQ_VALUE;
	udp->ud_phy_pad_vref_range	= DEFAULT_PHY_PAD_VREFDQ_RANGE;
	udp->ud_phy_pad_vref_value	= DEFAULT_PHY_PAD_VREF_VALUE;
	udp->ud_dram_vref_train_en	= DEFAULT_DRAM_VREFDQ_TRAIN_EN;
	udp->ud_dram_vref_finetune_en	= DEFAULT_DRAM_VREFDQ_FINETUNE_EN;
	udp->ud_phy_vref_train_en	= DEFAULT_PHY_VREF_TRAIN_EN;

	/* User Defined overrides for Phy Calib Mode(ZQ) */
	/* ase Interval
	 * b00 = 256 cycles
	 * b01 = 64K cycles
	 * b10 = 16M cycles  */
	udp->ud_phy_cal_mode_on         = DEFAULT_CAL_MODE_ON;
	udp->ud_phy_cal_base_interval   = DEFAULT_CAL_BASE_INTERVAL;
	udp->ud_phy_cal_interval_count_0= DEFAULT_CAL_INTERVAL_COUNT;

	/* User Defined Phy Drive & Term values */
	udp->ud_phy_pad_fdbk_drive      = DEFAULT_PAD_FDBK_DRIVE;
	udp->ud_phy_pad_addr_drive      = DEFAULT_PAD_ADDR_DRIVE;
	udp->ud_phy_pad_clk_drive       = DEFAULT_PAD_CLK_DRIVE;
	udp->ud_phy_pad_par_drive       = DEFAULT_PAD_PAR_DRIVE;
	udp->ud_phy_pad_err_drive       = DEFAULT_PAD_ERR_DRIVE;
	udp->ud_phy_pad_atb_ctrl        = DEFAULT_PAD_ATB_CTRL;
	udp->ud_phy_adctrl_slv_dly      = DEFAULT_ADCTRL_SLV_DLY;
	udp->ud_phy_dq_tsel_enable      = DEFAULT_DQ_TSEL_ENABLE;
	udp->ud_phy_dqs_tsel_enable     = DEFAULT_DQS_TSEL_ENABLE;
	udp->ud_phy_dq_tsel_select      = DEFAULT_DQ_TSEL_SELECT;
	udp->ud_phy_dqs_tsel_select     = DEFAULT_DQS_TSEL_SELECT;
}

/*  Mcb  User Defined parameters to known state  */
void ddr_sys_default_params_mcb_setup(struct apm_mcb_udparam *mcbudp)
{
	mcbudp->ud_spec_read            = MCU_SPEC_READ;
}

/*  Memc  User Defined parameters to known state  */
void ddr_sys_default_params_memc_setup(struct apm_memc *memc)
{
	memc->memc_ud.ud_mcu_enable_mask          = MCU_ENABLE_MASK;
	memc->memc_ud.ud_hash_en                  = DEFAULT_HASH_EN;
	memc->memc_ud.ud_interleaving_en          = DEFAULT_INTERLEAVING_EN;
	memc->memc_ud.ud_ignore_init_cecc_err     = 0;
	memc->memc_ud.ud_ignore_init_parity_err   = 0;
	memc->memc_ud.ud_reset_on_phytrain_err    = 0;
	memc->memc_ud.ud_bg_scrub                 = 0;
	memc->memc_ud.ud_page_mode                = 1;

	memc->memc_ud.ud_phy_lpbk_option          = NOPHYLPBK;
	memc->memc_ud.ud_spd_get		  = NULL;
}

/*  Library User Defined parameters to known state  */
void ddr_sys_default_params_setup(struct apm_memc *memc)
{
	struct apm_mcu *mcu;
	struct apm_mcb *mcb;
	apm_mcu_udparam_t *udp;
	apm_mcb_udparam_t *mcbudp;
	unsigned int iia;

	/* Initialize User Defined Data Structure to okay values */
	ddr_verbose("DRAM: user_defined setup\n");

	for (iia = 0; iia < CONFIG_SYS_NUM_DDR_CTLRS/CONFIG_SYS_NUM_MCU_PER_MCB; iia++) {
		mcb = (struct apm_mcb *) &memc->mcb[iia];
		mcbudp = (apm_mcb_udparam_t *) &mcb->mcb_ud;
		ddr_sys_default_params_mcb_setup(mcbudp);
	}

	for_each_mcu(iia) {
		mcu = (struct apm_mcu *) &memc->mcu[iia];
		mcu->parent = (void*) memc;
		udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
		ddr_sys_default_params_mcu_setup(udp);
	}

	/*  Overrides for specific Mcu here */
	ddr_sys_default_params_memc_setup(memc);
}

//======================================================================
int populate_mc_default_params(void *ptr)
{
	unsigned int err = 0;
	struct apm_memc *memc = (struct apm_memc *)ptr;
	struct apm_mcu *mcu;

	unsigned int iia;

	/* Initialize DDR structure */
	memc->full_addr = 1;

	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
	}

	// memory region
	memc->memspace.num_mem_regions = 0;
	memc->memspace.str_addr[0] = 0x0;
	memc->memspace.end_addr[0] = 0x0;
	memc->memspace.str_addr[1] = 0x0;
	memc->memspace.end_addr[1] = 0x0;
	memc->memspace.str_addr[2] = 0x0;
	memc->memspace.end_addr[2] = 0x0;
	memc->memspace.str_addr[3] = 0x0;
	memc->memspace.end_addr[3] = 0x0;

	return err;
}

