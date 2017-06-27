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
 *     Phy LoopBack (BIST) tests
 *****************************************************************************/
int phy_loopback_test(apm_mcu_t *mcu, phylpbkopt_e lpbk_opt)
{
	int err = 0;
	switch (lpbk_opt) {
	case PHYLPBK_INTLSFR1K:
		ddr_verbose("Starting INTLSFR1K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_1024, LPBKDTYPE_LFSR, LPBKMUX_INT);
		break;
	case PHYLPBK_INTLSFR8K:
		ddr_verbose("Starting INTLSFR8K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_8192, LPBKDTYPE_LFSR, LPBKMUX_INT);
		break;
	case PHYLPBK_INTLSFR64K:
		ddr_verbose("Starting INTLSFR64K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_64k, LPBKDTYPE_LFSR, LPBKMUX_INT);
		break;
	case PHYLPBK_INTCLKP1K:
		ddr_verbose("Starting INTCLKP1K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_1024, LPBKDTYPE_CLKPTRN, LPBKMUX_INT);
		break;
	case PHYLPBK_INTCLKP8K:
		ddr_verbose("Starting INTCLKP8K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_8192, LPBKDTYPE_CLKPTRN, LPBKMUX_INT);
		break;
	case PHYLPBK_INTCLKP64K:
		ddr_verbose("Starting INTCLKP64K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_64k, LPBKDTYPE_CLKPTRN, LPBKMUX_INT);
		break;
	case PHYLPBK_EXTLSFR1K:
		ddr_verbose("Starting EXTLSFR1K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_1024, LPBKDTYPE_LFSR, LPBKMUX_EXT);
		break;
	case PHYLPBK_EXTLSFR8K:
		ddr_verbose("Starting EXTLSFR8K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_8192, LPBKDTYPE_LFSR, LPBKMUX_EXT);
		break;
	case PHYLPBK_EXTLSFR64K:
		ddr_verbose("Starting EXTLSFR64K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_64k, LPBKDTYPE_LFSR, LPBKMUX_EXT);
		break;
	case PHYLPBK_EXTCLKP1K:
		ddr_verbose("Starting EXTCLKP1K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_1024, LPBKDTYPE_CLKPTRN, LPBKMUX_EXT);
		break;
	case PHYLPBK_EXTCLKP8K:
		ddr_verbose("Starting EXTCLKP8K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_8192, LPBKDTYPE_CLKPTRN, LPBKMUX_EXT);
		break;
	case PHYLPBK_EXTCLKP64K:
		ddr_verbose("Starting EXTCLKP64K\n");
		err = phy_lpbck_sequence(mcu, LPBKCNT_64k, LPBKDTYPE_CLKPTRN, LPBKMUX_EXT);
		break;
	case NOPHYLPBK:
		ddr_verbose("Not Starting Test: NOPHYLPBK\n");
	default:
		break;
	}
	return err;
}


int  phy_lpbck_sequence(apm_mcu_t *mcu, phylpbkcnt_e ctrl_lpbk_cnt,
			phylpbkdtype_e ctrl_data_type, phylpbkmux_e lpbk_mux_ext)
{
	unsigned int lpbk_result, regd, flds, jj, cntr;
	unsigned int ptimeout = 300;
	unsigned int mcu_id = mcu->id;
	int err = 0;
	/*  Note: takes approx 120us to run 64k clocks */

	ddr_verbose("MCU-PHY[%01d] Loopback sequence start!\n",mcu->id);
	ddr_verbose("Loopback  ctrl_lpbk_cnt=%01x dtype=%01x mux_ext=%01x\n",
		  ctrl_lpbk_cnt, ctrl_data_type, lpbk_mux_ext);

	/* Turn on all slices */
	for (jj = 0 ; jj < CDNPHY_NUM_SLICES ; jj++) {
		/* { GO, ctrl_lpbk_cnt, err_clr, ctrl_data_type, lpbk_mux_ext, EN } */
		regd = phy_read_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET);
		flds = 0x49 | (((uint32_t) lpbk_mux_ext & 0x1) << 1) |
			(((uint32_t) ctrl_data_type & 0x1) << 2) |
			(((uint32_t) ctrl_lpbk_cnt & 0x3) << 4) ;
		regd = FIELD_MCU_PHY_LPBK_CONTROL_0_SET(regd, flds);
		phy_write_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET, regd);
		/* Clear ctrl_data_err_clr */
		flds &= 0xF7;
		regd = FIELD_MCU_PHY_LPBK_CONTROL_0_SET(regd, flds);
		phy_write_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET, regd);
	}
	/* Check for done / err & turn off */
	for (jj = 0 ; jj < CDNPHY_NUM_SLICES ; jj++) {
		cntr = ptimeout;
		do {
			if (cntr < ptimeout)
				DELAY(1);
			lpbk_result = phy_read_reg(mcu_id,
					MCU_PHY_LPBK_RESULT_OBS_0_ADDR + jj * PHY_SLICE_OFFSET);
			cntr--;
		} while ((((lpbk_result >> 21) & 0x1) == 0) &&
			(((lpbk_result >> 19) & 0x1) == 0) &&
			(cntr != 0));
		ddr_verbose("LPBK result ln=%01d d=%01x ", jj , (lpbk_result >> 21) & 0x1);
		ddr_verbose("nsync=%01x e=%01x ", (lpbk_result >> 20) & 0x1,
							(lpbk_result >> 19) & 0x1);
		ddr_verbose("eb=%01x expd=0x%02x ", (lpbk_result >> 18) & 0x1,
							(lpbk_result >> 9) & 0x1FF);
		ddr_verbose("errd=0x%02x (count=%d)\n", lpbk_result & 0x1FF, cntr);
		if ((cntr == 0) || (((lpbk_result >> 19) & 0x1) == 1) ||
					(((lpbk_result >> 21) & 0x1) != 1)) {
			ddr_err("MCU-PHY[%01d] LPBK FAILED slice=%01d see results! ", mcu->id, jj);
			err++;
		}
		/* Shut off Loop Back mode */
		regd = phy_read_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET);
		flds = 0x09 | (((uint32_t) lpbk_mux_ext & 0x1) << 1) |
			(((uint32_t) ctrl_data_type & 0x1) << 2) |
			(((uint32_t) ctrl_lpbk_cnt & 0x3) << 4) ;
		regd = FIELD_MCU_PHY_LPBK_CONTROL_0_SET(regd, flds);
		phy_write_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET, regd);
		flds &= 0xB6;
		regd = FIELD_MCU_PHY_LPBK_CONTROL_0_SET(regd, flds);
		phy_write_reg(mcu_id, MCU_PHY_LPBK_CONTROL_0_ADDR + jj * PHY_SLICE_OFFSET, regd);
	}
	if (err)
		return -1;

	ddr_verbose("\nMCU-PHY[%01d] Loopback test PASS [Count:%d Data Type: %d Internal/External:%d]\n",
		mcu->id, (unsigned int)ctrl_lpbk_cnt, (unsigned int)ctrl_data_type,
		(unsigned int)lpbk_mux_ext);
	ddr_verbose("MCU-PHY[%01d] Loopback finished! Errors=%0d\n", mcu->id, err);
	return 0;
}
