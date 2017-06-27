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

/*
 * Phy Train
 */
int ddr_phy_training(struct apm_memc *memc, unsigned int flag)
{
	int err = 0;
	struct apm_mcu *mcu;
	unsigned int iia;

	ddr_pr("PHY calibrating ... ");
	ddr_info("\n");
	memc->p_pre_ddr_tlb_map(NULL, 0);
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		ddr_info("MCU[%d] - PHY Train \n", iia);
		err = phy_training_mode(mcu);
                if (err) {
			ddr_err("MCU[%d] - PHY Training Failure \n", iia);
			return -1;
                }
		memc->p_progress_bar(10 + iia * 10);
	}
	ddr_info("\n");
	ddr_pr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

	return err;
}

/* PHY Training routine */
int phy_training_mode(struct apm_mcu *mcu)
{
	int iia, err = 0;
	unsigned int __attribute__ ((unused)) rddata;
	unsigned int err_wrlvl, err_rdgate, err_rdlvl, err_wrcal;
	unsigned int err_wrdeskew, err_rddeskew, wrdeskew_en, rddeskew_en;
	unsigned int err_dramvref, err_phyvref = 0;
	unsigned int mcu_id = mcu->id;

	/* Need to include the flow for RDIMM and LRDIMM */
	for (iia = 0; iia < MCU_SUPPORTED_RANKS; iia++) {
		err_wrlvl = 0;
		err_rdgate = 0;
		err_rdlvl = 0;
		err_wrcal = 0;
		err_dramvref = 0;

		/* Check for active ranks */
		if ((mcu->ddr_info.physical_ranks & (1 << iia)) == 0)
			continue;

		/* Check for per training cs Enable */
		if ((get_per_cs_training_en(mcu_id) == 0) && (iia != 0))
			continue;

		/* Update CS Training Index as per target Rank */
		update_per_cs_training_index(mcu_id, iia);

		/* Write Leveling */
		err_wrlvl = dmc_wrlvl_routine(mcu, iia);
		if (err_wrlvl)
			return -1;

		/* RD Gate Leveling */
		err_rdgate = dmc_rdgate_routine(mcu, iia);
		if (err_rdgate)
			err |= (1 << iia);

		/* RD Leveling */
		if (err_rdgate == 0) {
			/* Update Read Pattern if required */
			dmc_rdlvl_pattern(mcu, iia);
			err_rdlvl = dmc_rdlvl_routine(mcu, iia);
		}

		if (err_rdlvl)
			err |= (1 << iia);

		/* Write Calibration */
		if (!err)
			err_wrcal = mcu_bist_phy_wrcal(mcu, iia);

		if (err_wrcal)
			err |= (1 << iia);

		/* DRAM VREFDQ training */
		if (!err)
			err_dramvref = mcu_dram_vref_training(mcu, iia);

		if (err_dramvref)
			err |= (1 << iia);

		if (err & (1 << iia))
			ddr_err("MCU-PHY[%d] rnk:%d HW Leveling/DIMM Vref Training errs:0x%x\n"
				"\tWR-LVL[0x%x], RDG[0x%x], RDE[0x%x]\n"
				"\tWR-CAL[0x%x], DRAM-VREF[0x%x]\n",
				mcu->id, iia, err, err_wrlvl, err_rdgate,
				err_rdlvl, err_wrcal, err_dramvref);
	}

	/* PHY VREF training */
	if (!err)
		err_phyvref = mcu_phy_vref_training(mcu, mcu->ddr_info.active_ranks);

	if (err_phyvref) {
		err |= (1 << MCU_SUPPORTED_RANKS);
		ddr_err("MCU-PHY[%d] PHY Vref Training errors:0x%x PHY-VREF:[0x%x]\n",
			mcu->id, err, err_phyvref);
        }

	/* Software Training for per bit deskew */
	for (iia = 0; iia < MCU_SUPPORTED_RANKS; iia++) {
		err_wrdeskew = 0;
		err_rddeskew = 0;

		wrdeskew_en = (mcu->mcu_ud.ud_deskew_en == DESKEW_WRDESKEW) |
			      (mcu->mcu_ud.ud_deskew_en == DESKEW_EN_BOTH) ;
		rddeskew_en = (mcu->mcu_ud.ud_deskew_en == DESKEW_RDDESKEW) |
			      (mcu->mcu_ud.ud_deskew_en == DESKEW_EN_BOTH) ;

		/* Check for active ranks */
		if ((mcu->ddr_info.physical_ranks & (1 << iia)) == 0)
			continue;

		/* Check for per cs training Enable */
		if ((get_per_cs_training_en(mcu_id) == 0) && (iia != 0))
			continue;

		/* Update CS Training Index as per target Rank */
		update_per_cs_training_index(mcu_id, iia);

		/* Write Deskew */
		if ((!err) & wrdeskew_en)
			err_wrdeskew = mcu_bist_phy_wrdeskew(mcu, iia);

		/* Read Deskew */
		if ((!err) & rddeskew_en)
			err_rddeskew = mcu_bist_phy_rddeskew(mcu, iia);

		if (err_wrdeskew)
			err |= (1 << iia);

		if (err_rddeskew)
			err |= (1 << iia);

		if (err & (1 << iia)) {
			ddr_err("MCU-PHY[%d] rnk:%d SW Leveling errs:0x%x\n"
				"\tWRDESKEW:[0x%x], RDDESKEW:[0x%x]\n",
				mcu->id, iia, err, err_wrdeskew, err_rddeskew);
		}
	}

	if (err)
		return -1;

	return 0;
}
