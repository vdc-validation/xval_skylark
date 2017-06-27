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
#include "ddr_mcu_bist.h"

/* PDA */
#define PDA_ENTRY		0x1
#define PDA_EXIT		0x0

/* VREFDQ */
#define DRAM_VREFDQ_TRAIN_MODE		0x1
#define DRAM_VREFDQ_NORMAL_MODE		0x0
#define DRAM_VREFDQ_WINDOW_STEP		4
#define DRAM_VREFDQ_TRAIN_STEP_ALL	2
#define DRAM_VREFDQ_TRAIN_STEP_PDA	1
#define DRAM_VREFDQ_MIN_VALUE		0x0
#define DRAM_VREFDQ_MAX_VALUE		0x32

/* PHY VREF */
#define PHY_PAD_VREF_STEP		1
#define PHY_PAD_VREF_MIN_VALUE		0x0
#define PHY_PAD_VREF_MAX_VALUE		0x3F

/*
 * Step size for finding Wr/Rd DSKW Window
 */
#define STEP_LARGE			8
#define STEP_SMALL			2

phy_rnk_train_res_t deskewd;

/******************************************************************************
 *     Run Deskew BIST
 *****************************************************************************/

int run_wrdqdly_le_bist (struct apm_mcu *mcu, unsigned int slice, unsigned int bit,
			unsigned int physliceon, unsigned int *delay_le, unsigned int start_delay,
			unsigned int end_delay, unsigned int delay_step)
{
	unsigned int regd, bist_cmp, test, ecc_bitmask = 0;
	unsigned long long bitmask = 0;
	int bist_incomplete, bist_err, err;
	unsigned int delay;
	unsigned int mcu_id = mcu->id;
	unsigned int offset = slice * PHY_SLICE_OFFSET;

	*delay_le = start_delay;
	for (delay = start_delay; delay >= end_delay; delay -= delay_step) {
		regd = phy_read_reg(mcu_id, wrdskw_get_reg_offset(bit) + offset);
		regd = wrdskw_set_delay_value(bit, regd, delay);
		phy_write_reg(mcu_id, wrdskw_get_reg_offset(bit) + offset, regd);

		/* Run Ctrl Update to set programmed delays */
		err = phy_update(mcu_id);
		if (err) {
			ddr_err("In phy_update: %s\n", __func__);
			return err;
		}

		bist_incomplete = 0;
		bist_err = 0;
		/* check different data patterns per bist test*/
		for (test = 0; test < BIST_MAX_PATTERNS; test++) {
			update_bist_datapattern(mcu_id, test);
			/*
			 * Start Bist
			 */
			mcu_bist_start(mcu_id);
			/*
			 * Polling for BIST_DONE bit to be set
			 * bist_incomplete = 0 if BIST completed
			 * bist_incomplete = -1 if polling times out
			 */
			bist_incomplete = mcu_bist_completion_poll(mcu_id);

			if (!bist_incomplete)
				bist_err = mcu_bist_status(mcu_id);
			else
				mcu_bist_stop(mcu_id);
			if ((bist_incomplete == -1) || (bist_err == -1)) {
				/*
				 * Make sure you check the timeout calculation in the polling function,
				 * if you happen to be here
				 */
				ddr_err ("BIST Not Completed"
					"because of TIMEOUT or ALERT Register Set!!\n");
				return -1;
			} else if (bist_err > 0) {
				/*
				 * Bitmask for bits 0-63 in "bitmask"
				 * Bitmask for ecc bits 64-71 in "ecc_bitmask"
				 */
				if (slice < CDNPHY_NUM_SLICES - 1)
					bitmask = (0x1ULL << ((slice * 8) + bit));
				else
					ecc_bitmask = 0x1 << bit;
				bist_cmp = mcu_bist_datacmp(mcu_id, bitmask, ecc_bitmask);
				if (bist_cmp)
					break;
			} else if (bist_err == 0) {
				*delay_le = delay;
			}
		}
	}
	return 0;
}

int run_wrdqdly_te_bist (struct apm_mcu *mcu, unsigned int slice, unsigned int bit,
			unsigned int physliceon, unsigned int *delay_te, unsigned int start_delay,
			unsigned int end_delay, unsigned int delay_step)
{
	unsigned int regd, bist_cmp, test, ecc_bitmask = 0;
	unsigned long long bitmask = 0;
	int bist_incomplete, bist_err, err;
	unsigned int delay;
	unsigned int mcu_id = mcu->id;
	unsigned int offset = slice * PHY_SLICE_OFFSET;

	*delay_te = start_delay;
	for (delay = start_delay; delay <= end_delay; delay += delay_step) {
		regd = phy_read_reg(mcu_id, wrdskw_get_reg_offset(bit) + offset);
		regd = wrdskw_set_delay_value(bit, regd, delay);
		phy_write_reg(mcu_id, wrdskw_get_reg_offset(bit) + offset, regd);

		/* Run Ctrl Update to set programmed delays */
		err = phy_update(mcu_id);
		if (err) {
			ddr_err("In phy_update: %s\n", __func__);
			return err;
		}

		bist_incomplete = 0;
		bist_err = 0;
		/* check different data patterns per bist test*/
		for (test = 0; test < BIST_MAX_PATTERNS; test++) {
			update_bist_datapattern(mcu_id, test);
			/*
			 * Start Bist
			 */
			mcu_bist_start(mcu_id);
			/*
			 * Polling for BIST_DONE bit to be set
			 * bist_incomplete = 0 if BIST completed
			 * bist_incomplete = -1 if polling times out
			 */
			bist_incomplete = mcu_bist_completion_poll(mcu_id);

			if (!bist_incomplete)
				bist_err = mcu_bist_status(mcu_id);
			else
				mcu_bist_stop(mcu_id);

			if ((bist_incomplete == -1) || (bist_err == -1)) {
				/*
				 * Make sure you check the timeout calculation in the polling function,
				 * if you happen to be here
				 */
				ddr_err ("BIST Not Completed"
					"because of TIMEOUT or ALERT Register Set!!\n");
				return -1;
			} else if (bist_err > 0) {
				/*
				 * Bitmask for bits 0-63 in "bitmask"
				 * Bitmask for ecc bits 64-71 in "ecc_bitmask"
				 */
				if (slice < CDNPHY_NUM_SLICES - 1)
					bitmask = (0x1ULL << ((slice * 8) + bit));
				else
					ecc_bitmask = 0x1 << bit;
				bist_cmp = mcu_bist_datacmp(mcu_id, bitmask, ecc_bitmask);
				if (bist_cmp)
					break;
			} else if (bist_err == 0) {
				*delay_te = delay;
			}
		}
	}
	return 0;
}

int run_rddqdly_le_bist (struct apm_mcu *mcu, unsigned int rise, unsigned int slice,
			unsigned int bit, unsigned int physliceon, unsigned int *delay_le,
			unsigned int start_delay, unsigned int end_delay, unsigned int delay_step)
{
	unsigned int regd, bist_cmp, test, ecc_bitmask = 0;
	unsigned long long bitmask = 0;
	int bist_incomplete, bist_err, err;
	unsigned int delay;
	unsigned int mcu_id = mcu->id;
	unsigned int offset = slice * PHY_SLICE_OFFSET;

	*delay_le = start_delay;
	for (delay = start_delay; delay >= end_delay; delay -= delay_step) {
		if (rise) {
			regd = phy_read_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + offset);
			regd =  rddskw_rise_set_delay_value(bit, regd, delay);
			phy_write_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + offset, regd);
		} else {
			regd = phy_read_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + offset);
			regd = rddskw_fall_set_delay_value(bit, regd, delay);
			phy_write_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + offset, regd);
		}
		/* Run Ctrl Update to set programmed delays */
		err = phy_update(mcu_id);
		if (err) {
			ddr_err("In phy_update: %s\n", __func__);
			return err;
		}

		bist_incomplete = 0;
		bist_err = 0;
		/* check different data patterns per bist test*/
		for (test = 0; test < BIST_MAX_PATTERNS; test++) {
			update_bist_datapattern(mcu_id, test);
			/*
			 * Start Bist
			 */
			mcu_bist_start(mcu_id);
			/*
			 * Polling for BIST_DONE bit to be set
			 * bist_incomplete = 0 if BIST completed
			 * bist_incomplete = -1 if polling times out
			 */
			bist_incomplete = mcu_bist_completion_poll(mcu_id);

			if (!bist_incomplete)
				bist_err = mcu_bist_status(mcu_id);
			else
				mcu_bist_stop(mcu_id);

			if ((bist_incomplete == -1) || (bist_err == -1)) {
				/*
				 * Make sure you check the timeout calculation in the polling function,
				 * if you happen to be here
				 */
				ddr_err ("BIST Not Completed"
					"because of TIMEOUT or ALERT Register Set!!\n");
				return -1;
			} else if (bist_err > 0) {
				/*
				 * Bitmask for bits 0-63 in "bitmask"
				 * Bitmask for ecc bits 64-71 in "ecc_bitmask"
				 */
				if (slice < CDNPHY_NUM_SLICES - 1)
					bitmask = (0x1ULL << ((slice * 8) + bit));
				else
					ecc_bitmask = 0x1 << bit;
				bist_cmp = mcu_bist_datacmp(mcu_id, bitmask, ecc_bitmask);
				if (bist_cmp)
					break;
			} else if (bist_err == 0) {
				*delay_le = delay;
			}
		}
	}
	return 0;
}

int run_rddqdly_te_bist (struct apm_mcu *mcu, unsigned int rise, unsigned int slice,
			unsigned int bit, unsigned int physliceon, unsigned int *delay_te,
			unsigned int start_delay, unsigned int end_delay, unsigned int delay_step)
{
	unsigned int regd, bist_cmp, test, ecc_bitmask = 0;
	unsigned long long bitmask = 0;
	int bist_incomplete, bist_err, err;
	unsigned int delay;
	unsigned int mcu_id = mcu->id;
	unsigned int offset = slice * PHY_SLICE_OFFSET;

	*delay_te = start_delay;
	for (delay = start_delay; delay <= end_delay; delay += delay_step) {
		if (rise) {
			regd = phy_read_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + offset);
			regd = rddskw_rise_set_delay_value(bit, regd, delay);
			phy_write_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + offset, regd);
		} else {
			regd = phy_read_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + offset);
			regd = rddskw_fall_set_delay_value(bit, regd, delay);
			phy_write_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + offset, regd);
		}
		/* Run Ctrl Update to set programmed delays */
		err = phy_update(mcu_id);
		if (err) {
			ddr_err("In phy_update: %s\n", __func__);
			return err;
		}

		bist_incomplete = 0;
		bist_err = 0;
		/* check different data patterns per bist test*/
		for (test = 0; test < BIST_MAX_PATTERNS; test++) {
			update_bist_datapattern(mcu_id, test);
			/*
			 * Start Bist
			 */
			mcu_bist_start(mcu_id);
			/*
			 * Polling for BIST_DONE bit to be set
			 * bist_incomplete = 0 if BIST completed
			 * bist_incomplete = -1 if polling times out
			 */
			bist_incomplete = mcu_bist_completion_poll(mcu_id);

			if (!bist_incomplete)
				bist_err = mcu_bist_status(mcu_id);
			else
				mcu_bist_stop(mcu_id);

			if ((bist_incomplete == -1) || (bist_err == -1)) {
				/*
				 * Make sure you check the timeout calculation in the polling function,
				 * if you happen to be here
				 */
				ddr_err ("BIST Not Completed"
					"because of TIMEOUT or ALERT Register Set!!\n");
				return -1;
			} else if (bist_err > 0) {
				/*
				 * Bitmask for bits 0-63 in "bitmask"
				 * Bitmask for ecc bits 64-71 in "ecc_bitmask"
				 */
				if (slice < CDNPHY_NUM_SLICES - 1)
					bitmask = (0x1ULL << ((slice * 8) + bit));
				else
					ecc_bitmask = 0x1 << bit;
				bist_cmp = mcu_bist_datacmp(mcu_id, bitmask, ecc_bitmask);
				if (bist_cmp)
					break;
			} else if (bist_err == 0) {
				*delay_te = delay;
			}
		}
	}
	return 0;
}

/******************************************************************************
 *     Phy Write calibration functions
 *****************************************************************************/

int mcu_bist_phy_wrcal(struct apm_mcu *mcu, int unsigned rank)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	unsigned int err = 0;
	unsigned int iia, jjb;
	unsigned int regd;
	unsigned int nfc[9];
	unsigned int x4nfc[9];
	unsigned int ldone = 0;
	int bist_incomplete, bist_err;
	unsigned int physliceon;
	unsigned int mcu_id = mcu->id;
	unsigned int bist2t = udp->ud_en2tmode;
	unsigned int offset;
	struct apm_mcu_bist_config b_config;

	ddr_verbose("\nMCU-PHY[%d] rank(%d): WR-CAL:\n", mcu_id, rank);
	physliceon = PHY_SLICE_EN_MASK;
	mcu_bist_setup(mcu_id, rank, bist2t);

	b_config.itercnt =		1;
	b_config.bistcrcen =		MCU_BIST_CRC_EN;
	b_config.bistcmprdsbl =		MCU_BIST_CMPR_DSBL;
	b_config.stoponmiscompare =	MCU_BIST_STOP_ON_MISCOMPARE;
	b_config.stoponalert =		MCU_BIST_STOP_ON_ALERT;
	b_config.readdbien =		MCU_BIST_READ_DBI_EN;
	b_config.writedbien =		MCU_BIST_WRITE_DBI_EN;
	b_config.mapcidtocs =		MCU_BIST_MAP_CID_TO_CS;
	b_config.rdloopcnt =		1;
	b_config.wrloopcnt =		1;

	mcu_bist_config(mcu_id, bist2t, &b_config);

	/* Set slices to zero full cycles */
	for( jjb = 0 ; jjb < CDNPHY_NUM_SLICES; jjb++) {
		nfc[jjb] = 0;
		x4nfc[jjb] = 0;
	}

	/*
	 * Set the data pattern and update
	 */
	update_bist_datapattern(mcu_id, WR_CAL_PATTERN_SELECTOR);

	/*
	 * Can loop 0-7 full cycles (Phy csr limit)
	 */
	for (iia = 0 ; iia < MAX_WRITE_PATH_LAT_ADD_CYCLES ; iia++) {
		ddr_verbose("WrCal Bist:  start loop(%d)\n",iia);
		for (jjb = 0 ; jjb < CDNPHY_NUM_SLICES ; jjb++) {
			offset = jjb * PHY_SLICE_OFFSET;

			if (!((physliceon >> (jjb * 2)) && 0x1))
				continue;
			regd = phy_read_reg(mcu_id, (MCU_PHY_WRITE_PATH_LAT_ADD_0_ADDR + offset));
			regd = FIELD_MCU_PHY_WRITE_PATH_LAT_ADD_0_SET(regd,  nfc[jjb]);
			phy_write_reg(mcu_id, (MCU_PHY_WRITE_PATH_LAT_ADD_0_ADDR + offset), regd);
			if (mcu->ddr_info.device_type != REGV_MEMORY_DEVICE_WIDTH_X4)
				continue;
			regd = phy_read_reg(mcu_id, (MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_ADDR + offset));
			regd = FIELD_MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_SET(regd, x4nfc[jjb]);
			phy_write_reg(mcu_id, (MCU_PHY_X4_WRITE_PATH_LAT_ADD_0_ADDR + offset), regd);
		}

		/* Run Ctrl Update to set programmed delays */
		err = phy_update(mcu_id);
		if (err) {
			ddr_err("In phy_update: %s\n", __func__);
			return err;
		}

		bist_incomplete = 0;
		bist_err = 0;

		/*
		 * Start Bist
		 */
		mcu_bist_start(mcu_id);
		/*
		 * Polling for BIST_DONE bit to be set
		 * bist_incomplete = 0 if BIST completed
		 * bist_incomplete = -1 if polling times out
		 */
		bist_incomplete = mcu_bist_completion_poll(mcu_id);

		if (!bist_incomplete)
			bist_err = mcu_bist_status(mcu_id);
		else
			mcu_bist_stop(mcu_id);

                mcu_bist_datacmp_reg_dump(mcu_id, 0);
                mcu_bist_datacmp_reg_dump(mcu_id, 1);

		if ((bist_incomplete == -1) || (bist_err == -1)) {

			/*
			 * Make sure you check the timeout calculation in the polling function,
			 * if you happen to be here
			 */
			ddr_err ("BIST Not Completed"
				"because of TIMEOUT or ALERT Register Set!!\n");
			/*
			 * breaking this loop will keep ldone = 0,
			 * which in turn will increment err and return error
			 */
			break;
		} else if (bist_err > 0) {
			for (jjb = 0 ; jjb < CDNPHY_NUM_SLICES; jjb++) {
				if (!((physliceon >> (jjb * 2)) && 0x1))
					continue;
				/*Do x4 case per nibble */
				if (mcu->ddr_info.device_type == REGV_MEMORY_DEVICE_WIDTH_X4) {
					/* Nibble 0 */
					if ( ((bist_err >> (jjb * 2)) & 0x1) == 0) {
						ldone |= 0x1 << (jjb * 2);
						ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2), nfc[jjb]);
					} else {
						ldone &= ~(0x1 << (jjb * 2));
						nfc[jjb] += 1;
						ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2), nfc[jjb]);
					}
					/* Nibble 1 */
					if ( ((bist_err >> (jjb * 2 + 1)) & 0x1) == 0) {
						ldone |= 0x1 << (jjb * 2 + 1);
						ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2 + 1), x4nfc[jjb]);
					} else {
						ldone &= ~(0x1 << (jjb * 2 + 1));
						x4nfc[jjb] += 1;
						ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2 + 1), x4nfc[jjb]);
					}
				} else {
					/*  x8 dimm case */
					if ( ((bist_err >> (jjb * 2)) & 0x3) == 0) {
						ldone |= 0x3 << (jjb * 2);
						ddr_verbose("[L%1d nfc=%1d] ", jjb, nfc[jjb]);
					} else {
						ldone &= ~(0x3 << (jjb * 2));
						nfc[jjb] += 1;
						ddr_verbose("[L%1d nfc=%1d] ", jjb, nfc[jjb]);
					}
				}

			}
			ddr_verbose("\n\tldone=0x%05x\tcmp=0x%05x\n\n", ldone, bist_err);
			/*
			 * TODO: WR_CAL DEBUG Matrix print
			 */
		} else if (bist_err == 0){
			ddr_verbose("WrCal BIST response good! \n");
			ldone = 0x3FFFF;
			break;
		}
	}
	ddr_verbose("MCU-PHY[%d] rank=%0d WrCal Bist: ldone=0x%05x \n",
		    mcu_id, rank, ldone);
	for (jjb = 0 ; jjb < CDNPHY_NUM_SLICES ; jjb++) {
		if (!((physliceon >> (jjb * 2)) && 0x1))
			continue;
		ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2), nfc[jjb]);
		if (mcu->ddr_info.device_type == REGV_MEMORY_DEVICE_WIDTH_X4)
			/* Nibble 1 */
			ddr_verbose("[L%1d nfc=%1d] ", (jjb * 2 + 1), x4nfc[jjb]);
	}
	if (ldone != 0x3FFFF) {
		ddr_err("\nMCU-PHY[%d] WrCal BIST failed!\n", mcu_id);
		err++;
	}
	return (err) ? (0x1 << rank) : 0;
}/* mcu_bist_phy_wrcal */

/******************************************************************************
 *	 Phy Write Deskew functions (post train)
 *****************************************************************************/
int mcu_bist_phy_wrdeskew(struct apm_mcu *mcu, unsigned int rank)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	int err = 0, bist_incomplete, bist_status;
	unsigned int slice, bit, start_delay;
	unsigned int regd;
	unsigned int physliceon;
	unsigned int wrdqdly[9][8] = {{PHY_CLK_WRDQx_SLAVE_DELAY}};
	unsigned int delay_le;
	unsigned int delay_te;
	unsigned int mcu_id = mcu->id;
	unsigned int bist2t = udp->ud_en2tmode;
	physliceon = PHY_SLICE_EN_MASK;

	ddr_verbose("\nMCU-PHY[0x%02X] rank (%d) Wr Bit Deskew:\nPhy Slice En = 0x%05X\n",
			mcu_id, rank, physliceon);

	mcu_bist_setup(mcu_id, rank, bist2t);

	ddr_verbose("MCU-PHY[%d] WR-DSK : Look for Leading/Trailing-Edge\n", mcu_id);
	start_delay = PHY_CLK_WRDQx_SLAVE_DELAY;
	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		if (!((physliceon >> (slice * 2)) && 0x1))
			continue;
		for (bit = 0; bit < 8; bit++) {
			/*
			 * look for LeadingEdge
			 */
			err = run_wrdqdly_le_bist (mcu, slice, bit, physliceon,
					&delay_le, start_delay, PHY_CLK_WRDQx_SLAVE_DELAY_MIN_LIMIT, STEP_LARGE);
			if ((!err) && (delay_le != start_delay)) {
				err = run_wrdqdly_le_bist (mcu, slice, bit, physliceon,
						&delay_le, delay_le,
						PHY_CLK_WRDQx_SLAVE_DELAY_MIN_LIMIT, STEP_SMALL);
			}

			ddr_verbose("\tLn[%d] - Bit[%d] LE:0x%X", slice, bit, delay_le);

			/* look for TrailingEdge */
			err = run_wrdqdly_te_bist (mcu, slice, bit, physliceon,
					&delay_te, start_delay, PHY_CLK_WRDQx_SLAVE_DELAY_MAX_LIMIT, STEP_LARGE);
			if ((!err) && (delay_le != start_delay)) {
				err = run_wrdqdly_te_bist (mcu, slice, bit, physliceon,
						&delay_te, delay_te,
						PHY_CLK_WRDQx_SLAVE_DELAY_MAX_LIMIT, STEP_SMALL);
			}

			ddr_verbose("\tLn[%d] - Bit[%d] TE:0x%X\n", slice, bit, delay_te);

			/* restore delay to old value for now */
			if (delay_le < delay_te) {
				wrdqdly[slice][bit] = (delay_le + delay_te)/2;
				ddr_verbose(" Window:0x%X NewDelay:0x%x\n",
						delay_te - delay_le, wrdqdly[slice][bit]);
			} else {
				wrdqdly[slice][bit] = PHY_CLK_WRDQx_SLAVE_DELAY;
				ddr_verbose("\nLn[%d] - Bit[%d] No Window: Using default\n NewDelay:0x%x\n",
						slice, bit, wrdqdly[slice][bit]);
			}
			if (wrdqdly[slice][bit] > PHY_CLK_WRDQx_SLAVE_DELAY) {
				if ((wrdqdly[slice][bit] - PHY_CLK_WRDQx_SLAVE_DELAY) > 0x100) {
					wrdqdly[slice][bit] = PHY_CLK_WRDQx_SLAVE_DELAY;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big change: Using default\n"
							"NewDelay:0x%x\n", slice, bit, wrdqdly[slice][bit]);
				}
			}
			if (wrdqdly[slice][bit] < PHY_CLK_WRDQx_SLAVE_DELAY) {
				if ((PHY_CLK_WRDQx_SLAVE_DELAY - wrdqdly[slice][bit]) > 0x100) {
					wrdqdly[slice][bit] = PHY_CLK_WRDQx_SLAVE_DELAY;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big change: Using default\n"
							" NewDelay:0x%x\n", slice, bit, wrdqdly[slice][bit]);
				}
			}
			regd = phy_read_reg(mcu_id, wrdskw_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET));
			regd = wrdskw_set_delay_value(bit, regd, wrdqdly[slice][bit] & 0x7FF);
			phy_write_reg(mcu_id, wrdskw_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET), regd);

			/* Run Ctrl Update to set programmed delays */
			err = phy_update(mcu_id);
			if (err) {
				ddr_err("In phy_update: %s\n", __func__);
				return err;
			}

		}
		ddr_verbose(" \n");
	}

	/*
	 * Run BIST Once
	 */
	mcu_bist_start(mcu_id);
	bist_incomplete = mcu_bist_completion_poll(mcu_id);
	bist_status = mcu_bist_status(mcu_id);

	if (bist_incomplete) {
		mcu_bist_stop(mcu_id);
		ddr_err("MCU-PHY[%d] WR-DSK: [0x%05X]\n", mcu_id, bist_status);
		return bist_incomplete;
	} else if (bist_status) {
		ddr_err("MCU-PHY[%d] WR-DSK: [0x%05X]\n", mcu_id, bist_status);
		err = -1;
	} else {
		ddr_verbose("MCU-PHY[%d] WR-DSK: Done!\n", mcu_id);
	}

	return err;

}/* mcu_bist_phy_wrdeskew */

/******************************************************************************
 *	 Phy Read Deskew functions (post train)
 *****************************************************************************/
int mcu_bist_phy_rddeskew(struct apm_mcu *mcu, unsigned int rank)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	int err = 0, bist_incomplete, bist_status;
	unsigned int slice, bit, start_delay;
	unsigned int regd;
	unsigned int physliceon;
	unsigned int rddqdly[9][8] = {{PHY_CLK_WRDQx_SLAVE_DELAY}};
	unsigned int delay_le;
	unsigned int delay_te;
	unsigned int mcu_id = mcu->id;
	unsigned int bist2t = udp->ud_en2tmode;

	/* Get initial phy values */
	mcu_phy_peek_rdeye_res_rnk(mcu_id, rank, &deskewd);

	physliceon = PHY_SLICE_EN_MASK;

	ddr_verbose("MCU-PHY[%d] rank (%d) Rd-RISE Bit Deskew\nPhy Slice En = 0x%05x\n",
			mcu_id, rank, physliceon);

	mcu_bist_setup(mcu_id, rank, bist2t);

	ddr_verbose("MCU-PHY[%d] RD-DSK : Look for Leading/Trailing-Edge\n", mcu_id);
	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		if (!((physliceon >> (slice*2)) && 0x1))
			continue;

		for (bit = 0; bit < 8; bit++) {
			start_delay = deskewd.sliced[slice].phy_rddqs_dqZ_rise_slave_delay[bit];

			/* look for LeadingEdge */
			err = run_rddqdly_le_bist(mcu, 1, slice, bit, physliceon, &delay_le,
					start_delay, PHY_CLK_RDDQx_SLAVE_DELAY_MIN_LIMIT, STEP_LARGE);
		        if ((!err) && (delay_le != start_delay)) {
				err = run_rddqdly_le_bist(mcu, 1, slice, bit, physliceon, &delay_le,
						delay_le,
						PHY_CLK_RDDQx_SLAVE_DELAY_MIN_LIMIT, STEP_SMALL);
			}

			ddr_verbose("\tLn[%d] - Bit[%d] LE:0x%X", slice, bit, delay_le);

			/* look for TrailingEdge */
			err = run_rddqdly_te_bist(mcu, 1, slice, bit, physliceon, &delay_te,
					start_delay, PHY_CLK_RDDQx_SLAVE_DELAY_MAX_LIMIT, STEP_LARGE);
		        if ((!err) && (delay_te != start_delay)) {
				err = run_rddqdly_te_bist(mcu, 1, slice, bit, physliceon, &delay_te,
						delay_te,
						PHY_CLK_RDDQx_SLAVE_DELAY_MAX_LIMIT, STEP_SMALL);
			}

			ddr_verbose("\tLn[%d] - Bit[%d] TE:0x%X\n", slice, bit, delay_te);

			if (delay_le < delay_te) {
				rddqdly[slice][bit] = (delay_le + delay_te)/2;
				ddr_verbose("Window:0x%X NewDelay:0x%x [HW:0x%x]\n",
						delay_te - delay_le, rddqdly[slice][bit], start_delay);
			} else {
				rddqdly[slice][bit] = start_delay;
				ddr_verbose("\nLn[%d] - Bit[%d] No Window: Using default NewDelay:0x%x [HW:0x%x]\n",
					    slice, bit, rddqdly[slice][bit], start_delay);
			}
			if (rddqdly[slice][bit] > start_delay) {
				if ((rddqdly[slice][bit] - start_delay) > 0xC0) {
					rddqdly[slice][bit] = start_delay;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big: Using default NewDelay:0x%x [HW:0x%x]\n",
						    slice, bit, rddqdly[slice][bit], start_delay);
				}
			}
			if (rddqdly[slice][bit] < start_delay) {
				if ((start_delay - rddqdly[slice][bit]) > 0xC0) {
					rddqdly[slice][bit] = start_delay;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big: Using default NewDelay:0x%x [HW:0x%x]\n",
						    slice, bit, rddqdly[slice][bit], start_delay);
				}
			}
			regd = phy_read_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET));
			regd = rddskw_rise_set_delay_value(bit, regd, rddqdly[slice][bit] & 0x3FF);
			phy_write_reg(mcu_id, rddskw_rise_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET), regd);

			/* Run Ctrl Update to set programmed delays */
			err = phy_update(mcu_id);
			if (err) {
				ddr_err("In phy_update: %s\n", __func__);
				return err;
			}

		}
		ddr_verbose(" \n");
	}
	/*
	 * Run BIST Once
	 */
	mcu_bist_start(mcu_id);
	bist_incomplete = mcu_bist_completion_poll(mcu_id);
	bist_status = mcu_bist_status(mcu_id);

	if (bist_incomplete) {
		mcu_bist_stop(mcu_id);
		ddr_err("MCU-PHY[%d] RD-RISE-DSK: [0x%05X]\n", mcu_id, bist_status);
		return bist_incomplete;
	} else if (bist_status) {
		ddr_err("MCU-PHY[%d] RD-RISE-DSK: [0x%05X]\n", mcu_id, bist_status);
		err = -1;
	} else {
		ddr_verbose("MCU-PHY[%d] RD_RISE-DSK: Done!\n", mcu_id);
	}

	ddr_verbose("\n\nMCU-PHY[0x%02X] rank (%d) Rd-FALL Bit Deskew:\n Phy Slice En = 0x%05x\n",
		    mcu_id, rank, physliceon);

	delay_le = 0;
	delay_te = 0;

	ddr_verbose("MCU-PHY[%d] RD-DSK : Look for Leading/Trailing-Edge\n", mcu_id);

	for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
		if (!((physliceon >> (slice*2)) && 0x1))
			continue;
		for (bit= 0; bit < 8; bit++) {
			/* look for LeadingEdge */
			start_delay = deskewd.sliced[slice].phy_rddqs_dqZ_fall_slave_delay[bit];

			err = run_rddqdly_le_bist(mcu, 0, slice, bit, physliceon, &delay_le,
					start_delay, PHY_CLK_RDDQx_SLAVE_DELAY_MIN_LIMIT, STEP_LARGE);
		        if ((!err) && (delay_le != start_delay)) {
				err = run_rddqdly_le_bist(mcu, 0, slice, bit, physliceon, &delay_le,
						delay_le,
						PHY_CLK_RDDQx_SLAVE_DELAY_MIN_LIMIT, STEP_SMALL);
			}
			ddr_verbose("\tLn[%d] - Bit[%d] LE:0x%X", slice, bit, delay_le);

			/* look for TrailingEdge */
			err = run_rddqdly_te_bist(mcu, 0, slice, bit, physliceon, &delay_te,
					start_delay, PHY_CLK_RDDQx_SLAVE_DELAY_MAX_LIMIT, STEP_LARGE);
		        if ((!err) && (delay_te != start_delay)) {
				err = run_rddqdly_te_bist(mcu, 0, slice, bit, physliceon, &delay_te,
						delay_te,
						PHY_CLK_RDDQx_SLAVE_DELAY_MAX_LIMIT, STEP_SMALL);
			}

			ddr_verbose("\tLn[%d] - Bit[%d] TE:0x%X\n", slice, bit, delay_te);

			if (delay_le < delay_te) {
				rddqdly[slice][bit] = (delay_le + delay_te)/2;
				ddr_verbose("Window:0x%X NewDelay:0x%x [HW:0x%x]\n",
					delay_te - delay_le, rddqdly[slice][bit], start_delay);
			} else {
				rddqdly[slice][bit] = start_delay;
				ddr_verbose("\nLn[%d] - Bit[%d] No Window: Using default NewDelay:0x%x [HW:0x%x]\n",
					    slice, bit, rddqdly[slice][bit], start_delay);
			}
			if (rddqdly[slice][bit] > start_delay) {
				if ((rddqdly[slice][bit] - start_delay) > 0xC0) {
					rddqdly[slice][bit] = start_delay;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big: Using default NewDelay:0x%x [HW:0x%x]\n",
						    slice, bit, rddqdly[slice][bit], start_delay);
				}
			}
			if (rddqdly[slice][bit] < start_delay) {
				if ((start_delay - rddqdly[slice][bit]) > 0xC0) {
					rddqdly[slice][bit] = start_delay;
					ddr_verbose("\nLn[%d] - Bit[%d] Too Big: Using default NewDelay:0x%x [HW:0x%x]\n",
						slice, bit, rddqdly[slice][bit], start_delay);
				}
			}
			regd = phy_read_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET));
			regd = rddskw_fall_set_delay_value(bit, regd, rddqdly[slice][bit] & 0x3FF);
			phy_write_reg(mcu_id, rddskw_fall_get_reg_offset(bit) + (slice * PHY_SLICE_OFFSET), regd);

			/* Run Ctrl Update to set programmed delays */
			err = phy_update(mcu_id);
			if (err) {
				ddr_err("In phy_update: %s\n", __func__);
				return err;
			}

		}
		ddr_verbose(" \n");
	}
	/*
	 * Run BIST Once
	 */
	mcu_bist_start(mcu_id);
	bist_incomplete = mcu_bist_completion_poll(mcu_id);
	bist_status = mcu_bist_status(mcu_id);

	if (bist_incomplete) {
		mcu_bist_stop(mcu_id);
		ddr_err("MCU-PHY[%d] RD-FALL-DSK: [0x%05X]\n", mcu_id, bist_status);
		return bist_incomplete;
	} else if (bist_status) {
		ddr_err("MCU-PHY[%d] RD-FALL-DSK: [0x%05X]\n", mcu_id, bist_status);
		err = -1;
	} else {
		ddr_verbose("MCU-PHY[%d] RD_FALL-DSK: Done!\n", mcu_id);
	}

	return (err) ? (0x1 << rank) : 0;
}/* mcu_bist_phy_rddeskew */

int bist_test_vref_training(struct apm_mcu *mcu, unsigned int pattern, unsigned int *res)
{
	unsigned int bist_incomplete;
	unsigned int mcu_id = mcu->id;

	/* Set BIST data pattern */
	update_bist_datapattern(mcu_id, pattern);

	/* Start BIST */
	mcu_bist_start(mcu_id);

	/* Poll BIST Done */
	bist_incomplete = mcu_bist_completion_poll(mcu_id);
	if (bist_incomplete) {
		/* BIST runout, Stopping forcefully  */
		mcu_bist_stop(mcu_id);
		return -1;
	} else {
		/* BIST passes, stop  */
		mcu_bist_stop(mcu_id);
	}
	/* Check BIST error */
	*res = mcu_bist_err_status(mcu_id);
	return 0;
}

int bist_test_pda_vref_training(struct apm_mcu *mcu, unsigned int pattern,
				 unsigned int *res, unsigned int comp)
{
	unsigned int bist_incomplete;
	unsigned int mcu_id = mcu->id;

	/* Set BIST data pattern */
	update_bist_datapattern(mcu_id, pattern);

	/* Start BIST */
	mcu_bist_start(mcu_id);

	/* Poll BIST Done */
	bist_incomplete = mcu_bist_completion_poll(mcu_id);
	if (bist_incomplete) {
		/* BIST runout, Stopping forcefully */
		mcu_bist_stop(mcu_id);
		return -1;
	} else {
		/* BIST passes, stop  */
		mcu_bist_stop(mcu_id);
	}
	/* Check BIST error */
	*res = mcu_bist_byte_status_line0(mcu_id, mcu->ddr_info.device_type, comp);
	if (*res) {
		ddr_verbose("BIST status for Byte %d\n", comp);
		mcu_bist_datacmp_reg_dump(mcu_id, 0);
	}
	return 0;
}
/******************************************************************************
 *     DIMM Vref Training functions (post train)
 *****************************************************************************/
int mcu_dram_vref_training(struct apm_mcu *mcu, unsigned int rank)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	int err = 0, fail = 0;
	unsigned int rank_sel = 1 << rank;
	unsigned int mcu_id = mcu->id;
	unsigned int sweep, done, comp, max_devices;
	unsigned int t_ck_ps;
	unsigned int start_vref, new_vref, low_vref, high_vref;
	unsigned int final_vref = 0, final_vref_comp = 0;
	unsigned int __attribute__ ((unused)) final_vref_pda[18] = {0};
	unsigned int bist_result = 0, pattern;
	struct apm_mcu_bist_address_setup b_addr_set;
	struct apm_mcu_bist_config b_config;

	if (!udp->ud_dram_vref_train_en)
		return err;
	ddr_verbose("\nMCU[%d] Starting DRAM VREFDQ Training for rank %d\n",
			mcu_id, rank);

	/* Step1: Setup BIST for DIMM Vref Training */
	/* Targetting different bank groups to insert breack in write burst */
	b_addr_set.rank0 =		rank;
	b_addr_set.rank1 =		rank;
	b_addr_set.bank =		MCU_BIST_BANK;
	b_addr_set.bankgroup0 =		MCU_BIST_BANKGROUP0;
	b_addr_set.bankgroup1 =		MCU_BIST_BANKGROUP1;
	b_addr_set.cid0 =		MCU_BIST_CID0;
	b_addr_set.cid1 =		MCU_BIST_CID1;
	b_addr_set.row =		MCU_BIST_ROW;
	b_addr_set.col =		MCU_BIST_COL;
	mcu_bist_address_setup(mcu_id, &b_addr_set);

	/* In-count=1: stop on error, ex-count=high: regressive test */
	b_config.itercnt =		10;
	b_config.bistcrcen =		MCU_BIST_CRC_EN;
	b_config.bistcmprdsbl =		MCU_BIST_CMPR_DSBL;
	b_config.stoponmiscompare =	MCU_BIST_STOP_ON_MISCOMPARE;
	b_config.stoponalert =		MCU_BIST_STOP_ON_ALERT;
	b_config.readdbien =		MCU_BIST_READ_DBI_EN;
	b_config.writedbien =		MCU_BIST_WRITE_DBI_EN;
	b_config.mapcidtocs =		MCU_BIST_MAP_CID_TO_CS;
	b_config.rdloopcnt =		4;
	b_config.wrloopcnt =		1;
	mcu_bist_config(mcu_id, 0, &b_config);

	/* MRS6: Calculate t_ck_ps */
	t_ck_ps = mcu->ddr_info.t_ck_ps;

	/* Start VREF-DQ training for all components */
	/* Start from center of vref, and then search low and high bound */
	/* Find a working value from max to start shmoo */
	new_vref =  DRAM_VREFDQ_MAX_VALUE;
	sweep = 0;
findstart:
	/* MRS6: Set New VREF-DQ */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_TRAIN_MODE,
			udp->ud_dram_vref_range, new_vref, t_ck_ps, 0);
	/* 1us delay for vref change to be stable */
	DELAY(1);
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_NORMAL_MODE,
			udp->ud_dram_vref_range, new_vref, t_ck_ps, 0);

	/* BIST test with default pattern */

	for (pattern = 0; pattern < BIST_MAX_PATTERNS; pattern++) {
		err = bist_test_vref_training(mcu, pattern, &bist_result);
		if (err || bist_result)
			break;
	}
	if (err < 0) {
		ddr_err("MCU[%d] Fatal Error: BIST poll fail.\n", mcu_id);
		return err;
	}
	if (bist_result) {
		/* Try next value */
		sweep = 0;
		if (new_vref < DRAM_VREFDQ_WINDOW_STEP) {
			/* No Window, use default value: passed for WR-CAL */
			ddr_verbose("\tUsing Default start value\n");
			new_vref = udp->ud_dram_vref_value;
		} else {
			new_vref -= DRAM_VREFDQ_WINDOW_STEP;
			goto findstart;
		}
	} else {
		sweep++;
		if (sweep < 2) {
			/* Search consiqutive two pass */
			new_vref -= DRAM_VREFDQ_WINDOW_STEP;
			goto findstart;
		} else if (new_vref < DRAM_VREFDQ_WINDOW_STEP) {
			/* No Window, use default value: passed for WR-CAL */
			ddr_verbose("\tUsing Default start value\n");
			new_vref = udp->ud_dram_vref_value;
		}
	}
	start_vref = new_vref;
	ddr_verbose("\t[Starting VREF range=%d & Value=0x%02X]\n",
			udp->ud_dram_vref_range + 1, start_vref);
	/* sweep=0: Search low bound, 1: Search high bound, 2: Final test */
	sweep = 0;
	done = 0;

	high_vref = low_vref = 0;
dimmvrefloop:
	/* MRS6: Set New VREF-DQ */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_TRAIN_MODE,
			udp->ud_dram_vref_range, new_vref, t_ck_ps, 0);
	/* 1us delay for vref change to be stable */
	DELAY(1);
	/* MRS6: Exiting VREF-DQ training mode */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_NORMAL_MODE,
			udp->ud_dram_vref_range, new_vref, t_ck_ps, 0);

	/* BIST test */
	for (pattern=0; pattern < BIST_MAX_PATTERNS; pattern++) {
		err = bist_test_vref_training(mcu, pattern, &bist_result);
		if (err < 0) {
			ddr_err("MCU[%d] BIST poll fail\n", mcu_id);
			return err;
		}
		/* Sweep low/high range for optimum vrefdq calculaiton */
		if (bist_result) {
			ddr_verbose("MCU[%d] VREFDQ training: sweep=%d Vref=0x%02X\n",
					mcu_id, sweep, new_vref);
			if (sweep == 0) {
				/* Minumum vref reached; store and look for max */
				low_vref = new_vref;
				new_vref = start_vref;
				sweep++;
				break;
			} else if (sweep == 1) {
				/* Maximum vref reached; store and calculate optimum */
				high_vref = new_vref;
				new_vref = (low_vref + low_vref + high_vref) / 3;
				sweep++;
				break;
			} else if (sweep == 2) {
				/* New optimum failed, set default and retest */
				ddr_err("MCU[%d]: Vref Training not stable\n", mcu_id);
				new_vref = udp->ud_dram_vref_value;
				sweep++;
				err = -1;
				break;
			} else {
				/* Default/Optimum value failed, report error */
				ddr_err("MCU[%d]: Vref Training not stable\n", mcu_id);
				final_vref = new_vref;
				done++;
				err = -1;
				break;
			}
		} else if (pattern == BIST_MAX_PATTERNS - 1) {
			if (sweep == 0) {
				if (new_vref < (DRAM_VREFDQ_MIN_VALUE + DRAM_VREFDQ_TRAIN_STEP_ALL)) {
					/* Reached min */
					low_vref = new_vref;
					ddr_verbose("MCU[%d] VREFDQ training: sweep=%d Vref=0x%02X\n",
							mcu_id, sweep, new_vref);
					new_vref = start_vref;
					sweep++;
				} else {
					/* Continue searching min-vref */
					new_vref -= DRAM_VREFDQ_TRAIN_STEP_ALL;
				}
				break;
			} else if (sweep == 1) {
				if (new_vref > (DRAM_VREFDQ_MAX_VALUE - DRAM_VREFDQ_TRAIN_STEP_ALL)) {
					/* Reached max, calculate optimum */
					high_vref = new_vref;
					ddr_verbose("MCU[%d] VREFDQ training: sweep=%d Vref=0x%02X\n",
							mcu_id, sweep, new_vref);
					new_vref = (low_vref + low_vref + high_vref) / 3;
					sweep++;
				} else {
					/* Continue searching max-vref */
					new_vref += DRAM_VREFDQ_TRAIN_STEP_ALL;
				}
				break;
			} else if (sweep >= 2) {
				/* New Optimum value passed; Done */
				final_vref = new_vref;
				sweep++;
				done++;
				break;
			}
		}
	}
	if (!done)
		goto dimmvrefloop;
	ddr_verbose("MCU[%d] VREFDQ training: Final-Vref=0x%02X\n"
			"\t[Low=0x%02X High0x%02X Optimum=0x%02X]\n",
			mcu_id, final_vref, low_vref, high_vref, new_vref);

	/*Exit VREF-DQ training mode */
	if (new_vref != final_vref) {
		/*
		 * TODO: Something wrong happened?
		 * Can not exit vref training with differnet vref value
		 * updade final_vref as new_vref
		 */
		err = -1;
		final_vref = new_vref;
	}

	/* Check if VREFDQ Finetune per component enabled */
	if (!udp->ud_dram_vref_finetune_en) {
		if (!err)
			ddr_verbose("MCU[%d] VREFDQ Training PASS!\n\n", mcu_id);
		else
			ddr_err("MCU[%d] VREFDQ Training FAIL!\n\n", mcu_id);
		return err;
	}

	/* Fine tune VREF-DQ per component */
	max_devices = (mcu->ddr_info.device_type == REGV_MEMORY_DEVICE_WIDTH_X4) ? 18 : 9;

	/* Enter VREF-DQ training mode for target component */
	comp = 0;

comploop:
	ddr_verbose("\nSearching Optimum VREFDQ for component: %d\n", comp);

	/* Start from earlier optimum vref, and then search low and high bound */
	new_vref = final_vref;

	/* sweep=0: Search low bound, 1: Search high bound, 2: Final test */
	sweep = 0;
	done = 0;

fineloop:
	/* MRS3: Enter PDA mode  for target component*/
	mr3_pda_access(mcu_id, rank_sel, PDA_ENTRY, comp, mcu->ddr_info.t_ck_ps,
			udp->ud_refresh_granularity);

	/* MRS6: Set New VREF-DQ for target component */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_TRAIN_MODE, udp->ud_dram_vref_range,
			new_vref, t_ck_ps, comp);
	/* 1us delay for vref change to be stable */
	DELAY(1);
	/* MRS6: Exiting VREF-DQ training mode */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_NORMAL_MODE,
			udp->ud_dram_vref_range, new_vref, t_ck_ps, comp);
	/* Exit PDA mode */
	mr3_pda_access(mcu_id, rank_sel, PDA_EXIT, comp, mcu->ddr_info.t_ck_ps,
			udp->ud_refresh_granularity);

	/* BIST test */
	for (pattern=0; pattern < BIST_MAX_PATTERNS; pattern++) {
		err = bist_test_pda_vref_training(mcu, pattern, &bist_result, comp);
		if (err < 0) {
			ddr_err("MCU[%d] BIST poll fail.\n", mcu_id);
			return err;
		}
		/* Sweep low/high range for optimum vrefdq calculaiton */
		if (bist_result) {
			ddr_verbose("MCU[%d] COMP[%d] VREFDQ training: sweep=%d Vref=0x%02X\n",
				mcu_id, comp, sweep, new_vref);
			if (sweep == 0) {
				/* Minumum vref reached; store and look for max */
				low_vref = new_vref;
				new_vref = final_vref;
				sweep++;
				break;
			} else if (sweep == 1) {
				/* Maximum vref reached; store and calculate optimum */
				high_vref = new_vref;
				new_vref = (low_vref + high_vref) / 2;
				sweep++;
				break;
			} else if (sweep == 2) {
				/* New optimum failed, set default and retest */
				new_vref = final_vref;
				sweep++;
				break;
			} else {
				/* Default/Optimum value failed, report error */
				ddr_err("MCU[%d] COMP[%d] VREFDQ Training not stable\n",
					mcu_id, comp);
				final_vref_comp = new_vref;
				done++;
				err = -1;
				sweep++;
				break;
			}
		} else {
			if (sweep == 0) {
				if (new_vref < (DRAM_VREFDQ_MIN_VALUE + DRAM_VREFDQ_TRAIN_STEP_PDA)) {
					/* Reached min */
					low_vref = new_vref;
					new_vref = final_vref;
					sweep++;
				} else {
					/* Continue searching min-vref */
					new_vref -= DRAM_VREFDQ_TRAIN_STEP_PDA;
				}
				break;
			} else if (sweep == 1) {
				if (new_vref > (DRAM_VREFDQ_MAX_VALUE - DRAM_VREFDQ_TRAIN_STEP_PDA)) {
					/* Reached max, calculate optimum */
					high_vref = new_vref;
					new_vref = (low_vref + high_vref) / 2;
					sweep++;
				} else {
					/* Continue searching max-vref */
					new_vref += DRAM_VREFDQ_TRAIN_STEP_PDA;
				}
				break;
			} else if (sweep >= 2) {
				/* New Optimum value passed; Done */
				final_vref_comp = new_vref;
				sweep++;
				done++;
				break;
			}
		}
	}
	if (!done && !err)
		goto fineloop;

	ddr_verbose("MCU[%d] COMP[%d] VREFDQ training: Final-Vref=0x%02X\n"
			"\t[Low=0x%02X High=0x%02X Optimum=0x%02X]\n",
			mcu_id, comp, final_vref_comp, low_vref,
			high_vref, new_vref);
	if (new_vref != final_vref_comp) {
		/*
		 * TODO: Something wrong happened?
		 * Can not exit vref training with differnet vref value
		 * updade final_vref as new_vref
		 */
		err = -1;
		final_vref_comp = new_vref;
	}

	/* MRS3: Enter PDA mode for target component to exit VREFDQ mode*/
	mr3_pda_access(mcu_id, rank_sel, PDA_ENTRY, comp, mcu->ddr_info.t_ck_ps,
			udp->ud_refresh_granularity);

	/* MRS6: Set New VREF-DQ for target component */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_TRAIN_MODE, udp->ud_dram_vref_range,
			final_vref_comp, t_ck_ps, comp);
	/* 1us delay for vref change to be stable */
	DELAY(1);
	/* MRS6: Exiting VREF-DQ training mode for target component */
	mr6_vreftrain(mcu_id, rank_sel, DRAM_VREFDQ_NORMAL_MODE, udp->ud_dram_vref_range,
			final_vref_comp, t_ck_ps, comp);

	/* Store final-verf per component */
	final_vref_pda[comp] = final_vref_comp;

	/* Exit PDA mode */
	mr3_pda_access(mcu_id, rank_sel, PDA_EXIT, comp,
			mcu->ddr_info.t_ck_ps, udp->ud_refresh_granularity);

	/* store per component error */
	if (err) {
		ddr_verbose("\tCOMP[%d] VREFDQ training failed!\n", comp);
		fail = -1;
	}

	comp++;
	if (comp < max_devices)
		goto comploop;

	/* Report New VREF-DQ setting per componet */
	ddr_verbose("\nFinal-VREFDQ: [Common value:0x%02X]\n\t", final_vref);
	for (comp = 0; comp < max_devices; comp++) {
		if (comp == 9)
			ddr_verbose("\n\t");
		ddr_verbose("[%d]=0x%02X ", comp, final_vref_pda[comp]);
	}
	ddr_verbose("\n");
	if (!fail)
		ddr_verbose("MCU[%d] VREFDQ Training PASS!\n\n", mcu_id);
	else
		ddr_err("MCU[%d] VREFDQ Training FAIL!\n\n", mcu_id);
	return fail;
}

/******************************************************************************
 *     PHY Vref Training functions (post train)
 *****************************************************************************/
int mcu_phy_vref_training(struct apm_mcu *mcu, unsigned int rankmask)
{
	apm_mcu_udparam_t *udp = (apm_mcu_udparam_t *) &mcu->mcu_ud;
	int err = 0;
	unsigned int mcu_id = mcu->id;
	unsigned int targetrank0, targetrank1;
	unsigned int sweep, done;
	unsigned int new_vref, low_vref, high_vref, final_vref = 0;
	unsigned int bist_result = 0, pattern;
	struct apm_mcu_bist_address_setup b_addr_set;
	struct apm_mcu_bist_config b_config;

	if (!udp->ud_phy_vref_train_en)
		return err;

	/* At least rank-0 will be valid */
	if (!(rankmask & 0x1)) {
		ddr_err("MCU[%d] Rank 0 not valid.\n", mcu_id);
		return -1;
	}
	targetrank0 = 0;
	/* if 2DPC, at least rank-4 will be valid else use slot-0 rank */
	if (mcu->ddr_info.two_dpc_enable) {
		if (rankmask & 0x10) {
			targetrank1 = 4;
		} else {
			ddr_err("MCU[%d] 2DPC enabled, but Rank 4 not valid.\n", mcu_id);
			return -1;
		}

		ddr_verbose("MCU[%d] Starting PHY VREF Training for ranks %d & %d",
			mcu_id, targetrank0, targetrank1);

		if (!targetrank0 || !targetrank1) {
			ddr_err("MCU[%d] PHY VREF training: Target ranks not present [%d %d]\n",
			mcu_id, targetrank0, targetrank1);
			return -1;
		}
	} else {
		targetrank1 = targetrank0;
		ddr_verbose("MCU[%d] Starting PHY VREF Training for rank %d\n",
			mcu_id, targetrank0);
	}

	/* Step1: Setup BIST for PHY Vref Training */
	/* Targetting different bank groups to insert breack in write burst */
	b_addr_set.rank0 =		targetrank0;
	b_addr_set.rank1 =		targetrank1;
	b_addr_set.bank =		MCU_BIST_BANK;
	b_addr_set.bankgroup0 =		MCU_BIST_BANKGROUP0;
	b_addr_set.bankgroup1 =		MCU_BIST_BANKGROUP1;
	b_addr_set.cid0 =		MCU_BIST_CID0;
	b_addr_set.cid1 =		MCU_BIST_CID1;
	b_addr_set.row =		MCU_BIST_ROW;
	b_addr_set.col =		MCU_BIST_COL;
	mcu_bist_address_setup(mcu_id, &b_addr_set);

	/* Read In-count=15: stop on read error, ex-count=high: regressive test */
	b_config.itercnt =		MCU_BIST_ITER_CNT;
	b_config.bistcrcen =		MCU_BIST_CRC_EN;
	b_config.bistcmprdsbl =		MCU_BIST_CMPR_DSBL;
	b_config.stoponmiscompare =	MCU_BIST_STOP_ON_MISCOMPARE;
	b_config.stoponalert =		MCU_BIST_STOP_ON_ALERT;
	b_config.readdbien =		MCU_BIST_READ_DBI_EN;
	b_config.writedbien =		MCU_BIST_WRITE_DBI_EN;
	b_config.mapcidtocs =		MCU_BIST_MAP_CID_TO_CS;
	b_config.rdloopcnt =		1;
	b_config.wrloopcnt =		MCU_BIST_WRLOOP_CNT;
	mcu_bist_config(mcu_id, 0, &b_config);

	/* Start VREF training for target ranks */
	/* Start from center of vref, and then search low and high bound */
	new_vref = udp->ud_phy_pad_vref_value;

	/* sweep = 0: Search low bound, 1: Search high bound, 2: Final test */
	sweep = 0;
	done = 0;

	high_vref = low_vref = 0;
phyvrefloop:
	/* Set new PHY VREF */
	phy_vref_ctrl(mcu_id, udp->ud_phy_pad_vref_range, new_vref);

	/* BIST test */
	for (pattern = 0; pattern < BIST_MAX_PATTERNS; pattern++) {
		err = bist_test_vref_training(mcu, pattern, &bist_result);
		if (err < 0) {
			ddr_err("MCU[%d] BIST poll fail.\n", mcu_id);
			return err;
		}
		/* Sweep low/high range for optimum vrefdq calculaiton */
		if (bist_result) {
			ddr_verbose("MCU[%d] VREF training: sweep=%d Vref=0x%02X\n",
				mcu_id, sweep, new_vref);
			if (sweep == 0) {
				/* Minumum vref reached; store and look for max */
				low_vref = new_vref;
				new_vref = udp->ud_phy_pad_vref_value;
				sweep++;
				break;
			} else if (sweep == 1) {
				/* Maximum vref reached; store and calculate optimum */
				high_vref = new_vref;
				new_vref = (low_vref + high_vref) / 2 - 2;
				sweep++;
				break;
			} else if (sweep == 2) {
				/* New optimum failed, set default and retest */
				ddr_err("MCU[%d]: Vref Training not stable\n", mcu_id);
				new_vref = udp->ud_phy_pad_vref_value;
				sweep++;
				err = -1;
				break;
			} else {
				/* Default/Optimum value failed, report error */
				ddr_err("MCU[%d]: Vref Training not stable\n", mcu_id);
				final_vref = new_vref;
				done++;
				err = -1;
				break;
			}
		} else if (pattern == BIST_MAX_PATTERNS - 1) {
			if (sweep == 0) {
				if (new_vref < (PHY_PAD_VREF_MIN_VALUE + PHY_PAD_VREF_STEP)) {
					/* Reached min value */
					low_vref = new_vref;
					ddr_verbose("MCU[%d] VREF training: sweep=%d \
							Vref=0x%02X\n", mcu_id, sweep, new_vref);
					new_vref = udp->ud_phy_pad_vref_value;
					sweep++;
				} else {
					/* Continue searching min-vref */
					new_vref -= PHY_PAD_VREF_STEP;
				}
				break;
			} else if (sweep == 1) {
				if (new_vref > (PHY_PAD_VREF_MAX_VALUE - PHY_PAD_VREF_STEP)) {
					/* Maximum vref reached; store and calculate optimum */
					high_vref = new_vref;
					ddr_verbose("MCU[%d] VREF training: sweep=%d \
							Vref=0x%02X\n", mcu_id, sweep, new_vref);
					new_vref = (low_vref + high_vref) / 2 - 2;
					sweep++;
				} else {
					/* Continue searching max-vref */
					new_vref += PHY_PAD_VREF_STEP;
				}
				break;
			} else if (sweep >= 2) {
				/* New Optimum value passed; Done */
				final_vref = new_vref;
				sweep++;
				done++;
				break;
			}
		}
	}

	if (!done && !err)
		goto phyvrefloop;

	ddr_verbose("MCU[%d] PHY VREF training: Final-Vref=0x%02X \
			\t[Low=0x%02X High=0x%02X Optimum=0x%02X]\n",
			mcu_id, final_vref, low_vref, high_vref, new_vref);

	if (new_vref != final_vref) {
		/*
		 * TODO: Something wrong happened?
		 * Can not exit vref training with differnet vref value
		 * updade final_vref as new_vref
		 */
		err = -1;
		final_vref = new_vref;
	}

	if (!err)
		ddr_verbose("MCU[%d] PHY VREF Training PASS!\n\n", mcu_id);
	else
		ddr_err("MCU[%d] PHY VREF Training FAIL!\n\n", mcu_id);
	return err;
}
