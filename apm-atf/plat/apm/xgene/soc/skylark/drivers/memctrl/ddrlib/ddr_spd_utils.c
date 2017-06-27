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
#include "ddr_mimic_spd.h"

/* CRC check for SPD */
static int crc16(char *ptr, int count)
{
	int crc, i;
	crc = 0;
	ddr_info("Reading SPD from 0x%p\n\n", ptr);
	while (--count >= 0) {
		crc = crc ^ (int)*ptr++ << 8;
		for (i = 0; i < 8; ++i)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
	}

	return crc & 0xffff;
}

/* SPD checks */
int spd_check(spd_eeprom_t * spd)
{
	/*
	 * SPD byte0[7] - CRC coverage
	 * 0 = CRC covers bytes 0~125
	 * 1 = CRC covers bytes 0~116
	 */
	int spdcrc_chk_sum;
	int spdcrc_cs_len;
	char spdcrc_crc_lsb;	/* byte 126 */
	char spdcrc_crc_msb;	/* byte 127 */
	char spdcrc_rd_crclsb;	/* byte 126 */
	char spdcrc_rd_crcmsb;	/* byte 127 */

#ifdef DDR3_MODE
	if ((spd->info_size_crc) & 0x80)
		spdcrc_cs_len = 117;
	else
		spdcrc_cs_len = 126;
#else
	spdcrc_cs_len = 126;
#endif
	spdcrc_chk_sum = crc16((char *)spd, spdcrc_cs_len);

	spdcrc_crc_lsb = (unsigned char)(spdcrc_chk_sum & 0xff);
	spdcrc_crc_msb = (unsigned char)(spdcrc_chk_sum >> 8);

	spdcrc_rd_crclsb = spd->crc[0];
	spdcrc_rd_crcmsb = spd->crc[1];

	if ((spdcrc_rd_crclsb == spdcrc_crc_lsb)
	    && (spdcrc_rd_crcmsb == spdcrc_crc_msb)) {
		return 0;
	} else {
		ddr_err("SPD checksum unexpected.\n"
		      "Checksum lsb in SPD = %02X, computed SPD = %02X\n"
		      "Checksum msb in SPD = %02X, computed SPD = %02X\n",
		      spdcrc_rd_crclsb, spdcrc_crc_lsb, spdcrc_rd_crcmsb,
		      spdcrc_crc_msb);
		return -1;
	}
}

void print_spd_basic(spd_eeprom_t *spd)
{
	unsigned char __attribute__ ((unused)) *ptr = 0;
	unsigned char i;
	ptr = ((spd_eeprom_t *) spd)->mpart;
	ddr_info("\tDIMM Vendor : 0x%X%X\n", ((spd_eeprom_t *) spd)->dmid_lsb,
		 ((spd_eeprom_t *) spd)->dmid_msb);
	ddr_info("\tDIMM Part : ");
	ddr_info("%c", (*ptr++));
	for (i = 0; i < 18; i++)
		ddr_info("%c", (*ptr++));
	ddr_info("\n\t");

#ifdef DDR3_MODE
	if (spd->mem_type == SPD_MEMTYPE_DDR3) {
		switch (spd->module_type & 0xf) {
		case SPD_DDR3_MODTYPE_RDIMM:		/* RDIMM */
			ddr_info("RDIMM: ");
			break;
		case SPD_DDR3_MODTYPE_UDIMM:
			ddr_info("UDIMM: ");
			break;
		case SPD_DDR3_MODTYPE_SODIMM:
		case 0x08:		/* SO-DIMM */
			ddr_info("SODIMM: ");
			break;
		case SPD_DDR3_MODTYPE_MICRODIMM:
			ddr_info("Micro-DIMM: ");
			break;
		case SPD_DDR3_MODTYPE_MINIRDIMM:
			ddr_info("Mini-RDIMM: ");
			break;
		case SPD_DDR3_MODTYPE_MINIUDIMM:
			ddr_info("Mini-UDIMM ");
			break;
		case SPD_DDR3_MODTYPE_LRDIMM:
			ddr_info("LRDIMM: ");
			break;
		default:
			ddr_err("Unknown DIMM Type: ");
			break;
		}
	}
#else
	if (spd->mem_type == SPD_MEMTYPE_DDR4) {
		switch (spd->module_type & 0xf) {

		case SPD_DDR4_MODTYPE_RDIMM:
			ddr_info("RDIMM: ");
			break;
		case SPD_DDR4_MODTYPE_UDIMM:
			ddr_info("UDIMM: ");
			break;
		case SPD_DDR4_MODTYPE_SODIMM:
			ddr_info("SODIMM: ");
			break;
		case SPD_DDR4_MODTYPE_LRDIMM:
			ddr_info("LRDIMM: ");
			break;
		default:
			ddr_err("Unknown DIMM Type: ");
			break;
		}
	}
#endif
	else {
		ddr_err("Unsupported Memory Type!\n");
	}

	ddr_info("%dMB/rank\n",
	       (unsigned int)(compute_rank_capacity((spd_eeprom_t *) spd) / (1024 * 1024)));
}

/* DDR3/4 Memory Type */
unsigned int compute_mem_type(spd_eeprom_t * spd)
{
	return (spd->mem_type);
}

/* DDR3/4 Package Type */
unsigned int compute_package_type(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return (spd->sdram_dev_type & 0x3);
#else
	return (spd->package_type & 0x3);
#endif
}

/* DDR3/4 Memory device Type */
unsigned int compute_mem_device_type(spd_eeprom_t * spd)
{
	return (spd->organization & 0x3);
}

/* DDR3/4 Bank Group */
unsigned int compute_bank_group(spd_eeprom_t * spd)
{
	return (spd->mem_type == SPD_MEMTYPE_DDR3) ? 0 :
	       ((spd->density_banks >> 6) & 0x3);
}

/* DDR3/4 related compute rank capacity in bytes */
unsigned long long compute_rank_capacity(spd_eeprom_t * spd)
{
	unsigned long long bsize;

	int nbit_sdram_cap_bsize = 0;
	int nbit_primary_bus_width = 0;
	int nbit_sdram_width = 0;

	if ((spd->density_banks & 0xf) < 7)
		/* Capacity: 0 = 256Mb */
		nbit_sdram_cap_bsize = (spd->density_banks & 0xf) + 28;
	if ((spd->bus_width & 0x7) < 4)
		/* Bus width: 0 = 8 bits */
		nbit_primary_bus_width = (spd->bus_width & 0x7) + 3;
	if ((spd->organization & 0x7) < 4)
		/* Device Width: 0 = 4 bits */
		nbit_sdram_width = (spd->organization & 0x7) + 2;

	/* (nbit_sdram_cap_bsize - 3) = convert from bits to bytes */
	/* (nbit_primary_bus_width - nbit_sdram_width) = #device per rank */
	bsize = 1ULL << ((nbit_sdram_cap_bsize - 3)
			 + (nbit_primary_bus_width - nbit_sdram_width));

	return bsize;
}

/* DDR4 3DS Stack height */
unsigned int compute_stack_height(spd_eeprom_t * spd)
{

	if (spd->mem_type != SPD_MEMTYPE_DDR4)
		return 1;

	if ((spd->package_type & 0x3) == SPD_3DS) {
		switch (spd->package_type & 0x70) {
		case SPD_2H:
			return 2;
		case SPD_4H:
			return 4;
		case SPD_8H:
			return 8;
		default:
			ddr_err("Die Count not supported: ");
			return 0;
		}
	}
	return 1;
}

/* One Hot encoding of active ranks */
void compute_active_ranks(void *p)
{
	struct apm_mcu *mcu = (struct apm_mcu *)p;

	switch (mcu->ddr_info.max_ranks * (mcu->ddr_info.two_dpc_enable+1)) {
	case 1:
		mcu->ddr_info.active_ranks = 0x1;
		break;
	case 2:
		if (mcu->ddr_info.two_dpc_enable)
			mcu->ddr_info.active_ranks = 0x11;
		else
			mcu->ddr_info.active_ranks = 0x3;
		break;
	case 3:
		mcu->ddr_info.active_ranks = 0x7;
		break;
	case 4:
		if (mcu->ddr_info.two_dpc_enable)
			mcu->ddr_info.active_ranks = 0x33;
		else
			mcu->ddr_info.active_ranks = 0xF;
		break;
	case 5:
		mcu->ddr_info.active_ranks = 0x1F;
		break;
	case 6:
		mcu->ddr_info.active_ranks = 0x3F;
		break;
	case 7:
		mcu->ddr_info.active_ranks = 0x7F;
		break;
	case 8:
		mcu->ddr_info.active_ranks = 0xFF;
		break;
	}
}

/* Compute Odd Ranks */
void compute_odd_ranks(void *p)
{
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	unsigned int odd_ranks;

	/* Odd Rank DQ Swap */
	if (mcu->ddr_info.stack_high == 2) {
		odd_ranks = 0xCC;
	} else if (mcu->ddr_info.stack_high == 4) {
		if (mcu->ddr_info.two_dpc_enable)
			odd_ranks = 0x00;
		else
			odd_ranks = 0xF0;
	} else if (mcu->ddr_info.stack_high == 8) {
		odd_ranks = 0x00;
	} else {
		odd_ranks = 0xAA;
	}

	mcu->ddr_info.odd_ranks = odd_ranks & mcu->ddr_info.active_ranks;
}

/* Compute Physical Ranks */
void compute_physical_ranks(void *p)
{
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	unsigned int physical_ranks;

	/* Odd Rank DQ Swap */
	if (mcu->ddr_info.stack_high == 2) {
		physical_ranks = 0x55;
	} else if (mcu->ddr_info.stack_high == 4) {
		if (mcu->ddr_info.two_dpc_enable)
		        physical_ranks = 0x11;
		else
			physical_ranks = 0x01;
	} else if (mcu->ddr_info.stack_high == 8) {
		physical_ranks = 0x01;
	} else {
	        physical_ranks = 0xFF;
	}

	mcu->ddr_info.physical_ranks = physical_ranks & mcu->ddr_info.active_ranks;
}

/* DDR3/4 related compute number of logical ranks supported by DIMM */
unsigned int compute_no_ranks(spd_eeprom_t * spd)
{
	int package_ranks = 0;
	int logical_ranks = 0;

	/* Maximum number of ranks supported in 8 */
	/* If DDR4 check for 3DS DIMM, else same of DDR3 */
	package_ranks = ((spd->organization >> 3) & 0x7) + 1;

	/* Monolithic DRAM Device */
	if (spd->mem_type != SPD_MEMTYPE_DDR4) {
		logical_ranks = package_ranks;
		return logical_ranks;
	}

	/* Logical Ranks per DIMM = # Package Ranks per DIMM * # Logical Ranks per Package Rank */
	/* Monolithic vs Non-Monolithic device */
	if ((spd->package_type & 0x80) == 0x80) {
		switch (spd->package_type & 0x3) {
		case SPD_DDP:
			  logical_ranks = package_ranks;
			  break;
		case SPD_QDP:
			  logical_ranks = package_ranks;
			  break;
		case SPD_3DS:
			  switch (spd->package_type & 0x70) {
			  case SPD_2H:
				  logical_ranks = package_ranks * 2;
				  break;
			  case SPD_4H:
				  logical_ranks = package_ranks * 4;
				  break;
			  case SPD_8H:
				  logical_ranks = package_ranks * 8;
				  break;
			  default:
				  ddr_err("Die Count not supported: ");
				  break;
			  }
		default:
			ddr_err("Unknown DIMM Package Type: ");
			break;
		}
	} else {
		/* Monolithic */
		logical_ranks = package_ranks;
	}
	return logical_ranks;
}


/* DDR3/4 related compute row bits for the device */
unsigned int compute_row_bits(spd_eeprom_t * spd)
{
	return ((spd->addressing >> 3) & 0x7) + 12;
}


/* DDR3/4 related compute column bits for the device */
unsigned int compute_column_bits(spd_eeprom_t * spd)
{
	return (spd->addressing & 0x7) + 9;
}


/* DDR3/4 related compute bank bits for the device */
unsigned int compute_bank_bits(spd_eeprom_t * spd)
{
	return (spd->mem_type == SPD_MEMTYPE_DDR3) ?
	  ((spd->density_banks >> 4) & 0x7) + 0x3 :
	  (((spd->density_banks >> 6) & 0x3) + (((spd->density_banks >> 4) & 0x3)) + 0x2);
}

/* DDR3/4 check ECC capability */
unsigned int check_ecc_capability(spd_eeprom_t * spd)
{
	return (spd->bus_width >> 3);
}


/* DDR3/4 CAS Latency */


/* DDR3/4 CWL Latency */


/* Compute SPD timing using MTB and FTB */
unsigned int compute_spd_timing(int txx_mtb, int txx_ftb)
{
	unsigned int tXX_result;
	/*
	 * To recalculate the value of tXX from the SPD values, a general formula BIOSes may use is:
	 * tXX = tXX(MTB) * MTB + tXX(FTB) * FTB
	 */
	if (txx_ftb > 127) txx_ftb -= 256;
	tXX_result = ((txx_mtb * SPD_MTB_PS) + (txx_ftb * SPD_FTB_PS));
	return tXX_result;
}

#ifdef DDR3_MODE
/* DDR4 tCCD_L timing in ps*/
unsigned int compute_trrd_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tRRD_min, spd->tRRD_min_fos);

}
#else
/* DDR4 tCCD_L timing in ps*/
unsigned int compute_tccd_l_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tCCD_L_min, spd->tCCD_L_min_fos);

}

/* DDR4 tRRD_L timing in ps*/
unsigned int compute_trrd_l_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tRRD_L_min, spd->tRRD_L_min_fos);

}

/* DDR4 tRRD_S timing in ps*/
unsigned int compute_trrd_s_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tRRD_S_min, spd->tRRD_S_min_fos);

}

#endif

/* DDR3/4 tRC timing in ps*/
unsigned int compute_trc_timing(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return compute_spd_timing((((spd->tRAS_tRC_ext & 0xf0) << 4) | spd->tRC_min_lsb), spd->trcmin_correct);
#else
	return compute_spd_timing((((spd->tRAS_tRC_ext & 0xf0) << 4) | spd->tRC_min_lsb), spd->tRC_min_fos);
#endif
}

/* DDR3/4 tRAS timing in ps*/
unsigned int compute_tras_timing(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return compute_spd_timing((((spd->tRAS_tRC_ext & 0xf0) << 4) | spd->tRAS_min_lsb), spd->trcmin_correct);
#else
	return compute_spd_timing((((spd->tRAS_tRC_ext & 0xf0) << 4) | spd->tRAS_min_lsb), spd->tRC_min_fos);
#endif
}

/* DDR3/4 tRP timing in ps*/
unsigned int compute_trp_timing(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return compute_spd_timing(spd->tRP_min, spd->trpmin_correct);
#else
	return compute_spd_timing(spd->tRP_min, spd->tRP_min_fos);
#endif
}

/* DDR3/4 tRCD timing in ps*/
unsigned int compute_trcd_timing(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return compute_spd_timing(spd->tRCD_min, spd->trcdmin_correct);
#else
	return compute_spd_timing(spd->tRCD_min, spd->tRCD_min_fos);
#endif
}

/* DDR3/4 tAA timing in ps*/
unsigned int compute_taa_timing(spd_eeprom_t * spd)
{
#ifdef DDR3_MODE
	return compute_spd_timing(spd->tAA_min, spd->taamin_correct);
#else
	return compute_spd_timing(spd->tAA_min, spd->tAA_min_fos);
#endif
}

/* DDR3/4 tFAW timing in ps*/
unsigned int compute_tfaw_timing(spd_eeprom_t *spd)
{
	return compute_spd_timing(((spd->tFAW_msb & 0xf) << 8 | spd->tFAW_min), 0);
}

#ifdef DDR3_MODE
/* DDR3 tRFC timing in ps*/
unsigned int compute_trfc_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(((spd->tRFC_min_msb << 8) | spd->tRFC_min_lsb), 0);
}
#else
/* DDR4 tRFC1 timing in ps*/
unsigned int compute_trfc1_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(((spd->tRFC1_min_msb << 8) | spd->tRFC1_min_lsb), 0);
}

/* DDR4 tRFC2 timing in ps*/
unsigned int compute_trfc2_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(((spd->tRFC2_min_msb << 8) | spd->tRFC2_min_lsb), 0);
}

/* DDR4 tRFC4 timing in ps*/
unsigned int compute_trfc4_timing(spd_eeprom_t * spd)
{
	return compute_spd_timing(((spd->tRFC4_min_msb << 8) | spd->tRFC4_min_lsb), 0);
}
#endif


#ifdef DDR3_MODE
/* DDR3 tCK minimum supported in ps*/
unsigned int compute_tck_min_ps(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tCK_min, spd->tckmin_correct);
}

#else
/* DDR4 tCKAVG minimum supported in ps*/
unsigned int compute_tckavg_min_ps(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tCKAVG_min, spd->tCKAVG_min_fos);
}

/* DDR4 tCKAVG maximum supported in ps*/
unsigned int compute_tckavg_max_ps(spd_eeprom_t * spd)
{
	return compute_spd_timing(spd->tCKAVG_max, spd->tCKAVG_max_fos);
}
#endif

/*
 * Round mclk_ps to nearest 10 ps in memory controller code.
 *
 * If an imprecise data rate is too high due to rounding error
 * propagation, compute a suitably rounded mclk_ps to compute
 * a working memory controller configuration.
 */
unsigned int get_memory_clk_period_ps(void *p)
{
	unsigned int mclk_ps;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	struct apm_memc *memc = (struct apm_memc *) mcu->parent;
	generic_spd_eeprom_t *spd;

	spd = &mcu->spd_info[0].spd_info;

	if (mcu->mcu_ud.ud_speed_grade != 0) {
		mclk_ps = cdiv(2000000, mcu->mcu_ud.ud_speed_grade);
	} else {
#ifdef DDR3_MODE
		mclk_ps = compute_tck_min_ps(spd);
#else
		mclk_ps = compute_tckavg_min_ps(spd);
#endif

	}

	/* Check if DIMM supports tck */
#ifdef DDR3_MODE
	if (mclk_ps <  compute_tck_min_ps(spd))
		mclk_ps = compute_tck_min_ps(spd);
#else
	if (!mcu->mcu_ud.ud_pll_force_en) {
		if (mclk_ps <  compute_tckavg_min_ps(spd))
			mclk_ps = compute_tckavg_min_ps(spd);

		if (mclk_ps > compute_tckavg_max_ps(spd)){
			ddr_err("Unsupported Clock Frequency!\n");
			memc->p_handle_fatal_err(memc, 0);
		}
	}
#endif
	return (mclk_ps);
}

unsigned int mclk_to_picos(void *p, unsigned int mclk)
{
	return get_memory_clk_period_ps(p) * mclk;
}

/*
 * compute the CAS write latency according to DDR3/4 spec
 */
unsigned int compute_cas_write_latency(void *p)
{
	unsigned int cwl;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	const unsigned int mclk_ps = mcu->ddr_info.t_ck_ps;

#ifdef DDR3_MODE
	if (mclk_ps >= 2500)		/* 800 */
		cwl = 5;
	else if (mclk_ps >= 1875)	/* 1066 */
		cwl = 6;
	else if (mclk_ps >= 1499)	/* 1333 */
		cwl = 7;
	else if (mclk_ps >= 1250)	/* 1600 */
		cwl = 8;
	else if (mclk_ps >= 1071)	/* 1866 */
		cwl = 9;
	else if (mclk_ps >= 937)	/* 2133 */
		cwl = 10;
	else if (mclk_ps >= 833)	/* 2400 */
		cwl = 11;
	else
		cwl = 0;
#else
	if (mclk_ps >= 1499)		/* 1333 */
		cwl = 9;
	else if (mclk_ps >= 1250)	/* 1600 */
		cwl = 9;
	else if (mclk_ps >= 1071)	/* 1866 */
		cwl = 10;
	else if (mclk_ps >= 937)	/* 2133 */
		cwl = 11;
	else if (mclk_ps >= 833)	/* 2400 */
		cwl = (mcu->mcu_ud.ud_write_preamble_2t) ? 14 : 12;
	else if (mclk_ps >= 750)	/* 2666 */
		cwl = (mcu->mcu_ud.ud_write_preamble_2t) ? 16 : 14;
	else if (mclk_ps >= 625)	/* 3200 */
		cwl = (mcu->mcu_ud.ud_write_preamble_2t) ? 18 : 16;
	else
		cwl = 0;
#endif
        /* User defined write latency, no checks */
	if (mcu->mcu_ud.ud_force_cwl != -1)
		cwl = mcu->mcu_ud.ud_force_cwl;

	return cwl;
}

unsigned int compute_cas_latency(void *p)
{
	unsigned int tAAmin_ps = 0;
	unsigned int tCKmin_X_ps = 0;
	int common_caslat, caslat_actual;
	unsigned int retry = 16;
	unsigned int mclk_ps;
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	struct apm_memc *memc = (struct apm_memc *) mcu->parent;
	generic_spd_eeprom_t *spd;

	spd = &mcu->spd_info[0].spd_info;

	mclk_ps = mcu->ddr_info.t_ck_ps;

	/* compute the common CAS latency supported between slots */
	common_caslat = 0;
	if (!common_caslat) {
#ifdef DDR3_MODE
		common_caslat = ((spd->caslat_msb << 8) | spd->caslat_lsb) << 4;
#else
		common_caslat = ((spd->caslat_3 << 24) | (spd->caslat_2 << 16) |
				 (spd->caslat_1 <<  8) | (spd->caslat_0 <<  0)	) << 7;
#endif
		ddr_info("\tSPD CAS Latency MAP: 0x%x\n", common_caslat);
	} else {
#ifdef DDR3_MODE
		common_caslat &= ((spd->caslat_msb << 8) | spd->caslat_lsb) << 4;
#else
		common_caslat &= ((spd->caslat_3 << 24) | (spd->caslat_2 << 16) |
				  (spd->caslat_1 <<  8) | (spd->caslat_0 <<  0)  ) << 7;
#endif
		ddr_info("\tSPD CAS Latency MAP: 0x%x\n", common_caslat);
	}

	if (!common_caslat) {
		ddr_err("No CAS latency for any slot on this mcu\n");
		memc->p_handle_fatal_err(memc, 0);
		return 1;
	}

	/* compute the max tAAmin tCKmin between slots */
	tAAmin_ps   = compute_taa_timing(spd);

#ifdef DDR3_MODE
	tCKmin_X_ps = compute_tck_min_ps(spd);
#else
	tCKmin_X_ps = compute_tckavg_min_ps(spd);
#endif

	/* validate if the memory clk is in the range of dimms */
	//if (mclk_ps < tCKmin_X_ps) {	/* TODO */
	if ((mclk_ps + 1) < tCKmin_X_ps) {
		ddr_err("The DIMM max tCKmin is %d ps, "
			"doesn't support the MCLK cycle %d ps\n",
			tCKmin_X_ps, mclk_ps);

		if (mcu->mcu_ud.ud_pll_force_en) {
			ddr_info("Ignoring warning...\n");
		} else {
			return 1;
		}
	} else {
	  /*
	   * update min speed grade
	   * Maximum limitation
	   */
	}
	/* determine the acutal cas latency */
	caslat_actual = cdiv(tAAmin_ps, mclk_ps);
	ddr_info("The choosen cas latency %d common caslat %x\n",
		 caslat_actual, common_caslat);

	/* check if the dimms support the CAS latency */
	while (((common_caslat & (0x1 << caslat_actual)) == 0) && (retry > 0)) {
		caslat_actual++;
		retry--;
	}
	/* once the caculation of caslat_actual is completed
	 * we must verify that this CAS latency value does not
	 * exceed tAAmax, which is 20 ns for all DDR3 speed grades
	 */
#ifdef DDR3_MDOE
	if (caslat_actual * mclk_ps > 20000) {
		ddr_err("The choosen cas latency %d is too large\n",
			 caslat_actual);
		return 1;
	}
#else
	if (caslat_actual * mclk_ps > 18000) {
		ddr_err("The choosen cas latency %d is too large\n",
			 caslat_actual);
		return 1;
	}
#endif
        /* User defined cas latency, no checks */
	if (mcu->mcu_ud.ud_force_cl != -1)
		caslat_actual = mcu->mcu_ud.ud_force_cl;

	ddr_info("\tLowest common CAS latency: %d\n", caslat_actual);
	return caslat_actual;
}

/* Address mirroring */
unsigned int compute_addr_mirror(void *p)
{
	unsigned int addr_mirror;
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	generic_spd_eeprom_t *spd;

	spd = &mcu->spd_info[0].spd_info;

#ifdef DDR3_MODE
	/* UDIMM mirror */
	if ((spd->module_type & 0xf) == SPD_DDR3_MODTYPE_UDIMM)
		 addr_mirror = spd->mod_section.unbuffered.addr_mapping;
	else
		addr_mirror = 0;
#else

	/* UDIMM mirror */
	if ((spd->module_type & 0xf) == SPD_DDR4_MODTYPE_UDIMM)
		 addr_mirror = spd->mod_section.unbuffered.addr_mapping;
	/* RDIMM mirror */
	else if ((spd->module_type & 0xf) == SPD_DDR4_MODTYPE_RDIMM)
		 addr_mirror = spd->mod_section.registered.addr_mapping;
	/* LRDIMM mirror */
	else if ((spd->module_type & 0xf) == SPD_DDR4_MODTYPE_LRDIMM)
		addr_mirror = spd->mod_section.load_reduced.addr_mapping;
	else
		addr_mirror = 0;
#endif
	return addr_mirror;
}

/* Configure Read Preamble */
unsigned int config_read_preamble(void *p)
{
	unsigned int read_preamble;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	const unsigned int mclk_ps = mcu->ddr_info.t_ck_ps;

	if (mclk_ps <= 750)	   /* 2666 */
		read_preamble = 2;
	else
		read_preamble = 1;

	return read_preamble;
}

/* Check for Write Preamble */
int check_for_write_preamble(void *p)
{
	int err = 0;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	const unsigned int mclk_ps = mcu->ddr_info.t_ck_ps;

	if (mcu->mcu_ud.ud_write_preamble_2t == 1)
	  err = (mclk_ps >= 937) ? 0 : -1;

	return err;
}

/* Read DBI latency */
unsigned int config_read_dbi_latency(void *p)
{
	unsigned int read_dbi_latency;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	const unsigned int mclk_ps = mcu->ddr_info.t_ck_ps;
	generic_spd_eeprom_t *spd;

	spd = &mcu->spd_info[0].spd_info;

	if (mclk_ps <= 937)	   /* 2133 */
		read_dbi_latency = 3;
	else
		read_dbi_latency = 2;

	if ((mcu->mcu_ud.ud_rd_dbi_enable == 0) | ((spd->package_type & 0x3) == SPD_3DS))
		read_dbi_latency = 0;

	return read_dbi_latency;
}

/* Read latency */
unsigned int compute_write_latency(void *p)
{
	unsigned int write_latency;
	struct apm_mcu *mcu = (struct apm_mcu *) p;
	/* TODO */
	write_latency = mcu->ddr_info.cw_latency + mcu->ddr_info.parity_latency; //+ m_cmd_reg_lat - m_data_reg_lat;
	return write_latency;
}

/* Write latency */
unsigned int compute_read_latency(void *p)
{
	unsigned int read_latency, read_dbi_latency;
	struct apm_mcu *mcu = (struct apm_mcu *) p;

#ifdef DDR3_MODE
	read_dbi_latency = 0;
#else
	read_dbi_latency = config_read_dbi_latency(mcu);
#endif
	/* TODO */
	read_latency = mcu->ddr_info.cas_latency + mcu->ddr_info.parity_latency + read_dbi_latency; //+ m_cmd_reg_lat + m_data_reg_lat;
	return read_latency;
}

/* SPD Init, perform checks to match SPDs */
unsigned int spd_compare(mcu_spd_eeprom_t *mcu_spd_l, mcu_spd_eeprom_t *mcu_spd_r)
{
	unsigned int err = 0;
	spd_eeprom_t *spd_l, *spd_r;

	spd_l = &mcu_spd_l->spd_info;
	spd_r = &mcu_spd_r->spd_info;

	/* DIMM Manufature's ID check */
	if ((spd_l->mmid_lsb != spd_r->mmid_lsb) |
	    (spd_l->mmid_msb != spd_r->mmid_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			 "have different *** DIMM Manufacture's ID ***\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* Mfg's Module Part Number check */
#ifdef DDR3_MODE
	spd_l->mpart[17] = 0;
	spd_r->mpart[17] = 0;
#endif
	if (memcmp(spd_l->mpart, spd_r->mpart, sizeof(spd_l->mpart)) != 0) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			 "have different ***** (%s) (%s)\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot, mcu_spd_r->mcu_id,
			mcu_spd_r->slot, spd_l->mpart, spd_r->mpart);
		err +=1;
	}

	/* SDRAM Mfg's Module Part Number check */
	if ((spd_l->dmid_lsb != spd_r->dmid_lsb) |
	    (spd_l->dmid_msb != spd_r->dmid_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			 "have different *** SDRAM Manufacture's ID ***\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

#ifndef DDR3_MODE
	/* Connector to SDRAM Bit Mapping */
	if (memcmp(spd_l->cntr_bit_map, spd_r->cntr_bit_map, sizeof(spd_l->cntr_bit_map)) != 0) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			 "have different *** Connector to SDRAM bit mapping ***\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

	/* Check if mem type matches */
	if (spd_l->mem_type != spd_r->mem_type) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Mem Type ***\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

#ifdef DDR3_MODE
	/* UDIMM check */
	if ((spd_l->module_type & 0xf) == SPD_DDR3_MODTYPE_UDIMM) {
		if (spd_l->mod_section.unbuffered.addr_mapping != spd_r->mod_section.unbuffered.addr_mapping) {
				ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
					"have different *** Address Mapping ***\n",
					mcu_spd_l->mcu_id, mcu_spd_l->slot,
					mcu_spd_r->mcu_id, mcu_spd_r->slot);
				err +=1;
		}
	}
#else
	/* UDIMM check */
	if ((spd_l->module_type & 0xf) == SPD_DDR4_MODTYPE_UDIMM) {
		if (spd_l->mod_section.unbuffered.addr_mapping != spd_r->mod_section.unbuffered.addr_mapping) {
				ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
					"have different *** Address Mapping ***\n",
					mcu_spd_l->mcu_id, mcu_spd_l->slot,
					mcu_spd_r->mcu_id, mcu_spd_r->slot);
				err +=1;
		}
	}
#endif

#ifdef DDR3_MODE
	/* RDIMM check */
	if ((spd_l->module_type & 0xf) == SPD_DDR3_MODTYPE_RDIMM) {
		if ((spd_l->mod_section.registered.rmid_lsb != spd_r->mod_section.registered.rmid_lsb) |
		    (spd_l->mod_section.registered.rmid_msb != spd_r->mod_section.registered.rmid_msb)) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Register Manufacture's ID ***\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}
	}
#else
	/* RDIMM check */
	if ((spd_l->module_type & 0xf) == SPD_DDR4_MODTYPE_RDIMM) {
		if ((spd_l->mod_section.registered.rmid_lsb != spd_r->mod_section.registered.rmid_lsb) |
		    (spd_l->mod_section.registered.rmid_msb != spd_r->mod_section.registered.rmid_msb)) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Register Manufacture's ID ***\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}
	}
#endif

#ifndef DDR3_MODE
	/* RDIMM check */
	if ((spd_l->module_type & 0xf) == SPD_DDR4_MODTYPE_RDIMM) {
		if (spd_l->mod_section.registered.rmrev != spd_r->mod_section.registered.rmrev) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Register Revision Number ****\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}

		if (spd_l->mod_section.registered.addr_mapping != spd_r->mod_section.registered.addr_mapping) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Address Mapping ***\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}
	}
#endif

	/* LRDIMM check */
#ifndef DDR3_MODE
	if ((spd_l->module_type & 0xf) == SPD_DDR4_MODTYPE_LRDIMM) {
		if ((spd_l->mod_section.load_reduced.lrmid_lsb != spd_r->mod_section.load_reduced.lrmid_lsb) |
		    (spd_l->mod_section.load_reduced.lrmid_msb != spd_r->mod_section.load_reduced.lrmid_msb)) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Memory Buffer Manufacture's ID ***\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}

		if (spd_l->mod_section.load_reduced.rmrev != spd_r->mod_section.load_reduced.rmrev) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Register Revision Number ****\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}
		if (spd_l->mod_section.load_reduced.addr_mapping != spd_r->mod_section.load_reduced.addr_mapping) {
			ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
				"have different *** Address Mapping ***\n",
				mcu_spd_l->mcu_id, mcu_spd_l->slot,
				mcu_spd_r->mcu_id, mcu_spd_r->slot);
			err +=1;
		}
	}
#endif

	/* Module Type check */
	if (spd_l->module_type != spd_r->module_type) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** DIMM Type ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* Addressing check */
	if (spd_l->addressing != spd_r->addressing) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** DIMM Type ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* Package Type check */
#ifdef DDR3_MODE
	if (spd_l->sdram_dev_type != spd_r->sdram_dev_type) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Package Type ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	if (spd_l->package_type != spd_r->package_type) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Package Type ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

	/* Module Organization (# of ranks and Device width) check */
	if (spd_l->organization != spd_r->organization) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Ranks/Device Width ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* Bus width check */
	if (spd_l->bus_width != spd_r->bus_width) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			 "have different *** Bus Width ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

#ifdef DDR3_MODE
	/* tCK_min check */
	if ((spd_l->tCK_min != spd_r->tCK_min) |
	    (spd_l->tckmin_correct != spd_r->tckmin_correct)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** tCK_min ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tCKAVG_min check */
	if ((spd_l->tCKAVG_min != spd_r->tCKAVG_min) |
	    (spd_l->tCKAVG_min_fos != spd_r->tCKAVG_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** tCKAVG_min ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

#ifdef DDR3_MODE
	/* tAA_min check */
	if ((spd_l->tAA_min != spd_r->tAA_min) |
	    (spd_l->taamin_correct != spd_r->taamin_correct)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min CAS Latency ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tAA_min check */
	if ((spd_l->tAA_min != spd_r->tAA_min) |
	    (spd_l->tAA_min_fos != spd_r->tAA_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min CAS Latency ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

#ifdef DDR3_MODE
	/* tWR_min check */
	if (spd_l->tWR_min != spd_r->tWR_min) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min Write Recovery Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

#ifdef DDR3_MODE
	/* tRCD_min check */
	if ((spd_l->tRCD_min != spd_r->tRCD_min) |
	    (spd_l->trcdmin_correct != spd_r->trcdmin_correct)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min RAS# to CAS# Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tRCD_min check */
	if ((spd_l->tRCD_min != spd_r->tRCD_min) |
	    (spd_l->tRCD_min_fos != spd_r->tRCD_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min RAS# to CAS# Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

#ifdef DDR3_MODE
	/* tRRD_min check */
	if ((spd_l->tRRD_min != spd_r->tRRD_min) |
	    (spd_l->tRRD_min_fos != spd_r->tRRD_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min Row Active to Row Active Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tRRD_L check */
	if ((spd_l->tRRD_L_min != spd_r->tRRD_L_min) |
	    (spd_l->tRRD_L_min_fos != spd_r->tRRD_L_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min Row Active to Row Active Delay Time same bank group ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tRRD_S check */
	if ((spd_l->tRRD_S_min != spd_r->tRRD_S_min) |
	    (spd_l->tRRD_S_min_fos != spd_r->tRRD_S_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min Row Active to Row Active Delay Time different bank group ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tCCD_L check */
	if ((spd_l->tCCD_L_min != spd_r->tCCD_L_min) |
	    (spd_l->tCCD_L_min_fos != spd_r->tCCD_L_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different *** Min CAS to CAS Delay Time, same bank group ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

#ifdef DDR3_MODE
	/* tRP_min check */
	if ((spd_l->tRP_min != spd_r->tRP_min) |
	    (spd_l->trpmin_correct != spd_r->trpmin_correct)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Row Precharge Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tRP_min check */
	if ((spd_l->tRP_min != spd_r->tRP_min) |
	    (spd_l->tRP_min_fos != spd_r->tRP_min_fos)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Row Precharge Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif

	/* tRAS_tRC_ext check */
	if (spd_l->tRAS_tRC_ext != spd_r->tRAS_tRC_ext) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Upper Nibbles for tRAS and tRC ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tRAS_min_lsb check */
	if (spd_l->tRAS_min_lsb != spd_r->tRAS_min_lsb) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Active to Precharge Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tRC_min_lsb check */
	if (spd_l->tRC_min_lsb != spd_r->tRC_min_lsb) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Active to Active/Refresh Delay Time, LSB ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

#ifdef DDR3_MODE
	/* tRFC_min check */
	if ((spd_l->tRFC_min_lsb != spd_r->tRFC_min_lsb) |
	    (spd_l->tRFC_min_msb != spd_r->tRFC_min_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Refresh Recovery Delay Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#else
	/* tRFC1_min check */
	if ((spd_l->tRFC1_min_lsb != spd_r->tRFC1_min_lsb) |
	    (spd_l->tRFC1_min_msb != spd_r->tRFC1_min_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Refresh Recovery Delay Time 1 ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tRFC2_min check */
	if ((spd_l->tRFC2_min_lsb != spd_r->tRFC2_min_lsb) |
	    (spd_l->tRFC2_min_msb != spd_r->tRFC2_min_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Refresh Recovery Delay Time 2 ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	/* tRFC4_min check */
	if ((spd_l->tRFC4_min_lsb != spd_r->tRFC4_min_lsb) |
	    (spd_l->tRFC4_min_msb != spd_r->tRFC4_min_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  Min Refresh Recovery Delay Time 4 ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}
#endif
	/* tFAW_min check */
	if ((spd_l->tFAW_min != spd_r->tFAW_min) |
	    (spd_l->tFAW_msb != spd_r->tFAW_msb)) {
		ddr_err("MCU[%d]-Slot[%d] & MCU[%d]-Slot[%d] "
			"have different ***  FAW Time ****\n",
			mcu_spd_l->mcu_id, mcu_spd_l->slot,
			mcu_spd_r->mcu_id, mcu_spd_r->slot);
		err +=1;
	}

	return err;
}	  /* spd_compare  */

/* SPD Init, perform checks to match SPDs */
int spd_init(void *p)
{
	int i, err;
	unsigned int activeslots;
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	struct apm_memc *memc = (struct apm_memc *)mcu->parent;

	err = 0;
	activeslots = 0;

	/* MIMIC_SPD */
	if (mcu->mcu_ud.ud_ddr_mimicspd) {
		activeslots = mcu->mcu_ud.ud_mimic_activeslots;
		/* using mimic SPD values based on "ddr_mimic_spd_array" */
		ddr_warn("Using mimic SPD values based on ddr_mimic_spd_array"
			 " [active-slots:0x%d] *\n", activeslots);
	}

	for (i = 0; i < CONFIG_SYS_DIMM_SLOTS_PER_CTLR; i++) {
		mcu->spd_info[i].mcu_id = mcu->id;
		mcu->spd_info[i].slot = i;
		/* MIMIC_SPD */
		if (mcu->mcu_ud.ud_ddr_mimicspd) {
			if (!((activeslots >> i) & 0x1))
				continue;

			ddr_info(" GetFixed SPD MCU[%d]-Slot[%d]\n", mcu->id, i);
			/* consider both slots have same 'part-number' dimm */
			memcpy((char *)&mcu->spd_info[i].spd_info,
			       ddr_mimic_spd_array, sizeof(spd_eeprom_t));
		} else {
			ddr_info(" GetI2C SPD MCU[%d]-Slot[%d]\n", mcu->id, i);
			err = memc->memc_ud.ud_spd_get(mcu, i);
			if (err < 0)
				continue;
			activeslots |= 0x1 << i;
			/* Add spd_check */
			if (spd_check(&mcu->spd_info[i].spd_info) < 0)
				return -1;
		}

	}
	err = 0;
	if (!activeslots || !mcu->enabled) {
		mcu->enabled = 0;
		ddr_info("MCU[%d] No DIMMs detected, disabling MCU\n", mcu->id);
		return 0;
	} else if ((activeslots == 0x2) | !mcu->enabled) {
		mcu->enabled = 0;
		ddr_err("MCU[%d] No DIMMs in Slot 0 detected, disabling MCU\n",
			mcu->id);
		return 0;
	} else {
		mcu->enabled = 1;
	}

	ddr_info("MCU[%d] Total Active Slots: 0x%d\n", mcu->id, activeslots);

	if (activeslots == 0x3) {
		/* Compare Slot 0 and Slot 1 */
		err = spd_compare(&mcu->spd_info[0], &mcu->spd_info[1]);
		if (err) {
			ddr_err("MCU[%d] Active slots mismatch\n", mcu->id);
			return -1;
		}
	}

	/* update the struct */
	mcu->activeslots = activeslots;

	/* Update DDR Info */
	mcu->ddr_info.ddr_type	      = mcu->spd_info[0].spd_info.mem_type;

	switch (mcu->spd_info[0].spd_info.module_type & 0xf) {
#ifdef DDR3_MODE
	case SPD_DDR3_MODETYPE_RDIMM:
		mcu->ddr_info.package_type = RDIMM;
		break;
	case SPD_DDR3_MODETYPE_UDIMM:
		mcu->ddr_info.package_type = UDIMM;
		break;
	case SPD_DDR3_MODETYPE_SODIMM:
		mcu->ddr_info.package_type = SODIMM;
		break;
	case SPD_DDR3_MODETYPE_LRDIMM:
		mcu->ddr_info.package_type = LRDIMM;
		break;
	default:
		break;
#else
	case SPD_DDR4_MODTYPE_RDIMM:
		mcu->ddr_info.package_type = RDIMM;
		break;
	case SPD_DDR4_MODTYPE_UDIMM:
		mcu->ddr_info.package_type = UDIMM;
		break;
	case SPD_DDR4_MODTYPE_SODIMM:
		mcu->ddr_info.package_type = SODIMM;
		break;
	case SPD_DDR4_MODTYPE_LRDIMM:
		mcu->ddr_info.package_type = LRDIMM;
		break;
	default:
		break;
#endif
	}

	mcu->ddr_info.device_type     = mcu->spd_info[0].spd_info.organization & 0x3;

	mcu->ddr_info.two_dpc_enable  = (activeslots == 0x3);
	mcu->ddr_info.stack_high      = compute_stack_height(&mcu->spd_info[0].spd_info);
	mcu->ddr_info.max_ranks       = compute_no_ranks(&mcu->spd_info[0].spd_info);

	compute_active_ranks(mcu);
	mcu->ddr_info.t_ck_ps	      = get_memory_clk_period_ps(mcu);
	mcu->ddr_info.cas_latency     = compute_cas_latency(mcu);
	mcu->ddr_info.cw_latency      = compute_cas_write_latency(mcu);
	mcu->ddr_info.addr_mirror     = compute_addr_mirror(mcu);
	mcu->ddr_info.read_preamble   = config_read_preamble(mcu);

	err = check_for_write_preamble(mcu);
	if (err) {
	  /* TODO */
	}

	mcu->ddr_info.write_preamble  = mcu->mcu_ud.ud_write_preamble_2t ? 2 : 1;
	mcu->ddr_info.parity_latency  = 0;
	mcu->ddr_info.read_latency    = compute_read_latency(mcu);
	mcu->ddr_info.write_latency   = compute_write_latency(mcu);
	mcu->ddr_info.ecc_en          = mcu->mcu_ud.ud_ecc_enable;
	compute_odd_ranks(mcu);
	compute_physical_ranks(mcu);

	return err;
}	  /* spd_init  */
