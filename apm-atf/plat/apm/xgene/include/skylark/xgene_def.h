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
 */

#ifndef __XGENE_DEF_H__
#define __XGENE_DEF_H__

#include <config.h>
#include <skylark_soc.h>

#define PLAT_ARM_GICD_BASE		GICD_REG_BASE
#define PLAT_ARM_GICR_BASE		GICR_REG_BASE
#define TICKS_PER_MICRO_SEC		(SYS_CNT_FREQ / 1000000)
#define XGENE_BOOT_UART_BASE		UART4_REG_BASE
#define XGENE_BOOT_UART_PORT		4
#define XGENE_BOOT_NS2_UART_BASE	UART1_REG_BASE
#define XGENE_BOOT_NS_UART_BASE		UART0_REG_BASE
#define XGENE_BOOT_UART_BAUDRATE	UART4_BAUDRATE
#define XGENE_BOOT_UART_CLK_IN_HZ	SYS_CLK_FREQ
#define XGENE_BL31_RUN_UART_BASE	UART4_REG_BASE
#define XGENE_BL31_RUN_UART_PORT	4
#define XGENE_BL31_RUN2_UART_BASE	UART1_REG_BASE
#define XGENE_BL31_RUN_UART_BAUDRATE	UART4_BAUDRATE
#define XGENE_BL31_RUN_UART_CLK_IN_HZ	SYS_CLK_FREQ

/* First 128KB of OCM is ROM so TZRAM_BASE start from OCM Base + 128KB */
#define OCM_ROM_SIZE		0x20000
/* The last 4KB of OCM is used for global data such as CPU trusted mailbox */
#define OCM_RW_DATA_SIZE	0x1000
#define TZRAM_BASE		(OCM_BASE + OCM_ROM_SIZE)
#define TZRAM_SIZE		(OCM_SIZE - OCM_ROM_SIZE - OCM_RW_DATA_SIZE)
#define TZRAM_END		(TZRAM_BASE + TZRAM_SIZE)
#define OCM_RW_DATA_BASE	TZRAM_END
#define OCM_RW_DATA_END		(OCM_RW_DATA_BASE + OCM_RW_DATA_SIZE)

/*
 * 32MB Secured shared memory between BLs
 */
#define XGENE_BL_SHARED_RAM_BASE	0x80000000
#define XGENE_BL_SHARED_RAM_SIZE	0x02000000

/*
 * Non secure DRAM mapping from 0x8820_0000 to 0xFFFF_FFFF
 */
#define XGENE_NS_DRAM_BASE			0x88200000
#define XGENE_NS_DRAM_SIZE			0x77E00000

#define XGENE_BL_SHARED_NS_RAM_BASE		XGENE_NS_DRAM_BASE
#define XGENE_BL_SHARED_NS_RAM_LIMIT	0x90000000
#define XGENE_BL_SHARED_NS_RAM_MAX_SIZE	(XGENE_BL_SHARED_NS_RAM_LIMIT -\
								XGENE_BL_SHARED_NS_RAM_BASE)
#define XGENE_NS_BL33_RAM_BASE			XGENE_BL_SHARED_NS_RAM_LIMIT
#define XGENE_NS_BL33_RAM_SIZE	\
	(XGENE_NS_DRAM_BASE + XGENE_NS_DRAM_SIZE - XGENE_NS_BL33_RAM_BASE)
/* Load address of Non-Secure Image */
#define XGENE_NS_BL33_IMAGE_OFFSET		(XGENE_NS_BL33_RAM_BASE + 0x2000000)
#define PLAT_XGENE_NS_NVRAM_OFFSET	0xD00000
#define PLAT_XGENE_NS_NVRAM_SIZE	0x100000

/*******************************************************************************
 * Platform core trusted mailbox defines.
 ******************************************************************************/
#define PLAT_XGENE_TRUSTED_MAILBOX_SIZE	(8 * PLATFORM_CORE_COUNT)
#define PLAT_XGENE_TRUSTED_MAILBOX_BASE	OCM_RW_DATA_BASE

/*******************************************************************************
 * BL1 specific defines.
 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
 * addresses.
 ******************************************************************************/
#define BL1_RO_BASE		OCM_BASE
#define BL1_RO_LIMIT		(OCM_BASE + OCM_ROM_SIZE)

/*
 * Put BL1 RW at the top of the Trusted SRAM.
 */
#define BL1_RW_BASE		(TZRAM_BASE +		\
						TZRAM_SIZE -	\
						PLAT_MAX_BL1_RW_SIZE)
#define BL1_RW_LIMIT	TZRAM_END

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
#if ARM_BL31_IN_DRAM
#error "TBD"
/*
 * Put BL31 at the bottom of TZC secured DRAM
 */
#define BL31_BASE		0x80000000 /* TBD */
#define BL31_LIMIT		(BL31_BASE + 0x80000) /* TBD */
#else
/*
 * Put BL31 at the top of the Trusted SRAM.
 */
#define BL31_BASE		(TZRAM_BASE +		\
						TZRAM_SIZE -	\
						PLAT_MAX_BL31_SIZE)
#define BL31_LIMIT		TZRAM_END
#endif

/*******************************************************************************
 * BL2 specific defines.
 ******************************************************************************/
#if ARM_BL31_IN_DRAM
/*
 * BL31 is loaded in the DRAM.
 * Put BL2 just below BL1.
 */
#define BL2_BASE		(BL1_RW_BASE - PLAT_MAX_BL2_SIZE)
#define BL2_LIMIT		BL1_RW_BASE
#else
/*
 * Put BL2 just below BL31.
 */
#define BL2_BASE		(BL31_BASE - PLAT_MAX_BL2_SIZE)
#define BL2_LIMIT		BL31_BASE
#endif

#define SCP_BL2U_BASE	BL31_BASE
#define BL2U_BASE		BL2_BASE
#define BL2U_LIMIT		BL2_LIMIT

/*******************************************************************************
 * BL32 specific defines.
 ******************************************************************************/
#define BL32_BASE                       0x82000000
#ifdef BL32_BASE
#define BL32_SEC_MEM_SIZE               0x6000000
#define BL32_SHM_MEM_BASE               0x88000000
#define BL32_SHM_MEM_SIZE               0x200000
#endif

/*******************************************************************************
 * FW Upgrade specific defines.
 ******************************************************************************/
/*
 * Slimpro load NS_BL1U from NVRAM to NS_BL1U_BASE in non secured DRAM.
 */
#define NS_BL1U_BASE		XGENE_BL_SHARED_NS_RAM_BASE
#define NS_BL1U_MAX_SIZE	0x20000 /* 128K */
#define NS_BL1U_LIMIT		(NS_BL1U_BASE + NS_BL1U_MAX_SIZE)

/* Location to store FWU_CERT in NS_BL1U */
#define NS_FWU_CERT_BASE		NS_BL1U_LIMIT
#define NS_FWU_CERT_MAX_SIZE	0x10000 /* 64K */
#define NS_FWU_CERT_LIMIT	(NS_FWU_CERT_BASE + NS_FWU_CERT_MAX_SIZE)

/* Location to store SCP_BL2U in NS_BL1U */
#define NS_SCP_BL2U_BASE		NS_FWU_CERT_LIMIT
#define NS_SCP_BL2U_MAX_SIZE	0x20000 /* 128K */
#define NS_SCP_BL2U_LIMIT	(NS_SCP_BL2U_BASE + NS_SCP_BL2U_MAX_SIZE)

/* Location to store BL2U in NS_BL1U */
#define NS_BL2U_BASE		NS_SCP_BL2U_LIMIT
#define NS_BL2U_MAX_SIZE	0x20000 /* 128K */
#define NS_BL2U_LIMIT		(NS_BL2U_BASE + NS_BL2U_MAX_SIZE)

/*
 * Firmware upgrade info:
 *	- This non secured DRAM region provides info to NS_BL1U, BL2U to process
 *	firmware upgrade or recovery
 */
#define NS_FWU_MAX_SIZE		0x2000000 /* 32MB */
#define NS_FWU_BASE			(XGENE_BL_SHARED_NS_RAM_LIMIT -\
							NS_FWU_MAX_SIZE)
#define NS_FWU_LIMIX		XGENE_BL_SHARED_NS_RAM_LIMIT

/*
 * Non secured DRAM region for PCC shared memory
 */
#define NS_PCC_SM_MAX_SIZE	0x200000 /* 2MB */
#define NS_PCC_SM_BASE		(NS_FWU_BASE - NS_PCC_SM_MAX_SIZE)
#define NS_PCC_SM_LIMIT		(NS_FWU_BASE)

/*
 * This offset for SPI_NOR service to determine what kind of devices.
 * It needs to be out of flash region
 */
#define NVRAM_SERVICE_ATF_OFFSET		SPI_NOR_SIZE
#define NVRAM_SERVICE_ATF_MAX_SIZE		0x1000000 /* 16MB */
#define NVRAM_SERVICE_ATF_LIMIT	\
	(SPI_NOR_SIZE + NVRAM_SERVICE_ATF_MAX_SIZE)
#define NVRAM_SERVICE_SMPMPRO_OFFSET	\
	(SPI_NOR_SIZE + NVRAM_SERVICE_ATF_MAX_SIZE)
#define NVRAM_SERVICE_SMPMPRO_MAX_SIZE	0x20000 /* 128KB */
#define NVRAM_SERVICE_SMPMPRO_LIMIT	\
	(NVRAM_SERVICE_SMPMPRO_OFFSET + NVRAM_SERVICE_SMPMPRO_MAX_SIZE)

/*******************************************************************************
 * Memory mapping setup.
 ******************************************************************************/
/* 4 secured bytes for BL1 determine FW upgrade or recovery mode */
#define PLAT_XGENE_FW_UPGRADE_REQUEST_BASE	XGENE_BL_SHARED_RAM_BASE

#define PLAT_XGENE_FW_MAX_SIZE		0xE00000
#define PLAT_XGENE_FW_RAM_BASE		(XGENE_BL_SHARED_RAM_BASE +\
		XGENE_BL_SHARED_RAM_SIZE - PLAT_XGENE_FW_MAX_SIZE)

#define PLAT_XGENE_MEMINFO_SIZE	0x80
#define PLAT_XGENE_MEMINFO_BASE	\
		(PLAT_XGENE_FW_RAM_BASE - PLAT_XGENE_MEMINFO_SIZE)

#define PLAT_XGENE_DIMMINFO_SIZE	0x1000 /* 4KB */
#define PLAT_XGENE_DIMMINFO_BASE	\
		(PLAT_XGENE_MEMINFO_BASE - PLAT_XGENE_DIMMINFO_SIZE)

#define PLAT_XGENE_FLASH_BUF_SIZE	0x10000 /* 64KB */
#define PLAT_XGENE_FLASH_BUF_BASE	\
		(PLAT_XGENE_DIMMINFO_BASE - PLAT_XGENE_FLASH_BUF_SIZE)

#define XGENE_MAP_BL_SHARED_RAM		MAP_REGION_FLAT(\
				XGENE_BL_SHARED_RAM_BASE,	\
				XGENE_BL_SHARED_RAM_SIZE,	\
				(MT_NON_CACHEABLE | MT_RW | MT_SECURE))

/* Device MMIO mapping */
#define XGENE_MAP_DEVICE_MMIO		MAP_REGION_FLAT(\
				0x0,\
				OCM_BASE,\
				(MT_DEVICE | MT_RW | MT_SECURE)),\
				MAP_REGION_FLAT(\
				(OCM_BASE + OCM_SIZE),\
				0x62F80000,\
				(MT_DEVICE | MT_RW | MT_SECURE))

#define XGENE_MAP_NS_DRAM			MAP_REGION_FLAT(\
				XGENE_NS_DRAM_BASE,\
				XGENE_NS_DRAM_SIZE,\
				(MT_MEMORY | MT_RW | MT_NS))

#define XGENE_MAP_DRAM_REGIONB	MAP_REGION_FLAT(\
				0x880000000,\
				0x780000000,\
				(MT_MEMORY | MT_RW | MT_NS))

#define XGENE_MAP_DRAM_REGIONC	MAP_REGION_FLAT(\
				0x8800000000,\
				0x7800000000,\
				(MT_MEMORY | MT_RW | MT_NS))

#define XGENE_MAP_DRAM_REGIOND	MAP_REGION_FLAT(\
				0x18000000000,\
				0x8000000000,\
				(MT_MEMORY | MT_RW | MT_NS))

#if BL32_BASE
#define XGENE_MAP_SEC_MEM       MAP_REGION_FLAT(\
                                BL32_BASE,\
                                BL32_SEC_MEM_SIZE,\
                                MT_MEMORY | MT_RW | MT_SECURE)
#endif

/*******************************************************************************
 * GIC secure interrupts
 ******************************************************************************/
/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology.
 */
#define ARM_IRQ_SEC_PHY_TIMER		29

#define ARM_IRQ_SEC_SGI_0		8
#define ARM_IRQ_SEC_SGI_1		9
#define ARM_IRQ_SEC_SGI_2		10
#define ARM_IRQ_SEC_SGI_3		11
#define ARM_IRQ_SEC_SGI_4		12
#define ARM_IRQ_SEC_SGI_5		13
#define ARM_IRQ_SEC_SGI_6		14
#define ARM_IRQ_SEC_SGI_7		15

/* List of X-Gene SPI interrupts that will trigger in EL3 */
#define XGENE_SPI_SMPROERR		64
#define XGENE_SPI_PMPROERR		68
#define XGENE_SPI_PCPHPERR		80
#define XGENE_SPI_PCPLPERR		81
#define XGENE_SPI_MEMERR		83
#define XGENE_SPI_NS_WS1_TIMER		93

/*
 * TBD: ATF needs to generate an interrupt to OS. What
 *      interrupt number should we use for RAS, until we
 *      move to SDEI?
 *
 *      Currently PMpro uses PCPHPERR IRQ 80 to notify GHES of errors.
 *
 *      See the following file in PMpro
 *        apm-smpmpro/core/ras_pmpro.c: ras_notify_armv8
 *
 *      See the following file in AptioV
 *        apm-ami-aptiov/SkylarkPkg/Asl/Hest.adt: Vector
 *        apm-ami-aptiov/SkylarkPkg/Boards/Osprey/Asl/Dsdt.asl: GHES
 */
#define XGENE_SPI_EXT_IRQ_RAS		72


#define PLAT_ARM_G1S_IRQS	ARM_IRQ_SEC_PHY_TIMER,	\
				ARM_IRQ_SEC_SGI_0,	\
				ARM_IRQ_SEC_SGI_1,	\
				ARM_IRQ_SEC_SGI_2,	\
				ARM_IRQ_SEC_SGI_3,	\
				ARM_IRQ_SEC_SGI_4,	\
				ARM_IRQ_SEC_SGI_5,	\
				ARM_IRQ_SEC_SGI_6,	\
				ARM_IRQ_SEC_SGI_7

#define PLAT_ARM_G0_IRQS	ARM_IRQ_SEC_SGI_0,	\
				ARM_IRQ_SEC_SGI_6,	\
				XGENE_SPI_NS_WS1_TIMER

#define XGENE_PRIMARY_CPU		0x0

#endif /* __XGENE_DEF_H__ */
