/*
 * Copyright (c) 2016, Applied Micro Circuits Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The non-volatile parameter layout in SPI-NOR is shown below. There is
 * two copies. The master copy is changeable by the user. The Last Known
 * copy is handled by the fail safe future. It is a last know bootable copy.
 *
 *  ---------------------------
 *  | Master Copy             | 16KB
 *  | Pre-boot parameters     |
 *  ---------------------------
 *  | Master Copy             | 16KB
 *  | Pre-boot parameters     |
 *  | w/o failsafe support    |
 *  ---------------------------
 *  | Master Copy             |
 *  | Manufactory &           | 32KB
 *  | Users parameters        |
 *  ---------------------------
 *  | Last Known Copy         | 16KB
 *  | Pre-boot parameters     |
 *  ---------------------------
 *  |                         | 16KB
 *  ---------------------------
 *  | Last Known Copy         |
 *  | Manufactory &           | 32KB
 *  | Users parameters        |
 *  ---------------------------
 *
 * As each non-volatile parameter requires 8 bytes, there is a total of 8K
 * parameters.
 */

#ifndef _XGENE_NVPARAM_H_
#define _XGENE_NVPARAM_H_

#include <stdint.h>

#define ENO_PARAM			-1000
#define NV_PARAM_BLOCKSIZE		(64 * 1024)
#define NV_PARAM_ENTRY_SIZE		8

#define NV_PREBOOT_PARAM_REGION_SIZE	0x4000
#define NV_PREBOOT2_PARAM_REGION_SIZE	0x4000
#define NV_MANU_USER_PARAM_REGION_SIZE	0x8000

enum nvparam {
	/*
	 * Physical storage device based is determined by the platform
	 *
	 * Pre-boot Non-volatile memory
	 */
	NV_PREBOOT_PARAM_START		= 0x000000,
	NV_RSVD1			= 0x000000,
	NV_RSVD2			= 0x000008,
	NV_DRAM_SPEED			= 0x000010,
	NV_DRAM_ECC_MODE		= 0x000018,
	NV_MCU_MASK			= 0x000020,
	/* NOTE: All NV_PCP are required to be sequential */
	NV_PCP_VDMC			= 0x000028,
	NV_PCP_DCC_SIGN			= 0x000030,
	NV_CSWCR0_IOBSNPFILTERDIS	= 0x000038,
	NV_CSWCR0_L3COCFRC		= 0x000040,
	NV_CSWCR0_L3CREQPRI		= 0x000048,
	NV_CSWCR0_PMDREQPRI		= 0x000050,
	NV_CSWCR0_HASHMASK		= 0x000058,
	NV_CSWCR0_HASHEN		= 0x000060,
	NV_CSWCR0_MCUINTLV		= 0x000068,
	NV_CSWCR0_MCBINTLV		= 0x000070,
	NV_CSWCR0_MCB1ROUTINGMODE	= 0x000078,
	NV_CSWCR0_MCB0ROUTINGMODE	= 0x000080,
	NV_CSWCR0_QUADL3C		= 0x000088,
	NV_CSWCR0_DUALMCB		= 0x000090,
	NV_CSWCR1			= 0x000098,
	NV_L3C_SCRUB			= 0x0000A0,
	NV_L3C_RCR			= 0x0000A8,
	NV_L3C_CR_DHASH			= 0x0000B0,
	NV_L3C_CR_DIHASH		= 0x0000B8,
	NV_L3C_CR_DINCRCG		= 0x0000C0,
	NV_L3C_DISABLE			= 0x0000C8,
	NV_L3C_SIZE			= 0x0000D0,
	NV_PCP_ACTIVEPMD		= 0x0000D8,
	NV_PMD_L2CR			= 0x0000E0,
	NV_PMD_CCCR			= 0x0000E8,
	NV_PMD_CCCR2			= 0x0000F0,
	NV_PMD_SKEWTUNE			= 0x0000F8,
	NV_FAILSAFE_RETRY		= 0x000100,
	NV_DDR_LOG_LEVEL		= 0x000108,
	NV_DDR_HASH_EN			= 0x000110,
	NV_DDR_INTLV_EN			= 0x000118,
	NV_DDR_SPECULATIVE_RD		= 0x000120,
	NV_DDR_ADDR_DECODE		= 0x000128,
	NV_DDR_STRIPE_DECODE		= 0x000130,
	NV_DDR_CRC_MODE			= 0x000138,
	NV_DDR_RD_DBI_EN		= 0x000140,
	NV_DDR_WR_DBI_EN		= 0x000148,
	NV_DDR_GEARDOWN_EN		= 0x000150,
	NV_DDR_BANK_HASH_EN		= 0x000158,
	NV_DDR_REFRESH_GRANULARITY	= 0x000160,
	NV_DDR_PMU_EN			= 0x000168,
	NV_DDR_WR_PREAMBLE_2T_MODE	= 0x000170,
	NV_DDR_PAD_VREF_CTRL		= 0x000178,
	NV_DDR_DESKEW_EN		= 0x000180,
	NV_DDR_RTR_S_MARGIN		= 0x000188,
	NV_DDR_RTR_L_MARGIN		= 0x000190,
	NV_DDR_RTR_CS_MARGIN		= 0x000198,
	NV_DDR_WTW_S_MARGIN		= 0x0001A0,
	NV_DDR_WTW_L_MARGIN		= 0x0001A8,
	NV_DDR_WTW_CS_MARGIN		= 0x0001B0,
	NV_DDR_RTW_S_MARGIN		= 0x0001B8,
	NV_DDR_RTW_L_MARGIN		= 0x0001C0,
	NV_DDR_RTW_CS_MARGIN		= 0x0001C8,
	NV_DDR_WTR_S_MARGIN		= 0x0001D0,
	NV_DDR_WTR_L_MARGIN		= 0x0001D8,
	NV_DDR_WTR_CS_MARGIN		= 0x0001E0,
	/*
	 * Second region of PCP preboot variables.
	 */
	NV_MCB_SPECRMR			= 0x0001E8,
	NV_MCB_SPECBMR			= 0x0001F0,
	NV_IOB_PACFG			= 0x0001F8,
	NV_IOB_BACFG			= 0x000200,

	/*
	 * NOTE: Add new param before NV_PREBOOT_PARAM_MAX and increase its
	 * value
	 */
	NV_PREBOOT_PARAM_MAX		= 0x000200,

	/*
	 * Non-failsafe pre-boot non-volatile memory
	 *
	 * These parameters does not support failsafe and will always
	 * read from its location.
	 */
	NV_PREBOOT2_PARAM_START		= 0x004000,
	NV_RO_BOARD_TYPE		= 0x004000, /* Follow BMC FRU format */
	NV_RO_BOARD_REV			= 0x004008, /* Follow BMC FRU format */

	/*
	 * NOTE: Add new param before NV_PREBOOT2_PARAM_MAX and increase its
	 * value
	 */
	NV_PREBOOT2_PARAM_MAX		= 0x004008,

	/* Manufactory/User Non-volatile memory */
	/* NOTE: All margin are required to be sequential */
	NV_MANU_USER_PARAM_START	= 0x008000,
	NV_DDR_VMARGIN			= 0x008000,
	NV_SOC_VMARGIN			= 0x008008,
	NV_AVS_VMARGIN			= 0x008010,
	NV_TPC_TM1_MARGIN		= 0x008018,
	NV_TPC_TM2_MARGIN		= 0x008020,
	NV_TPC_FREQ_THROTTLE		= 0x008028,
	NV_CPPC_PID_KP			= 0x008030,
	NV_CPPC_PID_KI			= 0x008038,
	NV_CPPC_PID_KD			= 0x008040,
	NV_CPPC_PID_PEDIOD_MS		= 0x008048,
	NV_SMPRO_CONSOLE		= 0x008050,
	NV_TB0PSCR			= 0x008058,
	NV_TB1PSCR			= 0x008060,

	/*
	 * NOTE: Add new param before NV_MANU_USER_PARAM_MAX and increase its
	 * value
	 */
	NV_MANU_USER_PARAM_MAX		= 0x008060,
};

#define NV_PERM_ALL	0xFFFF	/* Allowed for all */
#define NV_PERM_ATF	0x0001	/* Allowed for EL3 code */
#define NV_PERM_OPTEE	0x0004	/* Allowed for secure El1 */
#define NV_PERM_BIOS	0x0008	/* Allowed for EL2 non-secure */
#define NV_PERM_MANU	0x0010	/* Allowed for manufactory interface */
#define NV_PERM_BMC	0x0020	/* Allowed for BMC interface */

/*
 * Retrieve a non-volatile parameter
 *
 * @param:	Parameter ID to retrieve
 * @acl_rd:	Permission for read operation. See NV_PERM_XXX.
 * @val:	Pointer to an uint32 to store the value
 * @return:	-EINVAL if parameter is invalid
 *              -EPERM if permission not allowed
 *              -ENO_PARAM if no parameter set
 *              Negative value if non-volatile operation write failed
 *              Otherwise, 0 for success
 *
 * NOTE: If you need a signed value, cast it. It is expected that the
 * caller will carry the correct permission over various call sequences.
 *
 */
int plat_nvparam_get(enum nvparam param, uint16_t acl_rd, uint32_t *val);

/*
 * Set a non-volatile parameter
 *
 * @param:	Parameter ID to set
 * @acl_rd:	Permission for read operation
 * @acl_wr:	Permission for write operation
 * @val:	Unsigned int value to set.
 * @return:	-EINVAL if parameter is invalid
 *              -EPERM if permission not allowed
 *              Negative value if non-volatile operation write failed
 *              Otherwise, 0 for success
 *
 * NOTE: If you have a signed value, cast to unsigned. If the parameter has
 * not being created before, the provied permission is used to create the
 * parameter. Otherwise, it is checked for access. It is expected that the
 * caller will carry the correct permission over various call sequences.
 *
 */
int plat_nvparam_set(enum nvparam param, uint16_t acl_rd, uint16_t acl_wr,
		     uint32_t val);

/*
 * Clear a non-volatile parameter
 *
 * @param:	Parameter ID to set
 * @acl_wr:	Permission for write operation
 * @return:	-EINVAL if parameter is invalid
 *              -EPERM if permission not allowed
 *              Negative value if non-volatile operation write failed
 *              Otherwise, 0 for success
 *
 * NOTE: If you have a signed value, cast to unsigned. If the parameter has
 * not being created before, the provied permission is used to create the
 * parameter. Otherwise, it is checked for access. It is expected that the
 * caller will carry the correct permission over various call sequences.
 *
 */
int plat_nvparam_clr(enum nvparam param, uint16_t acl_wr);

/*
 * Clear all non-volatile parameters
 * @return:     Negative value if non-volatile operation write failed
 *              Otherwise, 0 for success
 */
int plat_nvparam_clr_all(void);

/*
 * Setup all current parameters as last good known setting
 *
 * @return:
 *              Negative value if non-volatile operation read/write failed
 *              Otherwise, 0 for success
 */
int plat_nvparam_save_last_good_known_setting(void);

/*
 * Restore last good known settings.
 *
 * @return:
 *              Negative value if non-volatile operation read/write failed
 *              Otherwise, 0 for success
 */
int plat_nvparam_restore_last_good_known_setting(void);

#endif
