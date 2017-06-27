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

#include <arch_helpers.h>
#include <apm_ddr.h>
#include <console_core.h>
#include <i2c.h>
#include <xgene_def.h>
#include <xgene_dimm_info.h>
#include <xgene_nvparam.h>
#include <xlat_tables.h>
#include "ddrlib/ddr_lib.h"
#include "ddrlib/ddr_ud.h"
#include "smpro_misc.h"

#define PROGRESS_STR_SZ		53
int ddrlog_verbose = DDR_LOG_LEVEL_WARNING;

static struct apm_memc memc;

typedef enum check_param_type {
	/* To be added */
	DDR_SPEED,
	DDR_MCU_MASK
} check_param_type_t;

struct xgene_plat_dram_info {
	u32 nr_regions;
	u64 total_size;
	u64 base[NR_MEM_REGIONS];
	u64 size[NR_MEM_REGIONS];
	u32 cur_speed;
};
static struct xgene_plat_dram_info sys_mem_info;
static struct xgene_plat_dimm_list *sys_dimm_info =
		(struct xgene_plat_dimm_list *) PLAT_XGENE_DIMMINFO_BASE;
static void ddr_xgene_disable_mmu_el3(void);

void ddr_bl_meminfo_update(void)
{
	struct xgene_plat_dram_info *meminfo;

	ddr_info("Update system DRAM information\n");
	meminfo = (struct xgene_plat_dram_info *) PLAT_XGENE_MEMINFO_BASE;
	memcpy((void *) meminfo, (void *) &sys_mem_info, sizeof(sys_mem_info));
}

static void ddr_progress_console_init(void)
{
	console_core_init(XGENE_BOOT_NS_UART_BASE, XGENE_BOOT_UART_CLK_IN_HZ,
			  XGENE_BOOT_UART_BAUDRATE);
}

static void ddr_progress_pr(int c)
{
	console_core_putc(c, XGENE_BOOT_NS_UART_BASE);
}

void ddr_progress_bar(unsigned int percent)
{
	static char progr_str[PROGRESS_STR_SZ];
	static char bar_str[20];
	char *p;
	int i;

	for (i = 0; i < 20; i++) {
		if (i <= percent / 5) {
			bar_str[i] = '=';
			continue;
		}
		if (i == percent / 5 + 1)
			bar_str[i] = '>';
		else
			bar_str[i] = ' ';
	}

	for (i = 0; i < PROGRESS_STR_SZ; i++)
		ddr_progress_pr('\b');

	p = &bar_str[0];
	sprintf(progr_str, "DRAM Initialization: [%3d%%] [ %s ]", percent, p);
	progr_str[PROGRESS_STR_SZ - 1] = '\0';
	p = &progr_str[0];
	while (*p)
		ddr_progress_pr(*p++);
	if (percent == 100)
		ddr_progress_pr('\n');
}

static u32 ddr_get_env(unsigned int nvparam, u32 default_val)
{
#if !XGENE_VHP
	u32 val;

	if (!plat_nvparam_get(nvparam, NV_PERM_ATF, &val))
		return val;
#endif
	return default_val;
}

/*
 * Check whether user parameters valid to avoid users' mistakes
 */
static int is_ddr_param_valid(check_param_type_t param_type, u32 param)
{
	if (param_type == DDR_SPEED) {
		switch (param) {
		case 1600:
		case 1866:
		case 2133:
		case 2400:
		case 2666:
		case 2667:
			return 1;
		default:
			return 0;
		}
	}
	if (param_type == DDR_MCU_MASK) {
		switch (param & 0xFF) {
		case 0x01:
		case 0x03:
		case 0x11:
		case 0x33:
		case 0x0F:
		case 0xFF:
			return 1;
		default:
			return 0;
		}
	}
	return 0;
}

int ddr_apm_dcache_flush(void *p, unsigned int flag)
{
	ddr_verbose("dcache flush...\n");
	dcsw_op_all(DCCISW);

	return 0;
}

int ddr_get_spd(void *p, unsigned int slot)
{
#if XGENE_VHP || XGENE_EMU
	return 0;
#else
	struct apm_mcu *mcu = (struct apm_mcu *)p;
	spd_eeprom_t *spd;
	unsigned char i2c_address;
	unsigned char val;
	int spd_sz, err;

	err = i2c_set_bus_num(2);
	if (err) {
		printf("i2c_set_bus_num error:%d\n", err);
		return -1;
	}

	/* No I2C address, consider not populated */
	i2c_address = mcu->spd_addr[slot];
	if (i2c_address == 0)
		return -1;

	spd = &mcu->spd_info[slot].spd_info;
	memset(spd, 0, sizeof(spd_eeprom_t));
	/*
	 * If a mux is provisioned, set the mux correctly first.
	 * Otherwise, just move on detecting SPDs.
	 */
	if (!mcu->spd_mux)
		goto no_mux;

	err = i2c_probe(mcu->spd_mux);
	if (!err) {
		err = i2c_write(mcu->spd_mux, 0, 1,
				(unsigned char *)&mcu->spd_mux_addr, 1, 0);
		if (err) {
			ddr_err("MCU[%d]: Fail send command to MUX\n", mcu->id);
			return err;
		}
	}

no_mux:
	/* Check if the SPD is dectected */
	err = i2c_probe(i2c_address);
	if (err)
		return -1;

	/* Check for DIMM type before reading */
	err = i2c_read(i2c_address, 2, 1, &val, 1);
	if (err || (val != SPD_MEMTYPE_DDR4)) {
		ddr_verbose("\tMCU[%d]-Slot[%d]-NO-DIMM\n", mcu->id, slot);
		return -1;
	}

	ddr_verbose("MCU[%d]-Slot[%d]-DDR4-DIMM-FOUND\n", mcu->id, slot);
	spd_sz = sizeof(spd_eeprom_t);
	err = i2c_read(i2c_address, 0, 1, (unsigned char *)spd, spd_sz / 2);
	if (err) {
		ddr_err("MCU[%d]-Slot[%d]: Error reading first 256 bytes SPD\n",
			mcu->id, slot);
		return -1;
	}

	/* Select page 1 - Upper 256 bytes */
	val = 0;
	err = i2c_write(0x37, 0, 1, &val, 1, 1);
	err += i2c_read(i2c_address, 0, 1, (unsigned char *)spd + spd_sz / 2,
			spd_sz / 2);
	/* Reset to page 0 */
	err += i2c_write(0x36, 0, 1, &val, 1, 1);
	if (err)
		ddr_err("MCU[%d]-Slot[%d]: Error reading upper 256 bytes SPD\n",
			mcu->id, slot);

	return err;
#endif
}

int ddr_update_dimm_list(struct apm_memc *memc)
{
	struct apm_mcu *mcu;
	int i, j, di = 0;
	generic_spd_eeprom_t *spd;
	struct xgene_plat_dimm_info *dimm_info;
	struct xgene_mem_spd_data *dimm_data;

	for_each_mcu(i) {
		mcu = (struct apm_mcu *) &memc->mcu[i];

		for (j = 0; j <= 1; j++) {
			if (!mcu->enabled || !(mcu->activeslots & (1 << j))) {
				sys_dimm_info->dimm[di++].info.dimm_status =
						DIMM_NOT_INSTALLED;
				continue;
			}

			spd = &mcu->spd_info[j].spd_info;
			dimm_data = &sys_dimm_info->dimm[di].spd_data;
			dimm_info = &sys_dimm_info->dimm[di++].info;
			dimm_info->dimm_status = DIMM_INSTALLED;
			memcpy(&dimm_data->Byte2, (uint8_t *)spd + 2,
					sizeof(dimm_data->Byte2));
			memcpy(dimm_data->Byte5To8, (uint8_t *)spd + 5,
					sizeof(dimm_data->Byte5To8));
			memcpy(dimm_data->Byte11To14, (uint8_t *)spd + 11,
					sizeof(dimm_data->Byte11To14));
			memcpy(dimm_data->Byte64To71, (uint8_t *)spd + 64,
					sizeof(dimm_data->Byte64To71));
			memcpy(dimm_data->Byte73To90, (uint8_t *)spd + 73,
					sizeof(dimm_data->Byte73To90));
			memcpy(dimm_data->Byte95To98, (uint8_t *)spd + 95,
					sizeof(dimm_data->Byte95To98));
			memcpy(dimm_data->Byte117To118, (uint8_t *)spd + 117,
					sizeof(dimm_data->Byte117To118));
			memcpy(dimm_data->Byte122To125, (uint8_t *)spd + 122,
					sizeof(dimm_data->Byte122To125));
			memcpy(dimm_data->Byte128To145, (uint8_t *)spd + 128,
					sizeof(dimm_data->Byte128To145));
			memcpy(dimm_data->Byte320To321, (uint8_t *)spd + 320,
					sizeof(dimm_data->Byte320To321));
			memcpy(dimm_data->Byte325To328, (uint8_t *)spd + 325,
					sizeof(dimm_data->Byte325To328));
			memcpy(dimm_data->Byte329To348, (uint8_t *)spd + 329,
					sizeof(dimm_data->Byte329To348));
			dimm_info->dimm_type = mcu->ddr_info.package_type;
			dimm_info->dimm_mfc_id = (spd->dmid_msb << 8)
							| spd->dmid_lsb;
			dimm_info->dimm_nr_rank = compute_no_ranks(spd);
			dimm_info->dimm_size = dimm_info->dimm_nr_rank *
						compute_rank_capacity(spd);
			dimm_info->dimm_dev_type = compute_mem_device_type(spd);
		}
	}

	sys_dimm_info->num_slot = di;

	return 0;
}

/* OCM and device IO mappings */
static void xgene_ocm_io_tlb_entry_add(void)
{
	mmap_add_region(0, 0, OCM_BASE, MT_DEVICE | MT_RW | MT_SECURE);
	mmap_add_region(OCM_BASE, OCM_BASE, OCM_ROM_SIZE,
			MT_MEMORY | MT_RO | MT_SECURE);
	mmap_add_region(TZRAM_BASE, TZRAM_BASE, OCM_SIZE - OCM_ROM_SIZE,
			MT_MEMORY | MT_RW | MT_SECURE);
	mmap_add_region(OCM_BASE + OCM_SIZE, OCM_BASE + OCM_SIZE, 0x62F80000,
			MT_DEVICE | MT_RW | MT_SECURE);
}

/*
 * Enable MMU for ECC init
 * This TLB table is created specifically for DDR initialization.
 * It will be cleanly removed after DDR initialization finishes
 * by ddr_xgene_disable_mmu_el3()
 */
int ddr_tlb_map(void *p, unsigned int flag)
{
	struct apm_memc *ddr = (struct apm_memc *)p;
	u32 i;

	ddr_xgene_disable_mmu_el3();

	xgene_ocm_io_tlb_entry_add();

	/* Update system dram information */
	sys_mem_info.nr_regions = ddr->memspace.num_mem_regions;
	for (i = 0; i < sys_mem_info.nr_regions; i++) {
		sys_mem_info.base[i] = ddr->memspace.str_addr[i];
		sys_mem_info.size[i] = (ddr->memspace.end_addr[i]
				- ddr->memspace.str_addr[i]) + 1;
		ddr_pr("DRAM Region[%d]: 0x%010llX - 0x%010llX\n", i,
			ddr->memspace.str_addr[i], ddr->memspace.end_addr[i]);
		/* Add memory map region */
		mmap_add_region(sys_mem_info.base[i], sys_mem_info.base[i],
				sys_mem_info.size[i],
				MT_NON_CACHEABLE | MT_RW | MT_NS);
		sys_mem_info.total_size += sys_mem_info.size[i];
	}
	init_xlat_tables();

	enable_mmu_el3(0);

	return 0;
}

/*
 * This TLB table is created specifically for DDR PHY training.
 * It helps to make PHY training much faster
 */
int pre_ddr_tlb_map(void *p, unsigned int flag)
{
	ddr_xgene_disable_mmu_el3();
	xgene_ocm_io_tlb_entry_add();
	init_xlat_tables();
	enable_mmu_el3(0);

	return 0;
}

static void ddr_skylark_board_cfg(struct apm_memc *pmemc)
{
	unsigned int i;

	/* Setup SPD I2C configuration */
	for_each_mcu(i) {
		pmemc->mcu[i].spd_mux = 0x70;
		if (i < 4) {
			pmemc->mcu[i].spd_mux_addr = 1; /* mux port 0 */
			pmemc->mcu[i].spd_addr[0] = 0x50 + (i * 2);
			pmemc->mcu[i].spd_addr[1] = 0x51 + (i * 2);
		} else {
			pmemc->mcu[i].spd_mux_addr = 2; /* mux port 1 */
			pmemc->mcu[i].spd_addr[0] = 0x50 + ((i - 4) * 2);
			pmemc->mcu[i].spd_addr[1] = 0x51 + ((i - 4) * 2);
		}
	}
}


int ddr_show_ecc_progress(void *ptr, unsigned int flag)
{
	switch (flag){
	case 0:
		ddr_pr("ECC init ................");
		ddr_pr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		break;
	case 1:
		ddr_pr("*");
		break;
	case 2:
		ddr_pr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		ddr_pr("................");
		ddr_pr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		break;
	case 3:
		ddr_pr("                ");
		ddr_pr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		ddr_pr("\b\b\b\b\b\b\b\b\b");
		break;
	default:
		break;
	}

	return 0;
}

/* Set User Defined Params here */
int ddr_ud_param(void *ptr, unsigned int flag)
{
	struct apm_memc *pmemc = (struct apm_memc *)ptr;
	int iia, value;

	ddr_verbose("DRAM: Skylark Eagle board detected.\n");
	ddr_skylark_board_cfg(pmemc);

	/* Spd content passed into ddrlib */
	pmemc->memc_ud.ud_spd_get = ddr_get_spd;
	pmemc->p_cache_flush = ddr_apm_dcache_flush;

	pmemc->p_show_ecc_progress = ddr_show_ecc_progress;
	pmemc->p_memsys_tlb_map = ddr_tlb_map;
	pmemc->p_pre_ddr_tlb_map = pre_ddr_tlb_map;
	pmemc->p_progress_bar = ddr_progress_bar;

	/* User to change DDR logging messages level */
	ddrlog_verbose = ddr_get_env(NV_DDR_LOG_LEVEL, DDR_LOG_LEVEL_WARNING);

	/* User Parameters for MEMC */
	value = ddr_get_env(NV_MCU_MASK, MCU_ENABLE_MASK);
	pmemc->memc_ud.ud_mcu_enable_mask =
		is_ddr_param_valid(DDR_MCU_MASK, value)? value : MCU_ENABLE_MASK;
	pmemc->memc_ud.ud_hash_en =
		ddr_get_env(NV_DDR_HASH_EN, DEFAULT_HASH_EN);
	pmemc->memc_ud.ud_interleaving_en =
		ddr_get_env(NV_DDR_INTLV_EN, DEFAULT_INTERLEAVING_EN);

	/* User Parameters for MCB */
	pmemc->mcb[0].mcb_ud.ud_spec_read =
		ddr_get_env(NV_DDR_SPECULATIVE_RD, MCU_SPEC_READ);
	pmemc->mcb[1].mcb_ud.ud_spec_read =
		ddr_get_env(NV_DDR_SPECULATIVE_RD, MCU_SPEC_READ);

	/* User Parameters for MCU */
	for_each_mcu(iia) {
	#if XGENE_VHP || XGENE_EMU
		pmemc->mcu[iia].mcu_ud.ud_ddr_mimicspd = 1;
	#else
		pmemc->mcu[iia].mcu_ud.ud_ddr_mimicspd = DEFAULT_MIMICSPD;
	#endif
	}

	value = ddr_get_env(NV_DRAM_SPEED, DEFAULT_DDR_SPEED);
	value = is_ddr_param_valid(DDR_SPEED, value)? value : DEFAULT_DDR_SPEED;
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_speed_grade = value;
	}

	value = ddr_get_env(NV_DDR_ADDR_DECODE, DEFAULT_ADDR_DECODE);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_addr_decode = value;
	}

	value = ddr_get_env(NV_DDR_STRIPE_DECODE, DEFAULT_STRIPE_DECODE);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_stripe_decode = value;
	}

	value = ddr_get_env(NV_DRAM_ECC_MODE, DEFAULT_ECC_MODE);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_ecc_enable = value;
	}

	value = ddr_get_env(NV_DDR_CRC_MODE, DEFAULT_CRC);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_crc_enable = value;
	}

	value = ddr_get_env(NV_DDR_RD_DBI_EN, DEFAULT_RD_DBI);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rd_dbi_enable = value;
	}

	value = ddr_get_env(NV_DDR_WR_DBI_EN, DEFAULT_WR_DBI);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wr_dbi_enable = value;
	}

	value = ddr_get_env(NV_DDR_GEARDOWN_EN, DEFAULT_GEARDOWN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_geardown_enable = value;
	}

	value = ddr_get_env(NV_DDR_BANK_HASH_EN, DEFAULT_BANK_HASH);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_bank_hash_enable = value;
	}

	value = ddr_get_env(NV_DDR_REFRESH_GRANULARITY, DEFAULT_REFRESH_GRANULARITY);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_refresh_granularity = value;
	}

	value = ddr_get_env(NV_DDR_PMU_EN, DEFAULT_PMU_ENABLE);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_pmu_enable = value;
	}

	value = ddr_get_env(NV_DDR_WR_PREAMBLE_2T_MODE, WR_PREAMBLE_2T_MODE);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_write_preamble_2t = value;
	}

	/* TODO: Add s/w vref train en/dis, vref range and vref value as NV-PARAMS */

	value = ddr_get_env(NV_DDR_DESKEW_EN, DEFAULT_DESKEW_EN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_deskew_en = value;
	}

	value = ddr_get_env(NV_DDR_RTR_S_MARGIN, DEFAULT_RTR_S_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtr_s_margin = value;
	}

	value = ddr_get_env(NV_DDR_RTR_L_MARGIN, DEFAULT_RTR_L_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtr_l_margin = value;
	}

	value = ddr_get_env(NV_DDR_RTR_CS_MARGIN, DEFAULT_RTR_CS_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtr_cs_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTW_S_MARGIN, DEFAULT_WTW_S_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtw_s_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTW_L_MARGIN, DEFAULT_WTW_L_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtw_l_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTW_CS_MARGIN, DEFAULT_WTW_CS_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtw_cs_margin = value;
	}

	value = ddr_get_env(NV_DDR_RTW_S_MARGIN, DEFAULT_RTW_S_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtw_s_margin = value;
	}

	value = ddr_get_env(NV_DDR_RTW_L_MARGIN, DEFAULT_RTW_L_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtw_l_margin = value;
	}

	value = ddr_get_env(NV_DDR_RTW_CS_MARGIN, DEFAULT_RTW_CS_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_rtw_cs_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTR_S_MARGIN, DEFAULT_WTR_S_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtr_s_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTR_L_MARGIN, DEFAULT_WTR_L_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtr_l_margin = value;
	}

	value = ddr_get_env(NV_DDR_WTR_CS_MARGIN, DEFAULT_WTR_CS_MARGIN);
	for_each_mcu(iia) {
		pmemc->mcu[iia].mcu_ud.ud_wtr_cs_margin = value;
	}

	return 0;
}

static char *ddr_get_dimm_type(package_type_e type)
{
	static char dimm_type_s[20];

	switch (type) {
	case RDIMM:
		sprintf(dimm_type_s, "%s", "RDIMM");
		break;
	case UDIMM:
		sprintf(dimm_type_s, "%s", "UDIMM");
		break;
	case SODIMM:
		sprintf(dimm_type_s, "%s", "SODIMM");
		break;
	case LRDIMM:
		sprintf(dimm_type_s, "%s", "LRDIMM");
		break;
	default:
		sprintf(dimm_type_s, "%s", "Unknown DIMM");
		break;
	}
	return dimm_type_s;
}


static char *ddr_get_device_type(unsigned int device)
{
	static char dev_type_s[4];

	switch (device) {
	case 0:
		sprintf(dev_type_s, "%s", "4");
		break;
	case 1:
		sprintf(dev_type_s, "%s", "8");
		break;
	case 2:
		sprintf(dev_type_s, "%s", "16");
		break;
	default:
		sprintf(dev_type_s, "%s", "??");
		break;
	}
	return dev_type_s;
}

static void ddr_show_info(struct apm_memc *memc)
{
	generic_spd_eeprom_t *spd;
	struct apm_mcu *mcu;
	int i, j, no_of_ranks, ecc_en = 1;
	unsigned long long dimm_size, total_capacity;
	char s[2][80];
	char s_mpart[17];
	char *tmp;

        total_capacity = 0;
	for_each_mcu(i) {
		mcu = &memc->mcu[i];
		if (!mcu->enabled)
			continue;

		/* for each slot */
		for (j = 0; j <= 1; j++) {
			if (!(mcu->activeslots & (1 << j)))
				continue;

			ecc_en &= mcu->ddr_info.ecc_en ? 1 : 0;
			spd = &mcu->spd_info[j].spd_info;
			no_of_ranks = compute_no_ranks(spd);
			tmp = ddr_get_dimm_type(mcu->ddr_info.package_type);
			sprintf(s[j], "%s", tmp);
			tmp = ddr_get_device_type(mcu->ddr_info.device_type);
			dimm_size = no_of_ranks * compute_rank_capacity(spd);
			total_capacity += dimm_size;

			sprintf(s[j] + strlen(s[j]), "[ID:%02x%02x]",
				spd->dmid_msb, spd->dmid_lsb);
			/* Print the first 16 characters of serial numbers only */
			s_mpart[16] = '\0';
			memcpy(s_mpart, spd->mpart, 16);
			sprintf(s[j] + strlen(s[j]), " %s %dGB %d rank(s) x%s",
				s_mpart, (int) (dimm_size / 0x40000000U),
				no_of_ranks, tmp);
			ddr_pr("  MCU[%d]-Slot[%d]: %s\n", i, j, s[j]);
		}
	}

	sys_mem_info.cur_speed =
			ddr_speed_display(memc->mcu[0].ddr_info.t_ck_ps);
	ddr_pr("DRAM: %dGB DDR4 %d %s\n",
		(int)(total_capacity / 0x40000000U),
		sys_mem_info.cur_speed,
		(ecc_en)? "ECC": "");
}

DEFINE_SYSOP_TYPE_FUNC(ic, iallu)

static void ddr_xgene_disable_mmu_el3(void)
{
	disable_mmu_el3();
	/* Invalidate TLBs at the current exception level */
	tlbialle3();
	dsb();
	isb();
	/* I-Cache flush */
	iciallu();
	/* D-Cache flush */
	dcsw_op_all(DCCISW);
	mmap_xlat_reset();
}

void ddr_xgene_hw_init(void)
{
	unsigned int err;

	ddr_pr("DRAM FW v%s\n", DDRLIB_VER);
	ddr_progress_console_init();
	ddr_progress_bar(0);
	/* Init the apm_memc structure */
	memset(&memc, 0, sizeof(struct apm_memc));

        /* SMPro Read/Write functions */
	memc.p_smpro_read  = plat_smpro_read;
	memc.p_smpro_write = plat_smpro_write;

	/* Setup functions for user/board settings */
	memc.p_user_param_overrides = ddr_ud_param;

        /* Board Setup */
	memc.p_board_setup = mcu_board_specific_settings;

	/* Setup Data Struct & functions */
	err = ddr_sys_setup(&memc, 0);
	if (err)
		memc.p_handle_fatal_err(&memc, 0);

	/* Run initialization */
	err = ddr_sys_init(&memc, 0);
	ddr_progress_bar(100);
	ddr_show_info(&memc);

	if (err)
		memc.p_handle_fatal_err(&memc, 0);

	ddr_update_dimm_list(&memc);

	ddr_xgene_disable_mmu_el3();

#if defined(DEBUG_POST_INIT_REG_DUMP)
	ddr_dump_reg(&memc);
#endif
}
