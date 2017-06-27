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

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include <common_def.h>
#include <tbbr_img_def.h>
#include <xgene_def.h>

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/
#define MAX_IO_DEVICES	3
#define MAX_IO_HANDLES	4
#define MAX_IRQ_HANDLERS 5

/*
 * PLAT_MAX_BL1_RW_SIZE is calculated using the current BL1 RW debug size
 * plus a little space for growth.
 */
#define PLAT_MAX_BL1_RW_SIZE	0xF000

/*
 * PLAT_MAX_BL2_SIZE is calculated using the current BL2 debug size plus a
 * little space for growth.
 */
#if TRUSTED_BOARD_BOOT
# define PLAT_MAX_BL2_SIZE	0x1E000
#else
# define PLAT_MAX_BL2_SIZE	0xC000
#endif

/*
 * PLAT_MAX_BL31_SIZE is 256KB
 */
#define PLAT_MAX_BL31_SIZE	0x40000

/* Size of cacheable stacks */
#if IMAGE_BL1
#if TRUSTED_BOARD_BOOT
# define PLATFORM_STACK_SIZE 0x1000
# define BL1_SECONDARY_STACK_SIZE 0x200
#else
# define PLATFORM_STACK_SIZE 0x1000
# define BL1_SECONDARY_STACK_SIZE 0x200
#endif
#elif IMAGE_BL2
# if TRUSTED_BOARD_BOOT
#  define PLATFORM_STACK_SIZE 0x1000
# else
#  define PLATFORM_STACK_SIZE 0x400
# endif
#elif IMAGE_BL2U
# define PLATFORM_STACK_SIZE 0x400
#elif IMAGE_BL31
# define PLATFORM_STACK_SIZE 0x400
#elif IMAGE_BL32
# define PLATFORM_STACK_SIZE 0x440
#elif IMAGE_BL1U_NS
# define PLATFORM_STACK_SIZE 0x400
#endif

#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * \
					 PLATFORM_MAX_CPUS_PER_CLUSTER)
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CORE_COUNT + \
					 PLATFORM_CLUSTER_COUNT + 1)

/*******************************************************************************
 * Power down state IDs
 ******************************************************************************/
/* Local power state for power domains in Run state */
#define ARM_LOCAL_STATE_RUN	0
/* Local power state for retention */
#define ARM_LOCAL_STATE_RET	1
/* Local power state for OFF/power-down */
#define ARM_LOCAL_STATE_OFF	2

#define PLAT_MAX_RET_STATE	ARM_LOCAL_STATE_RET
#define PLAT_MAX_OFF_STATE	ARM_LOCAL_STATE_OFF

#define PLAT_AFF0_SHIFT		0
#define PLAT_AFF1_SHIFT		4
#define PLAT_AFF2_SHIFT		8
#define PLAT_AFFLVL_MASK	0xF
#define PLAT_AFFLVL0		0
#define PLAT_AFFLVL1		1
#define PLAT_AFFLVL2		2
#define PLAT_AFFLVL0_VAL(pid) \
		(((pid) >> PLAT_AFF0_SHIFT) & PLAT_AFFLVL_MASK)
#define PLAT_AFFLVL1_VAL(pid) \
		(((pid) >> PLAT_AFF1_SHIFT) & PLAT_AFFLVL_MASK)
#define PLAT_AFFLVL2_VAL(pid) \
		(((pid) >> PLAT_AFF2_SHIFT) & PLAT_AFFLVL_MASK)

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ull << 41)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ull << 41)
#define MAX_XLAT_TABLES			4

/*
 * The mmap table size can be increased as much as we need.
 * However, it shouldn't be set too big because it locates on OCM
 */
#define MAX_MMAP_REGIONS		12

/*******************************************************************************
 * Some data must be aligned on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * internal and external caches.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

#endif /* __PLATFORM_DEF_H__ */
