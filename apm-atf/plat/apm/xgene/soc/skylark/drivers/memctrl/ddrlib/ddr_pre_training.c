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

#include "ddr_lib.h"

/*  Primary DDR initialization task */
int ddr_pre_training(struct apm_memc *memc, unsigned int flag)
{
	struct apm_mcu *mcu;
	int err = 0;
	unsigned int iia;

	/*
	 * SPD init & setup for bringup
	 * Note: get_spd() is outside of library
	 * add mimic spd as alternative
	 */
	ddr_verbose("\nDRAM: SPD discovery\n");
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&(memc->mcu[iia]);
		if (!mcu->enabled)
			continue;

                err = spd_init(mcu);

		/* Update mcu_mask with the actual populated MCUs */
		memc->mcu_mask |= mcu->enabled << iia;
	}
	memc->p_progress_bar(5);

	if (memc->p_board_setup != NULL)
		memc->p_board_setup(memc);

	err = (memc->mcu_mask == 0)? 1: 0;
	if (err) {
		ddr_err("No DIMM populated!\n");
		return err;
	}


	/* Configure addressing mode */
	err = pcp_addressing_mode(memc);
	if (err) {
		ddr_err("PCP Address configuration error\n");
		return err;
	}

#if !XGENE_VHP
	/* Validate PCP configurtaion */
	err = pcp_config_check(memc);
	if (err) {
		ddr_err("PCP configuration check error\n");
		return err;
	}
#endif

	ddr_verbose("MCU/PHY-PLL turn on and Unreset memory sub-system\n");

	err = mcu_unreset(memc);
	if (err) {
		ddr_err("Fail to take memory subsystem out of reset!\n");
		return err;
	}

	/* MCU CSR setup & Phy Init */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&(memc->mcu[iia]);
		if (!mcu->enabled)
			continue;

		config_dmc_parameters(mcu);
		update_pmu_registers(mcu);
                update_bist_addressing(mcu);
		update_bist_timing_registers(mcu->id);
#if !XGENE_EMU
		err = phy_csr_config(mcu);
		update_x4_fdbk_term_pvt(mcu);
		if (err) {
			ddr_err("MCU-PHY[%0d] DLL Init Failed\n", mcu->id);
			return err;
		}
#endif
	}

	for_each_mcu(iia) {
                mcu = (struct apm_mcu *)&(memc->mcu[iia]);
		if (!mcu->enabled)
			continue;

#if (!XGENE_EMU && !XGENE_VHP)
		err = phy_sw_rx_calibration(mcu->id, mcu->ddr_info.device_type,
					    mcu->mcu_ud.ud_sw_rx_cal_en);
		if (err) {
			ddr_err("MCU-PHY[%0d] SW RX Calibration Failed\n", mcu->id);
			return err;
		}
#endif
	}

#if !XGENE_EMU
	if (memc->memc_ud.ud_phy_lpbk_option != NOPHYLPBK) {
		/*
		 * Phy PRBS Loopback routine
		 */
		for_each_mcu(iia) {
			mcu = (struct apm_mcu *)&memc->mcu[iia];
			if (!mcu->enabled)
				continue;
			ddr_info("MCU[%d] - Phy-PRBS \n", iia);
			err += phy_loopback_test(mcu, memc->memc_ud.ud_phy_lpbk_option);
		}
		if (err)
			ddr_err("Phy PRBS Loopback [%d]\n", err);
		/* Phy un-usable after PRBS Loopback - must reset MC */
		return err;
	}
#endif
	/*
	 * MCU post power of initialization
	 */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		ddr_info("MCU[%d] - Dimm-Init \n", iia);
		dram_init(mcu);
		config_rcd_buffer(mcu);
		dram_mrs_program(mcu);
		dram_zqcl(mcu);
	}
	memc->p_progress_bar(10);

	return 0;
}

