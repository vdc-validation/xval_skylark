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
#ifndef _DDR_UD_H
#define _DDR_UD_H

/* MCB User defined */
#define MCU_SPEC_READ                 1

/* User Defined SPD etc */
#define MCU_ENABLE_MASK               0xFF
#define DEFAULT_MIMICSPD              0
#define DEFAULT_ACTIVE_SLOTS          0x1
#define DEFAULT_RC11_DIMMVDD          1500

/* Mcu training  User Params */
/* 0 - Multi Rank, 1 - Single Rank Training Mode */
#define DEFAULT_PHY_TRAIN_MODE        0
#define DEFAULT_PHY_SLICE_ON          0

/* User Defined overrides for PLL params */
#define DEFAULT_FORCE_PLL             0
#define DEFAULT_PLL_FBDIVC            0
#define DEFAULT_PLL_OUTDIV2           0
#define DEFAULT_PLL_OUTDIV3           0
#define DEFAULT_DDR_SPEED             2400

/* User Defined overrides for MCU Configuration */
#define DEFAULT_2T_MODE               0
#define DEFAULT_HASH_EN               1
#define DEFAULT_INTERLEAVING_EN       0
#define DEFAULT_ADDR_DECODE           1
#define DEFAULT_STRIPE_DECODE         0
#define DEFAULT_ECC_MODE              1
#define DEFAULT_CRC                   0
#define DEFAULT_RD_DBI                0
#define DEFAULT_WR_DBI                0
#define DEFAULT_GEARDOWN              0
#define DEFAULT_BANK_HASH             1
#define DEFAULT_REFRESH_GRANULARITY   0
#define DEFAULT_PMU_ENABLE            1
#define WR_PREAMBLE_2T_MODE           0
/* 0 - Disable, 1 - WRDESKEW, 2 - RDDESKEW, 3 - BOTH */
#define DESKEW_DISABLE                0
#define DESKEW_WRDESKEW               1
#define DESKEW_RDDESKEW               2
#define DESKEW_EN_BOTH                3
#define DEFAULT_DESKEW_EN             DESKEW_EN_BOTH
#define DEFAULT_SW_RX_CAL_EN          1

/* RTT related controls */
#define DEFAULT_RTT_WR                0
#define DEFAULT_RTT_NOM_SLOT0         2 /* 120 ohms */
#define DEFAULT_RTT_NOM_SLOT1         0
#define DEAFULT_RTT_PARK_SLOT0        0
#define DEAFULT_RTT_PARK_SLOT1        0
#define DEFAULT_RTT_NOM_WRLVL         0
#define DEFAULT_RTT_NOM_WRLVL_NONTGT  0
/* MR1 D.I.C Driver Impedance Control */
#define DEAFULT_MR1_DIC               0

/* User Defined margin to Mcu timing params */
#define DEFAULT_CL                   -1
#define DEFAULT_CWL                  -1
#define DEFAULT_RTR_S_MARGIN          0
#define DEFAULT_RTR_L_MARGIN          0
#define DEFAULT_RTR_CS_MARGIN         0
#define DEFAULT_WTW_S_MARGIN          0
#define DEFAULT_WTW_L_MARGIN          0
#define DEFAULT_WTW_CS_MARGIN         0
#define DEFAULT_RTW_S_MARGIN          0
#define DEFAULT_RTW_L_MARGIN          0
#define DEFAULT_RTW_CS_MARGIN         0
#define DEFAULT_WTR_S_MARGIN          0
#define DEFAULT_WTR_L_MARGIN          0
#define DEFAULT_WTR_CS_MARGIN         0

/* User Defined ODT timing */
#define DEFAULT_RDOTTOFF_MARGIN       0
#define DEFAULT_WROTTOFF_MARGIN       0

/*
 * VREF Training
 */
#define DEFAULT_DRAM_VREFDQ_RANGE	0x0	/* Range-1 */
#define DEFAULT_DRAM_VREFDQ_VALUE	0x19	/* 76.25% */
#define DEFAULT_PHY_PAD_VREFDQ_RANGE	0x0	/* Range-1 */
#define DEFAULT_PHY_PAD_VREF_VALUE	0x1F	/* 1F- 76% */
#define DEFAULT_DRAM_VREFDQ_TRAIN_EN	0x1
#define DEFAULT_DRAM_VREFDQ_FINETUNE_EN	0x0
#define DEFAULT_PHY_VREF_TRAIN_EN	0x1

/* User Defined overrides for Phy Calib Mode(ZQ) */
/* ase Interval
 * b00 = 256 cycles
 * b01 = 64K cycles
 * b10 = 16M cycles  */
#define DEFAULT_CAL_MODE_ON           PHY_CAL_EN_RUNTIME
#define DEFAULT_CAL_BASE_INTERVAL     PHY_CAL_INT_BASE
#define DEFAULT_CAL_INTERVAL_COUNT    PHY_CAL_INT_COUNT

/* User Defined Phy Drive & Term values */
#define DEFAULT_PAD_FDBK_DRIVE        PHY_PAD_FDBK_DRIVE
#define DEFAULT_PAD_ADDR_DRIVE        PHY_PAD_ADDR_DRIVE
#define DEFAULT_PAD_CLK_DRIVE         PHY_PAD_CLK_DRIVE
#define DEFAULT_PAD_PAR_DRIVE         PHY_PAD_PAR_DRIVE
#define DEFAULT_PAD_ERR_DRIVE         PHY_PAD_ERR_DRIVE
#define DEFAULT_PAD_ATB_CTRL          PHY_PAD_ATB_CTRL
#define DEFAULT_ADCTRL_SLV_DLY        ADCMD_GRP_SLAVE_DELAY_DEFAULT
#define DEFAULT_DQ_TSEL_ENABLE        PHY_DQ_TSEL_ENABLE
#define DEFAULT_DQS_TSEL_ENABLE       PHY_DQS_TSEL_ENABLE
#define DEFAULT_DQ_TSEL_SELECT        PHY_DQ_TSEL_SELECT
#define DEFAULT_DQS_TSEL_SELECT       PHY_DQS_TSEL_SELECT

#endif
