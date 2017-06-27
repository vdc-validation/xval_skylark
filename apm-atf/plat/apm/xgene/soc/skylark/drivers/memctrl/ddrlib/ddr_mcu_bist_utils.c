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

/******************************************************************************
 *     Mcu Bist Utility functions
 *****************************************************************************/
/* Register Dump */
void mcu_bist_datacmp_reg_dump(unsigned int mcu_id, unsigned int line)
{
	unsigned int iia, slice, wr_second_chunk, rd_second_chunk;
	unsigned long long wr_first_chunk, rd_first_chunk;

	apm_mcu_bist_2x64B_data_t wrdata[2];
	apm_mcu_bist_2x64B_data_t rddata[2];
	/*
	 * Data is stored in: unsigned char darr[8][9];
	 * [8] is 8 beats
	 * [9] is 9 bytes across 8 data + 1 ecc
	 */
	mcu_bist_peek_rddataX(mcu_id, &rddata[0], 0);
	mcu_bist_peek_rddataX(mcu_id, &rddata[1], 1);
	mcu_bist_peek_wrdataX(mcu_id, &wrdata[0], 0);
	mcu_bist_peek_wrdataX(mcu_id, &wrdata[1], 1);

	/*
	 * iia = beats
	 */
	if (line == 0) {
		for (iia = 0; iia < 8; iia++) {
			wr_first_chunk = 0;
			rd_first_chunk = 0;
			wr_second_chunk = 0;
			rd_second_chunk = 0;

			for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
				if (slice < CDNPHY_NUM_SLICES - 1) {
					wr_first_chunk |= ((unsigned long long)(wrdata[0].darr[iia][slice] & 0xFF) << (slice * 8));
					rd_first_chunk |= ((unsigned long long)(rddata[0].darr[iia][slice] & 0xFF) << (slice * 8));
				} else {
					wr_second_chunk |= (wrdata[0].darr[iia][8] & 0xFF);
                                        rd_second_chunk |= (rddata[0].darr[iia][8] & 0xFF);
				}
			}
			if ((wr_first_chunk != rd_first_chunk) | (wr_second_chunk != rd_second_chunk)) {
				ddr_verbose("[Beat %d Burst 0] ", iia);
				ddr_verbose("Expected: 0x%02X%016llX\t Actual: 0x%02X%016llX Mismatch: 0x%02X%016llX\n",
					    wr_second_chunk, wr_first_chunk, rd_second_chunk, rd_first_chunk,
					    wr_second_chunk ^ rd_second_chunk, wr_first_chunk ^ rd_first_chunk);
			}
		}
	} else {
		for (iia = 0; iia < 8; iia++) {
			wr_first_chunk = 0;
			rd_first_chunk = 0;
			wr_second_chunk = 0;
			rd_second_chunk = 0;

			for (slice = 0; slice < CDNPHY_NUM_SLICES; slice++) {
				if (slice < CDNPHY_NUM_SLICES - 1) {
					wr_first_chunk |= ((unsigned long long)(wrdata[1].darr[iia][slice] & 0xFF) << (slice * 8));
					rd_first_chunk |= ((unsigned long long)(rddata[1].darr[iia][slice] & 0xFF) << (slice * 8));
				} else {
					wr_second_chunk |= (wrdata[1].darr[iia][8] & 0xFF);
					rd_second_chunk |= (rddata[1].darr[iia][8] & 0xFF);
				}
			}
			if ((wr_first_chunk != rd_first_chunk) | (wr_second_chunk != rd_second_chunk)) {
				ddr_verbose("[Beat %d Burst 1] ", iia);
				ddr_verbose("Expected: 0x%02X%016llX\t Actual: 0x%02X%016llX Mismatch: 0x%02X%016llX\n",
					    wr_second_chunk, wr_first_chunk, rd_second_chunk, rd_first_chunk,
					    wr_second_chunk ^ rd_second_chunk, wr_first_chunk ^ rd_first_chunk);
                        }
		}
	}
}

/* TODO: Add Support for CRC */
void update_bist_datapattern(unsigned int mcu_id, unsigned int test)
{
	apm_mcu_bist_2x64B_data_t wrdata[2];
	unsigned int iia, jjb, data0, addrbase, dataX = 0;
	unsigned int pattern;

	/*
	 * Clearing the Read Data Registers
	 */
	for (dataX = 0; dataX < 2; dataX++) {
		addrbase = (dataX) ? PCP_RB_MCUX_BISTRDDATA1W00_PAGE_OFFSET :
					PCP_RB_MCUX_BISTRDDATA0W00_PAGE_OFFSET;
		/*
		 *  8 beats of 8 bytes = 64 bytes
		 */
		for (iia = 0 ; iia < 8; iia++) {
			for (jjb = 0; jjb < 2; jjb++) {
				data0 = 0;
				mcu_write_reg(mcu_id, addrbase + (2 * iia + jjb), data0);
			}
		}
		addrbase = (dataX) ? PCP_RB_MCUX_BISTRDDATA1ECC0_PAGE_OFFSET :
					PCP_RB_MCUX_BISTRDDATA0ECC0_PAGE_OFFSET;
		for (jjb = 0; jjb < 2; jjb++) {
			data0 = 0;
			mcu_write_reg(mcu_id, addrbase + jjb, data0);
		}
	}

	/*
	 * test < BIST_MAX_PATTERNS  to decide patterns for per bit deskew
	 * test >= BIST_MAX_PATTERNS to decide pattern for WR CAL
	 */
	if (test < BIST_MAX_PATTERNS) {
		/*
		 *  8 beats of 8 bytes = 64 bytes
		 */

		switch (test) {
		case 0: pattern = BIST_PATTERN0;
			break;
		case 1: pattern = BIST_PATTERN1;
			break;
		case 2: pattern = BIST_PATTERN2;
			break;
		default:pattern = BIST_PATTERN3;
			break;
		}

		for (iia = 0; iia < 8; iia++) {
			for (jjb = 0; jjb < CDNPHY_NUM_SLICES; jjb++) {
				wrdata[0].darr[iia][jjb] =  (iia & 1) ?
					(pattern) & 0xFF : (~pattern) & 0xFF;
				wrdata[1].darr[iia][jjb] =  (iia & 1) ?
					(~pattern) & 0xFF : (pattern) & 0xFF;
			}
		}
	} else if (test == BIST_MAX_PATTERNS) {
		/*
		 *  8 beats of 8 bytes = 64 bytes
		 */
		for (iia = 0; iia < 8; iia++) {
			for (jjb = 0; jjb < CDNPHY_NUM_SLICES; jjb++) {
				wrdata[0].darr[iia][jjb] =  0xFF &
					((0xf0e1d2c3f0e1d2c3ULL >> iia) >> jjb);
				wrdata[1].darr[iia][jjb] =  0xFF &
					((0x3c2d1e0f3c2d1e0fULL >> iia) >> jjb);
			}
		}
	} else {
		/*
		 *  Used for Write Calibration
		 *  8 beats of 9B == 72B
		 */
		for (iia = 0; iia < 8; iia++) {
			for (jjb = 0; jjb < CDNPHY_NUM_SLICES; jjb++) {
				/*
				 * Using iia for x4 support(different value per nibble per beat)
				 * Using jjb for x8 so that all bytes
				 * have different data across all beats
				 */
				wrdata[0].darr[iia][jjb] = ((iia << 4) & 0xF0) | (~iia & 0x0F);
				wrdata[1].darr[iia][jjb] = ((~jjb << 4) & 0xF0) | (~iia & 0x0F);
			}
		}
	}
	mcu_bist_data_setup(mcu_id, &wrdata[0], &wrdata[1]);
}

/* TODO: Add Support for CRC */
void mcu_bist_data_setup(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata0,
			apm_mcu_bist_2x64B_data_t *pdata1)
{
	unsigned int iia, jjb, data, addrbase;

	/*
	 * Iteration for a pair of Writes
	 */
	addrbase =  PCP_RB_MCUX_BISTWRDATA0W00_PAGE_OFFSET;
	for (iia = 0 ; iia < 8; iia++) {
		for (jjb = 0; jjb < 2; jjb++) {
			data = (((unsigned int)pdata0->darr[iia][3 + 4 * jjb] & 0xFF) << 24) |
				(((unsigned int)pdata0->darr[iia][2 + 4 * jjb] & 0xFF) << 16) |
				(((unsigned int)pdata0->darr[iia][1 + 4 * jjb] & 0xFF) << 8) |
				(((unsigned int)pdata0->darr[iia][0 + 4 * jjb] & 0xFF) );
			mcu_write_reg(mcu_id, addrbase + (2 * iia + jjb), data);
		}
	}

	addrbase =  PCP_RB_MCUX_BISTWRDATA1W00_PAGE_OFFSET;
	for (iia = 0 ; iia < 8; iia++) {
		for (jjb = 0; jjb < 2; jjb++) {
			data = (((unsigned int)pdata1->darr[iia][3 + 4 * jjb] & 0xFF) << 24) |
				(((unsigned int)pdata1->darr[iia][2 + 4 * jjb] & 0xFF) << 16) |
				(((unsigned int)pdata1->darr[iia][1 + 4 * jjb] & 0xFF) << 8) |
				(((unsigned int)pdata1->darr[iia][0 + 4 * jjb] & 0xFF) );
			mcu_write_reg(mcu_id, addrbase + (2 * iia + jjb), data);
		}
	}

	addrbase = PCP_RB_MCUX_BISTWRDATA0ECC0_PAGE_OFFSET;
	for (jjb = 0; jjb < 2; jjb++) {
		data = (((unsigned int)pdata0->darr[3 + 4 * jjb][8] & 0xFF) << 24) |
			(((unsigned int)pdata0->darr[2 + 4 * jjb][8] & 0xFF) << 16) |
			(((unsigned int)pdata0->darr[1 + 4 * jjb][8] & 0xFF) << 8) |
			(((unsigned int)pdata0->darr[0 + 4 * jjb][8] & 0xFF) );
		mcu_write_reg(mcu_id, addrbase + jjb, data);
	}

	addrbase = PCP_RB_MCUX_BISTWRDATA1ECC0_PAGE_OFFSET;
	for (jjb = 0; jjb < 2; jjb++) {
		data = (((unsigned int)pdata1->darr[3 + 4 * jjb][8] & 0xFF) << 24) |
			(((unsigned int)pdata1->darr[2 + 4 * jjb][8] & 0xFF) << 16) |
			(((unsigned int)pdata1->darr[1 + 4 * jjb][8] & 0xFF) << 8) |
			(((unsigned int)pdata1->darr[0 + 4 * jjb][8] & 0xFF) );
		mcu_write_reg(mcu_id, addrbase + jjb, data);
	}
}

void mcu_bist_peek_rddataX(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata,
			   unsigned int dataX)
{
	unsigned int iia, jjb, data, addrbase;
	/*
	 * Data is stored in: unsigned char darr[8][9];
	 * [8] is 8 beats
	 * [9] is 9 bytes across 8 data + 1 ecc
	 */
	addrbase = (dataX) ? PCP_RB_MCUX_BISTRDDATA1W00_PAGE_OFFSET :
				PCP_RB_MCUX_BISTRDDATA0W00_PAGE_OFFSET;
	for (iia = 0 ; iia < 8; iia++) {
		for (jjb = 0; jjb < 2; jjb++) {
			data = mcu_read_reg(mcu_id, addrbase + (2 * iia + jjb));
			pdata->darr[iia][3 + 4 * jjb] =	(unsigned char)((data >> 24) & 0xFF);
			pdata->darr[iia][2 + 4 * jjb] =	(unsigned char)((data >> 16) & 0xFF);
			pdata->darr[iia][1 + 4 * jjb] =	(unsigned char)((data >>  8) & 0xFF);
			pdata->darr[iia][0 + 4 * jjb] =	(unsigned char) (data & 0xFF);
		}
	}
	addrbase = (dataX) ? PCP_RB_MCUX_BISTRDDATA1ECC0_PAGE_OFFSET :
				PCP_RB_MCUX_BISTRDDATA0ECC0_PAGE_OFFSET;
	for (jjb = 0; jjb < 2; jjb++) {
		data = mcu_read_reg(mcu_id, addrbase + jjb);
		pdata->darr[3 + 4 * jjb][8] = (unsigned char)((data >> 24) & 0xFF);
		pdata->darr[2 + 4 * jjb][8] = (unsigned char)((data >> 16) & 0xFF);
		pdata->darr[1 + 4 * jjb][8] = (unsigned char)((data >>  8) & 0xFF);
		pdata->darr[0 + 4 * jjb][8] = (unsigned char) (data & 0xFF);
	}
}

void mcu_bist_peek_wrdataX(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata,
			   unsigned int dataX)
{
	unsigned int iia, jjb, data, addrbase;
	/*
	 * Data is stored in: unsigned char darr[8][9];
	 * [8] is 8 beats
	 * [9] is 9 bytes across 8 data + 1 ecc
	 */
	addrbase = (dataX) ? PCP_RB_MCUX_BISTWRDATA1W00_PAGE_OFFSET :
				PCP_RB_MCUX_BISTWRDATA0W00_PAGE_OFFSET;
	for (iia = 0 ; iia < 8; iia++) {
		for (jjb = 0; jjb < 2; jjb++) {
			data = mcu_read_reg(mcu_id, addrbase + (2 * iia + jjb));
			pdata->darr[iia][3 + 4 * jjb] = (unsigned char)((data >> 24) & 0xFF);
			pdata->darr[iia][2 + 4 * jjb] =	(unsigned char)((data >> 16) & 0xFF);
			pdata->darr[iia][1 + 4 * jjb] =	(unsigned char)((data >>  8) & 0xFF);
			pdata->darr[iia][0 + 4 * jjb] =	(unsigned char) (data & 0xFF);
		}
	}
	addrbase = (dataX) ? PCP_RB_MCUX_BISTWRDATA1ECC0_PAGE_OFFSET :
				PCP_RB_MCUX_BISTWRDATA0ECC0_PAGE_OFFSET;
	for (jjb = 0; jjb < 2; jjb++) {
		data = mcu_read_reg(mcu_id, addrbase + jjb);
		pdata->darr[3 + 4 * jjb][8] = (unsigned char)((data >> 24) & 0xFF);
		pdata->darr[2 + 4 * jjb][8] = (unsigned char)((data >> 16) & 0xFF);
		pdata->darr[1 + 4 * jjb][8] = (unsigned char)((data >>  8) & 0xFF);
		pdata->darr[0 + 4 * jjb][8] = (unsigned char) (data & 0xFF);
	}
}

int mcu_bist_datacmp(unsigned int mcu_id, unsigned long long bitmask, unsigned int ecc_bitmask)
{
	unsigned int bout = 0, iia, jjb, slice, bit;

	apm_mcu_bist_2x64B_data_t wrdata[2];
	apm_mcu_bist_2x64B_data_t rddata[2];
	/*
	 * Data is stored in: unsigned char darr[8][9];
	 * [8] is 8 beats
	 * [9] is 9 bytes across 8 data + 1 ecc
	 */
	mcu_bist_peek_rddataX(mcu_id, &rddata[0], 0);
	mcu_bist_peek_rddataX(mcu_id, &rddata[1], 1);
	mcu_bist_peek_wrdataX(mcu_id, &wrdata[0], 0);
	mcu_bist_peek_wrdataX(mcu_id, &wrdata[1], 1);

	/*
	 * iia = beats
	 * jjb = bits (72)
	 */
	for (iia = 0; iia < 8; iia++) {
		for (jjb = 0; jjb < (8 * CDNPHY_NUM_SLICES); jjb++) {
			slice =  jjb / 8;
			bit = jjb % 8;
			if (slice < CDNPHY_NUM_SLICES - 1) {
				if (!((bitmask >> jjb) & 1))
					continue;
			} else {
				if (!((ecc_bitmask >> bit) & 1))
					continue;
			}
			if (((wrdata[0].darr[iia][slice]) ^
				(rddata[0].darr[iia][slice])) & (1 << bit)) {
				/* Update bout to return the mask for 72 bits */
				bout |= 1;
                                /* TODO: Redo this to reduce display content */
				/*
				 * Can be used to print the pattern that failed
				 * ddr_verbose("[Beat %d Byte %d Bit %d Burst 0] 0x%x <> 0x%x\n",
				 *		iia, jjb / 8, bit, wrdata[0].darr[iia][slice],
				 *		rddata[0].darr[iia][slice]);
				 */
			}
		}
	}
	/*
	 * If first Rd Word MisCompares -
	 * Mcu does not capture 2nd Read Word
	 */
	if (bout != 0)
		return bout;

	for (iia = 0; iia < 8; iia++) {
		for (jjb = 0; jjb < (8 * CDNPHY_NUM_SLICES); jjb++) {
			slice =  jjb / 8;
			bit = jjb % 8;
			if (slice < CDNPHY_NUM_SLICES - 1) {
				if (!((bitmask >> jjb) & 1))
					continue;
			} else {
				if (!((ecc_bitmask >> bit) & 1))
					continue;
			}
			if (((wrdata[1].darr[iia][slice]) ^
				(rddata[1].darr[iia][slice])) & (1 << bit)) {
				/* Update bout to return the mask for 72 bits */
				bout = 1;
                                /* TODO: Redo this to reduce display content */
				/*
				 * Can be used to print the pattern that failed
				 * ddr_verbose("[Beat %d Byte %d Bit %d Burst 1] 0x%x <> 0x%x\n",
				 *		iia, jjb / 8, bit, wrdata[0].darr[iia][slice],
				 *		rddata[0].darr[iia][slice]);
				 */
			}
		}
	}
	return bout;
}

void mcu_bist_start(unsigned int mcu_id)
{
	unsigned int mcubistctl;

	/* BIST requires DMC to be in READY State */
	dmc_ready_state(mcu_id);

	mcubistctl = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_BISTEN_SET(mcubistctl, 1);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET, mcubistctl);
}

void mcu_bist_stop(unsigned int mcu_id)
{
	unsigned int mcubistctl;
	mcubistctl = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_BISTEN_SET(mcubistctl, 0);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET, mcubistctl);
}

int mcu_bist_completion_poll(unsigned int mcu_id)
{
	unsigned long long loopcnt;
	unsigned int rdloopcnt, wrloopcnt;
	int err = 0;
	/* loopcnt = wrloopcnt + rdloopcnt */
	wrloopcnt = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET);
	wrloopcnt = wrloopcnt & PCP_RB_MCUX_MCUBISTCTL_WRLOOPCNT_MASK;
	wrloopcnt = wrloopcnt >> PCP_RB_MCUX_MCUBISTCTL_WRLOOPCNT_SHIFT;
	rdloopcnt = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET);
	rdloopcnt = rdloopcnt & PCP_RB_MCUX_MCUBISTCTL_RDLOOPCNT_MASK;
	rdloopcnt = rdloopcnt >> PCP_RB_MCUX_MCUBISTCTL_RDLOOPCNT_SHIFT;
	loopcnt = rdloopcnt + wrloopcnt;
	loopcnt = loopcnt * (mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTITERCNT_PAGE_OFFSET));
	/*
	 * For Back to Back Transaction
	 * Polling Time = ((rd/wr inner loop )*itercnt) * ((30cycles) * (1250ps))
	 * 30 cycles  = wl(14) + burst(4) + wrcal(7) + write delay(2)
	 */
	err = (mcu_poll_reg(mcu_id, PCP_RB_MCUX_MCUBISTSTS_PAGE_OFFSET, 1,
					PCP_RB_MCUX_MCUBISTSTS_BIST_DONE_MASK, loopcnt));
	return err;
}

unsigned int mcu_bist_err_status(unsigned int mcu_id)
{
	unsigned int mcu_data = 0;
	mcu_data = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTSTS_PAGE_OFFSET);
	mcu_data = ((mcu_data & PCP_RB_MCUX_MCUBISTSTS_BIST_ERR_MASK) >>
					PCP_RB_MCUX_MCUBISTSTS_BIST_ERR_SHIFT);

	ddr_debug("MCU[%d]: BIST ERR STATUS: 0x%X\n", mcu_id, mcu_data);
	return mcu_data;
}

int mcu_bist_status(unsigned int mcu_id)
{
	/* Check ALERT_N register. status != 0 indicates error due to alert */
	unsigned int line0_status, line1_status;
	unsigned int status;

	line0_status = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTLINE0STS_PAGE_OFFSET);
	line1_status = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTLINE1STS_PAGE_OFFSET);
	status = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTALERT_PAGE_OFFSET);
	ddr_debug("MCU[%d]: BIST Line 0 Status: 0x%08X\nBIST Line 1 Status: 0x%08X\n"
		  "BIST ALERT Status: 0x%08X\n", mcu_id, line0_status, line1_status, status);
	if (status)
		return -1;
	else
		return (line0_status | line1_status);
}
int mcu_bist_byte_status_line0(unsigned int mcu_id, unsigned int device_type, unsigned int byte)
{
	unsigned int line0_status, line0_byte_status;

	line0_status = mcu_read_reg(mcu_id, PCP_RB_MCUX_MCUBISTLINE0STS_PAGE_OFFSET);
	if (device_type == REGV_MEMORY_DEVICE_WIDTH_X4)
		line0_byte_status = line0_status & (0x1 << byte);
	else
		line0_byte_status = line0_status & (0x3 << (byte * 2));
	ddr_debug("MCU[%d]: COMP[%d]:Line0 Status: 0x%08X Byte Status: 0x%08X\n",
			 mcu_id, byte, line0_status, line0_byte_status);
	return line0_byte_status;
}

void mcu_bist_address_setup(unsigned int mcu_id, struct apm_mcu_bist_address_setup *b_addr_set)
{

	unsigned int mcubistrowaddr = 0;
	unsigned int mcubistcoladdr = 0;
	unsigned int mcubistcidbank = 0;
	unsigned int mcubistrank = 0;
	unsigned int mcubistdatacsn = 0;

	mcubistcidbank = PCP_RB_MCUX_MCUBISTCIDBANK_BANK_SET(mcubistcidbank, b_addr_set->bank);
	mcubistcidbank = PCP_RB_MCUX_MCUBISTCIDBANK_BG0_SET(mcubistcidbank, b_addr_set->bankgroup0);
	mcubistcidbank = PCP_RB_MCUX_MCUBISTCIDBANK_BG1_SET(mcubistcidbank, b_addr_set->bankgroup1);
	mcubistcidbank = PCP_RB_MCUX_MCUBISTCIDBANK_CID0_SET(mcubistcidbank, b_addr_set->cid0);
	mcubistcidbank = PCP_RB_MCUX_MCUBISTCIDBANK_CID1_SET(mcubistcidbank, b_addr_set->cid1);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTCIDBANK_PAGE_OFFSET, mcubistcidbank);

	mcubistrowaddr = PCP_RB_MCUX_MCUBISTROWADDR_ADDR_SET(mcubistrowaddr, b_addr_set->row);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTROWADDR_PAGE_OFFSET, mcubistrowaddr);

	mcubistcoladdr = PCP_RB_MCUX_MCUBISTCOLADDR_ADDR_SET(mcubistcoladdr, b_addr_set->col);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTCOLADDR_PAGE_OFFSET, mcubistcoladdr);

	mcubistrank = PCP_RB_MCUX_MCUBISTRANK_LINE0RANK_SET(mcubistrank, b_addr_set->rank0);
	mcubistrank = PCP_RB_MCUX_MCUBISTRANK_LINE1RANK_SET(mcubistrank, b_addr_set->rank1);
	mcubistrank = PCP_RB_MCUX_MCUBISTRANK_LINE0LOGICALRANK_SET(mcubistrank, b_addr_set->rank0);
	mcubistrank = PCP_RB_MCUX_MCUBISTRANK_LINE1LOGICALRANK_SET(mcubistrank, b_addr_set->rank1);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTRANK_PAGE_OFFSET, mcubistrank);

	mcubistdatacsn = PCP_RB_MCUX_MCUBISTDATACSN_LINE0DATACSN_SET(mcubistdatacsn,
						(0xFF & ~(1 << b_addr_set->rank0)));
	mcubistdatacsn = PCP_RB_MCUX_MCUBISTDATACSN_LINE1DATACSN_SET(mcubistdatacsn,
						(0xFF & ~(1 << b_addr_set->rank1)));
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTDATACSN_PAGE_OFFSET, mcubistdatacsn);

}

void mcu_bist_config(unsigned int mcu_id, unsigned int bist2t, struct apm_mcu_bist_config *b_config)
{
	unsigned int mcubistitercnt = 0;
	unsigned int mcubistctl = 0;

	mcubistitercnt = PCP_RB_MCUX_MCUBISTITERCNT_ITERATION_COUNT_SET(mcubistitercnt, b_config->itercnt);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTITERCNT_PAGE_OFFSET, mcubistitercnt);

	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_BISTCRCEN_SET(mcubistctl, b_config->bistcrcen);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_BIST2T_SET(mcubistctl, bist2t);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_BISTCMPRDSBL_SET(mcubistctl, b_config->bistcmprdsbl);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_STOPONMISCOMPARE_SET(mcubistctl, b_config->stoponmiscompare);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_STOPONALERT_SET(mcubistctl, b_config->stoponalert);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_READDBIEN_SET(mcubistctl, b_config->readdbien);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_WRITEDBIEN_SET(mcubistctl, b_config->writedbien);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_MAPCIDTOCS_SET(mcubistctl, b_config->mapcidtocs);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_RDLOOPCNT_SET(mcubistctl, b_config->rdloopcnt);
	mcubistctl = PCP_RB_MCUX_MCUBISTCTL_WRLOOPCNT_SET(mcubistctl, b_config->wrloopcnt);
	mcu_write_reg(mcu_id, PCP_RB_MCUX_MCUBISTCTL_PAGE_OFFSET, mcubistctl);
}

void mcu_bist_setup (unsigned int mcu_id, unsigned int rank, unsigned int bist2t)
{
	struct apm_mcu_bist_address_setup b_addr_set;
	struct apm_mcu_bist_config b_config;

	b_addr_set.rank0 =		rank;
	b_addr_set.rank1 =		rank;
	b_addr_set.bank =		MCU_BIST_BANK;
	b_addr_set.bankgroup0 =		MCU_BIST_BANKGROUP0;
	b_addr_set.bankgroup1 =		MCU_BIST_BANKGROUP1;
	b_addr_set.cid0 =		MCU_BIST_CID0;
	b_addr_set.cid1 =		MCU_BIST_CID1;
	b_addr_set.row =		MCU_BIST_ROW;
	b_addr_set.col =		MCU_BIST_COL;

	b_config.itercnt =		MCU_BIST_ITER_CNT;
	b_config.bistcrcen =		MCU_BIST_CRC_EN;
	b_config.bistcmprdsbl =		MCU_BIST_CMPR_DSBL;
	b_config.stoponmiscompare =	MCU_BIST_STOP_ON_MISCOMPARE;
	b_config.stoponalert =		MCU_BIST_STOP_ON_ALERT;
	b_config.readdbien =		MCU_BIST_READ_DBI_EN;
	b_config.writedbien =		MCU_BIST_WRITE_DBI_EN;
	b_config.mapcidtocs =		MCU_BIST_MAP_CID_TO_CS;
	b_config.rdloopcnt =		MCU_BIST_RDLOOP_CNT;
	b_config.wrloopcnt =		MCU_BIST_WRLOOP_CNT;

	mcu_bist_address_setup(mcu_id, &b_addr_set);
	mcu_bist_config(mcu_id, bist2t, &b_config);
}
