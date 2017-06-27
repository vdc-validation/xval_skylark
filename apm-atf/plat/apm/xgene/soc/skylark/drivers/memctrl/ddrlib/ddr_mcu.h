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
#ifndef _DDR_MCU_H
#define _DDR_MCU_H

#include "ddr_dmc_defines.h"
#include "ddr_pcp_defines.h"

#define MCU_SUPPORTED_RANKS 8U
#define PHY_SUPPORTED_LANES 9U

/*
 * USER_CONFIG2 scrtach register defined for software usage
 */

/*  MODULE_TYPE_ADDR [ USER_CONFIG2 ]  */
#define DMC520_MODULE_TYPE_ADDR USER_CONFIG2_ADDR
#define FIELD_DMC520_MODULE_TYPE_MSB 3
#define FIELD_DMC520_MODULE_TYPE_LSB 0
#define FIELD_DMC520_MODULE_TYPE_WIDTH 4
#define FIELD_DMC520_MODULE_TYPE_MASK 0xF
#define FIELD_DMC520_MODULE_TYPE_SHIFT_MASK 0x0
#define FIELD_DMC520_MODULE_TYPE_RD(src) ((0xF & (uint32_t)(src)) >> 0)
#define FIELD_DMC520_MODULE_TYPE_WR(dst) (0xF & ((uint32_t)(dst) >> 0))
#define FIELD_DMC520_MODULE_TYPE_SET(dst, src) (((dst) & ~0xF) | (((uint32_t)(src) << 0) & 0xF))

/*  LOGICAL_RANK_ADDR [ USER_CONFIG2 ]  */
#define DMC520_LOGICAL_RANK_ADDR USER_CONFIG2_ADDR
#define FIELD_DMC520_LOGICAL_RANK_MSB 11
#define FIELD_DMC520_LOGICAL_RANK_LSB 4
#define FIELD_DMC520_LOGICAL_RANK_WIDTH 8
#define FIELD_DMC520_LOGICAL_RANK_MASK 0xFF0
#define FIELD_DMC520_LOGICAL_RANK_SHIFT_MASK 0x4
#define FIELD_DMC520_LOGICAL_RANK_RD(src) ((0xFF & (uint32_t)(src)) >> 4)
#define FIELD_DMC520_LOGICAL_RANK_WR(dst) (0xFF & ((uint32_t)(dst) >> 4))
#define FIELD_DMC520_LOGICAL_RANK_SET(dst, src) (((dst) & ~0xFF0) | (((uint32_t)(src) << 4) & 0xFF0))

/*  PHYSICAL_RANK_ADDR [ USER_CONFIG2 ]  */
#define DMC520_PHYSICAL_RANK_ADDR USER_CONFIG2_ADDR
#define FIELD_DMC520_PHYSICAL_RANK_MSB 19
#define FIELD_DMC520_PHYSICAL_RANK_LSB 12
#define FIELD_DMC520_PHYSICAL_RANK_WIDTH 8
#define FIELD_DMC520_PHYSICAL_RANK_MASK 0xFF000
#define FIELD_DMC520_PHYSICAL_RANK_SHIFT_MASK 0xC
#define FIELD_DMC520_PHYSICAL_RANK_RD(src) ((0xFF000 & (uint32_t)(src)) >> 12)
#define FIELD_DMC520_PHYSICAL_RANK_WR(dst) (0xFF000 & ((uint32_t)(dst) >> 12))
#define FIELD_DMC520_PHYSICAL_RANK_SET(dst, src) (((dst) & ~0xFF000) | (((uint32_t)(src) << 12) & 0xFF000))


/* User Status Register */
#define APM_USER_STATUS_INIT_CMPLT_BIT   0
#define APM_USER_STATUS_LP_REQ_BIT       1
#define APM_USER_STATUS_LP_ACK_BIT       2
#define APM_USER_STATUS_PHYUPD_REQ_BIT   3
#define APM_USER_STATUS_PHYUPD_ACK_BIT   4
#define APM_USER_STATUS_CTRLUPD_REQ_BIT  5
#define APM_USER_STATUS_CTRLUPD_ACK_BIT  6

#endif
