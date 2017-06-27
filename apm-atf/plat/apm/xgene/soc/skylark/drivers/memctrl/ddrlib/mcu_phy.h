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

#ifndef _MCU_PHY_H
#define _MCU_PHY_H

/* PHY Defines Input */
#define IO_INPUT_ENABLE_DELAY   900
#define CDNPHY_NUM_SLICES 	9

typedef enum phylpbkcnt {
        LPBKCNT_FREERUN = 0,
        LPBKCNT_1024 = 1,
        LPBKCNT_8192 = 2,
        LPBKCNT_64k = 3,
        EndLPBKCNT_Count = 4
} phylpbkcnt_e;

typedef enum phylpbkdtype {
        LPBKDTYPE_LFSR = 0,
        LPBKDTYPE_CLKPTRN = 1,
        EndLPBKDTYPE_Count = 2
} phylpbkdtype_e;

typedef enum phylpbkmux {
        LPBKMUX_INT = 0,
        LPBKMUX_EXT = 1,
        EndLPBKMUX_Count = 2
} phylpbkmux_e;

/* Pad Drive */
typedef struct phy_pad_drv_term {
        unsigned int phy_pad_fdbk_drive;
        unsigned int phy_pad_x4_fdbk_drive;
	unsigned int phy_pad_data_drive;
        unsigned int phy_pad_dqs_drive;
        unsigned int phy_pad_addr_drive;
        unsigned int phy_pad_clk_drive;
	unsigned int phy_pad_par_drive;
        unsigned int phy_pad_err_drive;
        unsigned int phy_pad_atb_ctrl;
	unsigned int phy_pad_data_term;
        unsigned int phy_pad_dqs_term;
        unsigned int phy_pad_addr_term;
	unsigned int phy_pad_clk_term;
} __attribute__((aligned(8))) phy_pad_drv_term_t;

/* IE/OE Slice */
typedef struct phy_dq_dqs_ie_oe_slice {
        unsigned int dq_end_full_cycle;
	unsigned int dq_end_half_cycle;
	unsigned int dq_start_full_cycle;
	unsigned int dq_start_half_cycle;
        unsigned int dqs_end_full_cycle;
	unsigned int dqs_end_half_cycle;
	unsigned int dqs_start_full_cycle;
	unsigned int dqs_start_half_cycle;
} __attribute__((aligned(8))) phy_dq_dqs_ie_oe_slice_t;

typedef struct phy_dq_dqs_ie_oe_timing_all{
	phy_dq_dqs_ie_oe_slice_t  sliced[CDNPHY_NUM_SLICES];
} __attribute__((aligned(8))) phy_dq_dqs_ie_oe_timing_all_t;


typedef struct phy_master_delay_lock_vals {
        unsigned int phy_full_d[7];
	unsigned int phy_qtr_d[7];
} __attribute__((aligned(8))) phy_master_delay_lock_vals_t;

typedef struct phy_all_mstr_dly_lock_vals{
	phy_master_delay_lock_vals_t  sliced[CDNPHY_NUM_SLICES];
} __attribute__((aligned(8))) phy_all_mstr_dly_lock_t;

/* used by Wrdeskew/Rdeskew */
typedef struct phy_slice_train_res{
        /*  [8] - (8 parameters per slice, one for each DQZ)  */
	unsigned int   phy_clk_wrdm_slave_delay;
	unsigned int   phy_clk_wrdqZ_slave_delay[8];
	unsigned int   phy_clk_wrdqs_slave_delay;
	unsigned int   phy_x4_clk_wrdqs_slave_delay;
	unsigned int   phy_write_path_lat_add;
	unsigned int   phy_x4_write_path_lat_add;
	unsigned int   phy_rddqs_dm_fall_slave_delay;
	unsigned int   phy_rddqs_dm_rise_slave_delay;
	unsigned int   phy_rddqs_dqZ_fall_slave_delay[8];
	unsigned int   phy_rddqs_dqZ_rise_slave_delay[8];
	unsigned int   phy_rddqZ_slave_delay[8];
	unsigned int   phy_rddqs_gate_slave_delay;
	unsigned int   phy_x4_rddqs_gate_slave_delay;
	unsigned int   phy_rddqs_latency_adjust;
	unsigned int   phy_x4_rddqs_latency_adjust;
	unsigned int   phy_wrlvl_delay_early_threshold;
	unsigned int   phy_wrlvl_early_force;
	unsigned int   phy_wrlvl_delay_period_threshold;
	unsigned int   phy_x4_wrlvl_early_force;
	unsigned int   phy_x4_wrlvl_delay_period_threshold;
	unsigned int   phy_x4_wrlvl_delay_early_threshold;
} __attribute__((aligned(8))) phy_slice_train_res_t;

typedef struct phy_rnk_train_res{
	unsigned int            ranknum;
	phy_slice_train_res_t   sliced[CDNPHY_NUM_SLICES];
} __attribute__((aligned(8))) phy_rnk_train_res_t;

typedef struct phy_slice_train_obs{
	unsigned int   phy_wrdqs_base_slv_dly_enc_obs;
	unsigned int   phy_wrdq_base_slv_dly_enc_obs;
	unsigned int   phy_wr_adder_slv_dly_enc_obs;
	unsigned int   phy_rddq_slv_dly_enc_obs;
	unsigned int   phy_rddqs_base_slv_dly_enc_obs;
	unsigned int   phy_rddqs_gate_slv_dly_enc_obs;
	unsigned int   phy_rddqs_dq_rise_adder_slv_dly_enc_obs;
	unsigned int   phy_rddqs_dq_fall_adder_slv_dly_enc_obs;
	unsigned int   phy_gtlvl_hard0_delay_obs;
	unsigned int   phy_gtlvl_hard1_delay_obs;
	unsigned int   phy_gtlvl_status_obs;
	unsigned int   phy_rdlvl_rddqs_dq_le_dly_obs;
	unsigned int   phy_rdlvl_rddqs_dq_te_dly_obs;
	unsigned int   phy_rdlvl_status_obs;
	unsigned int   phy_wrlvl_hard0_delay_obs;
	unsigned int   phy_wrlvl_hard1_delay_obs;
	unsigned int   phy_wrlvl_status_obs;
	unsigned int   phy_wrlvl_err_obs;
} __attribute__((aligned(8))) phy_slice_train_obs_t;

typedef struct phy_rnk_train_obs{
	unsigned int            ranknum;
	phy_slice_train_obs_t   sliced[CDNPHY_NUM_SLICES];
} __attribute__((aligned(8))) phy_rnk_train_obs_t;

/******************************************************************************/
/*  PHY CSR Defaults                                                          */
/******************************************************************************/
#define PHY_IE_MODE                       0x00
#define PHY_PER_RANK_CS_MAP               0xFF
/* 0x05 < DDR_2400, 0x06 >= DDR_2400 */
#define PHY_RPTR_UPDATE                   0x06
/* DDR4 - {0x4, 0x5}, DDR3 - 0x0 */
#define PHY_PAD_CAL_MODE                  0x4
#define PHY_TWO_CYC_PREAMBLE              0x0

/* PLL CTRL */
#define PHY_PLL_CTRL                      0xA2
#define PHY_PLL_WAIT                      0x64

/*  Phy timing tsel (required always) */
#define PHY_DQ_IE_TIMING                  0x80
#define PHY_DQ_OE_TIMING                  0x42
#define PHY_DQ_TSEL_WR_TIMING             0x42
#define PHY_DQ_TSEL_RD_TIMING             0x40
/* Bug 57673 */
#define PHY_DQ_TSEL_ENABLE                0x5
#define PHY_DQ_TSEL_SELECT                0x99FF99 /* WR - 34 ohms, RD - 80 ohms */
#define PHY_DQS_IE_TIMING                 0x80
#define PHY_DQS_OE_TIMING                 0x41
#define PHY_DQS_TSEL_WR_TIMING            0x41
#define PHY_DQS_TSEL_RD_TIMING            0x40
#define PHY_DQS_TSEL_ENABLE               0x1
#define PHY_DQS_TSEL_SELECT               0x99FF99 /* WR - 34 ohms, RD - 80 ohms */

/* Phy drive and termination (required always) */
#define PHY_PAD_FDBK_DRIVE                0x80FF
#define PHY_PAD_ADDR_DRIVE                0x0FF
#define PHY_PAD_CLK_DRIVE                 0x0FF
#define PHY_PAD_PAR_DRIVE                 0x0FF
#define PHY_PAD_ERR_DRIVE                 0x0FF
#define PHY_PAD_DQS_DRIVE                 0xAB
#define PHY_PAD_DATA_DRIVE                0xAB /* RX_DQ_EN set only for X8 devices */
#define PHY_PAD_ATB_CTRL                  0x0

/* Master delay (required always) */
#define PHY_MASTER_DELAY_START            0x10
#define PHY_MASTER_DELAY_STEP             0x08
#define PHY_MASTER_DELAY_WAIT             0x42
#define PHY_ADRCTL_MASTER_DELAY_START     0x10
#define PHY_ADRCTL_MASTER_DELAY_STEP      0x08
#define PHY_ADRCTL_MASTER_DELAY_WAIT      0x42

/*  Phy leveling (required for PHY training) */
#define PHY_WRLVL_DLY_STEP                0x8
#define PHY_WRLVL_CAPTURE_CNT             0xF
#define PHY_WRLVL_RESP_WAIT_CNT           0xF
#define PHY_WRLVL_UPDT_WAIT_CNT           0xF
#define PHY_WRLVL_DELAY_EARLY_THRESHOLD   0x200
#define PHY_WRLVL_EARLY_FORCE_ZERO        0x0
#define PHY_WRLVL_DELAY_PERIOD_THRESHOLD  0x0

#define PHY_GTLVL_CAPTURE_CNT             0xF
#define PHY_GTLVL_UPDT_WAIT_CNT           0xA
#define PHY_GTLVL_RESP_WAIT_CNT           0xA
#define PHY_GTLVL_DLY_STEP                0xC
#define PHY_GTLVL_FINAL_STEP              0x100
#define PHY_GTLVL_BACK_STEP               0x180
#define PHY_GTLVL_LAT_ADJ_START           0x0
#define PHY_GTLVL_RDDQS_SLV_DLY_START     0x0

#define PHY_RDLVL_CAPTURE_CNT             0x8
#define PHY_RDLVL_UPDT_WAIT_CNT           0xA
/* Linear: 0x0 Inside-Out: 0x1 */
#define PHY_RDLVL_OP_MODE                 0x0
#define PHY_RDLVL_DLY_STEP                0x4
#define PHY_RDLVL_DATA_MASK               0x00
/* Inside-Out starting at 0x90 (per CS) */
#define PHY_RDLVL_RDDQS_DQ_SLV_DLY_START  0x0
#define PHY_RDDQS_GATE_SLAVE_DELAY        0x120
 /* DDR4 only */
#define PHY_RDLVL_DATA_SWIZZLE            0x32103210

/* Phy Periodic Calibration (optional/post-training) */
#define PHY_CAL_CLK_SELECT                0x7
#define PHY_CAL_SAMPLE_WAIT               0xC
/* Enable Runtime Auto I/O cal */
#define PHY_CAL_EN_RUNTIME                0x1
/* 16M cycles */
#define PHY_CAL_INT_BASE                  0x2
/* 0: Every base counts */
#define PHY_CAL_INT_COUNT                 0x01

/* RX CAL Controls */
#define PHY_RX_CAL_START                  0x0
#define PHY_RX_CAL_SAMPLE_WAIT            0x20

/* Phy ZQ (Used to overwrite Auto-ZQ Calibration values) */
#define PHY_PAD_DATA_TERM                 0x000410
#define PHY_PAD_DQS_TERM                  0x000410
#define PHY_PAD_ADDR_TERM                 0x000410
#define PHY_PAD_CLK_TERM                  0x000410
#define PHY_PAD_FDBK_TERM                 0x004410

/* ADDR/CMD */
#define ADCMD_GRP_SHIFT                   0
#define ADCMD_GRP_SLAVE_DELAY_DEFAULT     0x100
#define ADCMD_GRP_SLAVE_DELAY_HIGHLOAD    0x1A0

/* Slave Delay (per CS) (required if no PHY training) */
#define PHY_CLK_WRDQx_SLAVE_DELAY         0x260
#define PHY_CLK_WRDQS_SLAVE_DELAY         0x0
#define PHY_CLK_WRDM_SLAVE_DELAY          0x260
#define PHY_RDDQx_SLAVE_DELAY             0x0
#define PHY_RDDQS_DQx_RISE_SLAVE_DELAY    0x0 // 0x80U
#define PHY_RDDQS_DQx_FALL_SLAVE_DELAY    0x0 // 0x80U
#define PHY_RDDQS_DM_RISE_SLAVE_DELAY     0x0 // 0x90U
#define PHY_RDDQS_DM_FALL_SLAVE_DELAY     0x0 // 0x90U

/*
 * Cadence bug ref:_00Dd0c1Z9._500d0Hct9a:ref]
 * Bug 43313 - MCU PHY: Write DQ slave delay lines must be > 0x1C0
 * program Wr DQ Slave delay greater than 0x1C0U
 */
#define PHY_CLK_WRDQx_SLAVE_DELAY_MIN_LIMIT 0x1D0
#define PHY_CLK_WRDQx_SLAVE_DELAY_MAX_LIMIT 0x360
#define PHY_CLK_RDDQx_SLAVE_DELAY_MIN_LIMIT 0x10
#define PHY_CLK_RDDQx_SLAVE_DELAY_MAX_LIMIT 0x180

/* Phy latency defaults (optional) */
#define PHY_WRITE_PATH_LAT_ADD            0x0
#define PHY_RDDQS_LATENCY_ADJUST          0x1
#define PHY_GATE_ERROR_DELAY_SELECT       0x7
#define PHY_RDDATA_EN_IE_DLY              0x0

#define PHY_SLICE_EN_MASK                 0x3FFFF
#define PHY_SLICE_NOECC_MASK              0x0FFFF
#define PHY_SLICE_OFFSET                  512


/* RX Calibration Obs Index */
#define PHY_RX_CAL_OBS_DQ0_BIT           0
#define PHY_RX_CAL_OBS_DQ1_BIT           1
#define PHY_RX_CAL_OBS_DQ2_BIT           2
#define PHY_RX_CAL_OBS_DQ3_BIT           3
#define PHY_RX_CAL_OBS_DQ4_BIT           4
#define PHY_RX_CAL_OBS_DQ5_BIT           5
#define PHY_RX_CAL_OBS_DQ6_BIT           6
#define PHY_RX_CAL_OBS_DQ7_BIT           7
#define PHY_RX_CAL_OBS_DM_BIT            8
#define PHY_RX_CAL_OBS_DQS_BIT           9
#define PHY_RX_CAL_OBS_FDBK_BIT          10
#define PHY_RX_CAL_OBS_X4_FDBK_BIT       11

/* RX Calibration Obs Index */
#define PHY_RX_CAL_OBS_DQ0_BIT           0
#define PHY_RX_CAL_OBS_DQ1_BIT           1
#define PHY_RX_CAL_OBS_DQ2_BIT           2
#define PHY_RX_CAL_OBS_DQ3_BIT           3
#define PHY_RX_CAL_OBS_DQ4_BIT           4
#define PHY_RX_CAL_OBS_DQ5_BIT           5
#define PHY_RX_CAL_OBS_DQ6_BIT           6
#define PHY_RX_CAL_OBS_DQ7_BIT           7
#define PHY_RX_CAL_OBS_DM_BIT            8
#define PHY_RX_CAL_OBS_DQS_BIT           9
#define PHY_RX_CAL_OBS_FDBK_BIT          10
#define PHY_RX_CAL_OBS_X4_FDBK_BIT       11

#define PHY_RX_CAL_MIN_VALUE             0x0
#define PHY_RX_CAL_MAX_VALUE             0x3F

#define PHY_RX_CAL_START_VALUE           (0x1F << 6 | 0x1F) /* Both Up and Down */

#include "ddr_phy_defines.h"

#endif /* _MCU_PHY_H */
