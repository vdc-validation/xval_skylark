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

#ifndef __APM_DDR_SDRAM_H__
#define __APM_DDR_SDRAM_H__

#define DDRLIB_VER     "170531"

/* ARM Trusted Firmware */
#include <debug.h>
#include <delay_timer.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include "ddr_spd.h"

#define ddr_pr(...)		printf(__VA_ARGS__)

typedef __uint64_t u64;
typedef __uint32_t u32;

#define DDR_LOG_LEVEL_NONE		0
#define DDR_LOG_LEVEL_ERROR		10
#define DDR_LOG_LEVEL_NOTICE		20
#define DDR_LOG_LEVEL_WARNING		30
#define DDR_LOG_LEVEL_INFO		40
#define DDR_LOG_LEVEL_VERBOSE		50
#define DDR_LOG_LEVEL_DEBUG		60

/* This variable should be defined in the DDR driver file */
extern int ddrlog_verbose;

#ifndef DDR_LOG_LEVEL
#define DDR_LOG_LEVEL	DDR_LOG_LEVEL_WARNING
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_ERROR
# define ddr_err(...)		ddr_pr("ERR: " __VA_ARGS__)
#else
# define ddr_err(...)
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_NOTICE
# define ddr_notice(...)	ddr_pr("NOTICE: " __VA_ARGS__)
#else
# define ddr_notice(...)
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_WARNING
# define ddr_warn(...)		ddr_pr("WARN: " __VA_ARGS__)
#else
# define ddr_warn(...)
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_INFO
# define ddr_info(...)						\
	do {							\
		if (ddrlog_verbose >= DDR_LOG_LEVEL_INFO)	\
			ddr_pr(__VA_ARGS__);			\
	} while(0)
#else
# define ddr_info(...)
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_VERBOSE
# define ddr_verbose(...)					\
	do {							\
		if (ddrlog_verbose >= DDR_LOG_LEVEL_VERBOSE)	\
			ddr_pr(__VA_ARGS__);			\
	} while(0)
#else
# define ddr_verbose(...)
#endif

#if DDR_LOG_LEVEL >= DDR_LOG_LEVEL_DEBUG
# define ddr_debug(...)					\
	do {							\
		if (ddrlog_verbose >= DDR_LOG_LEVEL_DEBUG)	\
			ddr_pr(__VA_ARGS__);			\
	} while(0)
#else
# define ddr_debug(...)
#endif

/******************************************************************************/

#ifndef NULL
#define NULL 0
#endif

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

/******************************************************************************/

/* ddr_spd.h */
#undef DDR3_MODE
typedef spd_eeprom_t generic_spd_eeprom_t;

#define NR_MEM_REGIONS	4

#define CONFIG_SYS_NUM_MCB		2
#define CONFIG_SYS_NUM_DDR_CTLRS	8
#define CONFIG_SYS_NUM_MCU_PER_MCB	4

#define CONFIG_SYS_DIMM_SLOTS_PER_CTLR	2
#define CONFIG_SYS_NO_OF_DIMMS \
	(CONFIG_SYS_NUM_DDR_CTLRS * CONFIG_SYS_DIMM_SLOTS_PER_CTLR)

#define for_each_mcu(i)	\
	for (i = 0; i < CONFIG_SYS_NUM_DDR_CTLRS; i++)

/******************************************************************************/
/*      Typedef Enums etc                                                     */
/******************************************************************************/
typedef enum ddrtype {
	DDR4 = 0,
	DDR3 = 1
} ddr_type_e;

typedef enum packagetype {
	RDIMM  = 1,
	UDIMM  = 2,
	SODIMM = 3,
	LRDIMM = 4
} package_type_e;

typedef enum ddr3odtrttnom {
	RttNomZero = 0,
	RttNom60ohm = 1,
	RttNom120ohm = 2,
	RttNom40ohm = 3,
	RttNom20ohm = 4,
	RttNom30ohm = 5,
	RttNomRsvd0 = 6,
	RttNomRsvd1 = 7,
	EndRttNomCount = 8
} ddr3odtrttnom_e;

typedef enum ddr3odtrttwr {
	RttWrOff = 0,
	RttWr60ohm = 1,
	RttWr120ohm = 2,
	RttWrRsvd = 3,
	EndRttWrCount = 4
} ddr3odtrttwr_e;

typedef enum phywrcalopt {
	AWRCALBISTCHK = 0,
	BWRCALESTDLY,
	ENDWRCALCOUNT
} phywrcalopt_e;

typedef enum mcubistop {
	NOMCUBIST = 0,
	ENDMCUBISTCOUNT
} mcubistop_e;

typedef enum mcutrafficenopt {
	MCUNORMALTRAFFIC = 0,
	MCUENDLESSBISTTRAFFIC = 1,
	ENDMCUTRAFFICCOUNT
} mcutrafficenopt_e;

typedef enum phylpbkopt {
	NOPHYLPBK = 0,
	PHYLPBK_INTLSFR1K = 1,
	PHYLPBK_INTLSFR8K = 2,
	PHYLPBK_INTLSFR64K = 3,
	PHYLPBK_INTCLKP1K = 4,
	PHYLPBK_INTCLKP8K = 5,
	PHYLPBK_INTCLKP64K = 6,
	PHYLPBK_EXTLSFR1K = 7,
	PHYLPBK_EXTLSFR8K = 8,
	PHYLPBK_EXTLSFR64K = 9,
	PHYLPBK_EXTCLKP1K = 10,
	PHYLPBK_EXTCLKP8K = 11,
	PHYLPBK_EXTCLKP64K = 12,
	ENDPRBSLOOPBACKCOUNT = 13
} phylpbkopt_e;

#define MAX_SPEED_CFG		7
#define MAX_SIZE		5
#define MAX_CONFIG		3

/* Default memory config */
#define DEF_SPEED_GRD		MHZ400
#define DEF_MEM_SIZE		MB4096
#define DEF_MEM_CONFIG		CFG512Mx8

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

#define ONE_BILLION     1000000000
#define SIZE256MB       0x10000000
#define MPIC_FRR        0x1000

/* MCU User-defined Parameters */
typedef struct apm_mcu_udparam {
	/* Mcu User Params */
	/* User Defined SPD etc */
	int           ud_ddr_mimicspd;
	int           ud_mimic_activeslots;
	int           ud_rc11_dimmvdd;

	/* Mcu training  User Params */
	int           ud_singlerank_train_mode;
	int           ud_physlice_en;

	/* User Defined overrides for PLL params */
	int           ud_pll_force_en;
	int           ud_pllctl_fbdivc;
	int           ud_pllctl_outdiv2;
	int           ud_pllctl_outdiv3;
	/* If none zero, used by DDRlib */
	int           ud_speed_grade;

	/* User Defined overrides for MCU Configuration */
	int           ud_en2tmode;
	int           ud_addr_decode;
	int           ud_stripe_decode;
	/* 0-Disbale, 1-SECDED, 2-ChipKill (RS Encoding) */
	int           ud_ecc_enable;
	/* 0- CRC Disable, 1 - CRC Enabled */
	int           ud_crc_enable;
	int           ud_rd_dbi_enable;
	int           ud_wr_dbi_enable;
	int           ud_geardown_enable;
	int           ud_bank_hash_enable;
	int           ud_refresh_granularity;
	int           ud_pmu_enable;
	int           ud_write_preamble_2t;
	int           ud_deskew_en;
	int           ud_sw_rx_cal_en;

	/* RTT related controls */
	int           ud_rtt_wr;
	int           ud_rtt_nom_s0;
	int           ud_rtt_nom_s1;
	int           ud_rtt_park_s0;
	int           ud_rtt_park_s1;
	int           ud_rtt_nom_wrlvl_tgt;
	int           ud_rtt_nom_wrlvl_non_tgt;
	int           ud_mr1_dic;        /* MR1 D.I.C Driver Impedance Control */

	/* User Defined margin to Mcu timing params */
	int           ud_force_cl;
	int           ud_force_cwl;
	int           ud_rtr_s_margin;
	int           ud_rtr_l_margin;
	int           ud_rtr_cs_margin;
	int           ud_wtw_s_margin;
	int           ud_wtw_l_margin;
	int           ud_wtw_cs_margin;
	int           ud_rtw_s_margin;
	int           ud_rtw_l_margin;
	int           ud_rtw_cs_margin;
	int           ud_wtr_s_margin;
	int           ud_wtr_l_margin;
	int           ud_wtr_cs_margin;

	/* User Defined ODT timing */
	int           ud_rdodt_off_margin;
	int           ud_wrodt_off_margin;

	/* User Defined VREF training enable */
	int		ud_dram_vref_range;
	int		ud_dram_vref_value;
	int		ud_phy_pad_vref_range;
	int		ud_phy_pad_vref_value;
	int		ud_dram_vref_train_en;
	int		ud_dram_vref_finetune_en;
	int		ud_phy_vref_train_en;

	/* User Defined overrides for Phy Calib Mode(ZQ) */
	/* ase Interval
	 * b00 = 256 cycles
	 * b01 = 64K cycles
	 * b10 = 16M cycles  */
	int           ud_phy_cal_mode_on;
	int           ud_phy_cal_base_interval;
	int           ud_phy_cal_interval_count_0;

	/* User Defined Phy Drive & Term values */
	int           ud_phy_pad_fdbk_drive;
	int           ud_phy_pad_addr_drive;
	int           ud_phy_pad_clk_drive;
	int           ud_phy_pad_par_drive;
	int           ud_phy_pad_err_drive;
	int           ud_phy_pad_atb_ctrl;
	int           ud_phy_adctrl_slv_dly;
	int           ud_phy_dq_tsel_enable;
	int           ud_phy_dqs_tsel_enable;
	int           ud_phy_dq_tsel_select;
	int           ud_phy_dqs_tsel_select;
} __attribute__((aligned(8))) apm_mcu_udparam_t;

/* MCB user defined params structure */
typedef struct apm_mcb_udparam {
	int           ud_spec_read;
} __attribute__((aligned(8))) apm_mcb_udparam_t;

/* Memc user defined params structure */
typedef struct apm_memc_udparam {
	int           ud_mcu_enable_mask;
        int           ud_hash_en;
        int           ud_interleaving_en;

	/* Flow control */
	int           ud_ignore_init_cecc_err;
	int           ud_ignore_init_parity_err;
	int           ud_reset_on_phytrain_err;
	int           ud_bg_scrub;
	int           ud_page_mode;

	phylpbkopt_e  ud_phy_lpbk_option;
        /* Load spd content for each slots */
	int (*ud_spd_get) (void *, unsigned int);
} __attribute__((aligned(8))) apm_memc_udparam_t;


/* Pll Ctl */
typedef struct mcu_pllctl{
	unsigned int      pllctl_fbdivc;
	unsigned int      pllctl_outdiv2;
	unsigned int      pllctl_outdiv3;
} __attribute__((aligned(8))) mcu_pllctl_t;

typedef struct ddr_info {
        /* DDR3 vs DDR4 */
        unsigned int ddr_type;
        /* UDIMM, RDIMM, SODIMM, LRDIMM */
        package_type_e package_type;
        /* 3DS Stack height */
        unsigned int stack_high;
        /* x4 vs x8 */
        unsigned int device_type;
        /* One-hot encoding */
        unsigned int active_ranks;
        unsigned int physical_ranks;
        unsigned int odd_ranks;
        /* Per Slot */
        unsigned int max_ranks;
        unsigned int two_dpc_enable;
	unsigned int ecc_en;
        unsigned int t_ck_ps;
        unsigned int cas_latency;
        unsigned int cw_latency;
        unsigned int read_preamble;
        unsigned int write_preamble;
        unsigned int parity_latency;
        unsigned int write_latency;
        unsigned int read_latency;
        unsigned int addr_mirror;
        unsigned int addr_map_mode;
        unsigned int apm_addr_map_mode;
} __attribute__((aligned(8))) ddr_info_t;

typedef struct apm_mcu {
	unsigned int id;
	void *parent;
	unsigned int enabled;
	unsigned int mcb_id;

	/*  Private RB utility functions */
	unsigned long long phyl_rb_base;
	unsigned long long phyh_rb_base;
	unsigned long long dmcl_rb_base;
	unsigned long long dmch_rb_base;
	unsigned long long mcu_rb_base;
	unsigned int (*mcb_rd) (int, unsigned int);
	void (*mcb_wr) (int, unsigned int, unsigned int);
	unsigned int (*mcu_rd) (unsigned int, unsigned int);
	void (*mcu_wr) (unsigned int, unsigned int, unsigned int);
	unsigned int (*dmc_rd) (unsigned int, unsigned int);
	void (*dmc_wr) (unsigned int, unsigned int, unsigned int);
	unsigned int (*phy_rd) (unsigned int, unsigned int);
	void (*phy_wr) (unsigned int, unsigned int, unsigned int);

	/* PLL Ctl */
	mcu_pllctl_t mcu_pllctl;

        /* User Defined Parameters */
	apm_mcu_udparam_t mcu_ud;

        /* SPD Info */
	mcu_spd_eeprom_t spd_info[CONFIG_SYS_DIMM_SLOTS_PER_CTLR];

        /* MCU-DDR Info */
	ddr_info_t ddr_info;
	unsigned int activeslots;

	/* I2C SPD address */
	unsigned char spd_mux;	/* SPD mux I2C address */
	unsigned char spd_mux_addr;	/* SPD mux address for this MCU */
	unsigned char spd_addr[CONFIG_SYS_DIMM_SLOTS_PER_CTLR];
} __attribute__((aligned(8))) apm_mcu_t;

typedef struct apm_mcb {
	unsigned int dual_mcu;
	unsigned int mcu_intrlv;
	unsigned int mcu_intrlv_sel;

	apm_mcb_udparam_t mcb_ud;
} __attribute__((aligned(8))) apm_mcb_t;

typedef struct apm_mem_space {
	/* Up to four memory regions */
	unsigned int num_mem_regions;
	unsigned long long str_addr[NR_MEM_REGIONS];
	unsigned long long end_addr[NR_MEM_REGIONS];
} __attribute__((aligned(8))) apm_mem_space_t;


typedef struct apm_memc {
	unsigned int dual_mcb;
	unsigned int mcb_intrlv;
	unsigned int mcb_intrlv_sel;
	unsigned int full_addr;
	/* MCU base @0x8000_0000 for 32b mem support*/
	unsigned int is_mcu_base_2g;
	unsigned int mcu_mask;

	apm_memc_udparam_t    memc_ud;
	struct apm_mcb        mcb[CONFIG_SYS_NUM_MCB];
	struct apm_mcu        mcu[CONFIG_SYS_NUM_DDR_CTLRS];
	struct apm_mem_space  memspace;
	/* Private functions  */

	/* Public functions */
	/* User Over-rideable Sub-training function pointers */
	int (*p_user_param_overrides) (void *, unsigned int);
	void (*p_board_setup) (struct apm_memc *memc);
	int (*p_show_ecc_progress) (void *, unsigned int);
	int (*p_cache_flush) (void *, unsigned int);
	int (*p_memsys_addressmap) (void *, unsigned int);
	int (*p_memsys_tlb_map) (void *, unsigned int);
	int (*p_pre_ddr_tlb_map) (void *, unsigned int);
	void (*p_progress_bar) (unsigned int);
        /* Test &/or ecc-init all MC memory */
	int (*p_memsys_mtest_ecc_init) (struct apm_memc *memc, unsigned int);
	void (*p_handle_fatal_err) (void *, unsigned int);
        /* SMPro read & write */
	int (*p_smpro_read) (uint32_t reg, uint32_t *val, int urgent);
        int (*p_smpro_write) (uint32_t reg, uint32_t val, int urgent);
} __attribute__((aligned(8))) apm_memc_t;

/*****************************************************************/
/*     Public  functions                                         */
/*****************************************************************/
void ddr_sys_default_params_setup(struct apm_memc *memc);
int ddr_sys_setup(struct apm_memc *memc, unsigned int flag);
int ddr_sys_init(struct apm_memc *memc, unsigned int flag);

#endif
