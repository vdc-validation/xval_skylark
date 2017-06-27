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

#include "ddr_lib.h"

/* This function is used by AERO/DMT. Start address is DRAM C region */
unsigned long long ddr_base_address(void)
{
	return 0x88000000000ULL;
}


/* Compute Active channels */
unsigned int compute_active_channels(void)
{
	unsigned int active_channels, count = 0, i, data = 0;

	data = iob_read_reg( PCP_RB_RBM_PAGE, PCP_SYSMAP_RB_RBM_RBASR1_OFFSET);

	active_channels = PCP_RB_RBM_RBASR1_MCUAVAIL_RD(data);

	for(i = 0; i < 8 ; i++) {
		count += ((active_channels & (1 << i)) != 0);
	}
	return count;
}

/* Compute the no of ranks */
unsigned int compute_no_of_ranks(void)
{
	unsigned int data;

	data = dmc_read_reg(0U, DMC520_RANK_BITS_NOW_ADDR);

	return (1U << FIELD_DMC520_RANK_BITS_NOW_RD(data));
}

/* Compute rank capacity */
unsigned long long compute_rank_size(void)
{
	unsigned int data, row_bits, column_bits, bank_bits;

	data = dmc_read_reg(0U, ADDRESS_CONTROL_NOW_ADDR);

	row_bits    = FIELD_DMC520_ROW_BITS_NOW_RD(data) + 11;
	column_bits = FIELD_DMC520_COLUMN_BITS_NOW_RD(data) + 8;
	bank_bits   = FIELD_DMC520_BANK_BITS_NOW_RD(data);

	return (1ULL << (row_bits + column_bits + bank_bits + 3));
}

/* This function is used by AERO/DMT. DDR System capacity */
unsigned long long ddr_mem_capacity(void)
{
	unsigned int  no_of_ranks, no_of_channels;
	unsigned long long mem_capacity, per_rank_capacity;

	/* No of Active channels */
	no_of_channels = compute_active_channels();

	/* All channels have same number of ranks, read channel 0 */
	no_of_ranks = compute_no_of_ranks();

	/* No of row, column, bank bits define the rank capacity */
	per_rank_capacity = compute_rank_size();

	mem_capacity = no_of_channels * no_of_ranks * per_rank_capacity;

	ddr_verbose("Per Rank Size: 0x%llX System Memory Capacity: 0x%llX\n",
		    per_rank_capacity, mem_capacity);
	return mem_capacity;
}

/* This function is used to get physical_address->system_address map */
unsigned long long get_sys_addr(unsigned long long phy_addr)
{
	unsigned int  addr_map_mode, read_data;
	unsigned long long sys_addr;

	read_data = dmc_read_reg(0, DMC520_ADDR_MAP_MODE_NOW_ADDR);
	addr_map_mode = FIELD_DMC520_ADDR_MAP_MODE_NOW_RD(read_data) & 0x4;

	if (addr_map_mode) {
		if ((phy_addr > 0) & (phy_addr < 0x80000000)) {
			sys_addr = (phy_addr + 0x80000000);
		} else if ((phy_addr >= 0x80000000) & (phy_addr < 0x800000000)) {
			sys_addr = (phy_addr + 0x800000000);
                } else if ((phy_addr >= 0x800000000) & (phy_addr < 0x8000000000)) {
			sys_addr = (phy_addr + 0x8000000000);
		} else if ((phy_addr >= 0x8000000000) & (phy_addr < 0x80000000000)) {
			sys_addr = (phy_addr + 0x80000000000);
		} else {
			ddr_err("Incorrect physical address!\n");
			sys_addr = (phy_addr + 0x800000000000);
		}
	} else {
		sys_addr = phy_addr;
	}

	ddr_verbose("Physical Address: 0x%llX System Address: 0x%llX\n", phy_addr, sys_addr);
	return sys_addr;
}

/*
 * This function is used by DMT.
 * Provides cacheline stride due to interleaving
 * This is fatal if returned with error (non-zero)
 */
int cacheline_stride(unsigned int *val)
{
	unsigned int stride_length, addr_map_mode, data, apm_addr_map_mode;

	/* Check for interleaving mode, use MCU0 since its always active*/
	data = dmc_read_reg(0U, DMC520_ADDR_MAP_MODE_NOW_ADDR);
	addr_map_mode = FIELD_DMC520_ADDR_MAP_MODE_NOW_RD(data) & 0x7;

	data = mcu_read_reg(0U, PCP_RB_MCUX_MCUAMCR_PAGE_OFFSET);
	apm_addr_map_mode = PCP_RB_MCUX_MCUAMCR_APMADDRMAPMODE_RD(data);

	if (!(addr_map_mode >> 2)) {
		ddr_err("System Address map not enabled!\n");
		return -1;
	}

	if (((addr_map_mode & 0x3) == 0x0) & (apm_addr_map_mode != 0x0)) {
		ddr_err("Cacheline stride for hash mode not supported!\n");
		return -1;
	}

	/* Stride is map_mode * 64 */
	stride_length = 128 * (1 << (addr_map_mode & 0x3));
	*val = stride_length;

	return 0;
}

/*
 * This function is used by DMT. MCU-Rank based address calculation.
 * These are offset addresses. The base address is provided by another function
 * THIS DOESN'T SUPPORT HASHING OR STRIPE DECODE
 */
unsigned long long mcu_offset_address(unsigned int mcu_id, unsigned int rank_addr,
                                      unsigned int bank_addr, unsigned int row_addr,
                                      unsigned int column_addr)
{

	unsigned int data, apm_map_mode, addr_decode, mcb_intrlv, steer_bits = 0;
	unsigned int row_bits, column_bits, bank_bits, rank_bits;
	unsigned int row_mask, column_mask, bank_mask, rank_mask;
	unsigned long long sys_offset = 0, rank_offset = 0U;

	data = dmc_read_reg(0U, DMC520_ADDRESS_DECODE_NOW_ADDR);
	addr_decode = FIELD_DMC520_ADDRESS_DECODE_NOW_RD(data);

	data = dmc_read_reg(0U, ADDRESS_CONTROL_NOW_ADDR);

	row_bits    = FIELD_DMC520_ROW_BITS_NOW_RD(data);
	column_bits = FIELD_DMC520_COLUMN_BITS_NOW_RD(data);
	bank_bits   = FIELD_DMC520_BANK_BITS_NOW_RD(data);
	rank_bits   = FIELD_DMC520_RANK_BITS_NOW_RD(data);

	/* Adjust row, column bits */
	row_bits = row_bits + 11;
	column_bits = column_bits + 8;

	column_mask  = (1 << column_bits) - 1;
	row_mask     = (1 << row_bits) - 1;
	bank_mask    = (1 << bank_bits) - 1;
	rank_mask    = (1 << rank_bits) - 1;

	switch(addr_decode) {
	case 0x0: /* BRB Rank, Bank[msb:2], Row, Bank[1:0], Column */
		rank_offset |= (column_addr & column_mask);
		rank_offset |= ((bank_addr & 0x3)       << column_bits);
		rank_offset |= ((row_addr & row_mask)    << (column_bits + 2));
		rank_offset |= (((bank_addr & 0xC) >> 2) << (row_bits + column_bits + 2));
		rank_offset |= ((rank_addr & rank_mask)   << (row_bits + column_bits + bank_bits));
		break;
	case 0x1: /* RB Row, Rank, Bank, Column */
		rank_offset |= (column_addr & column_mask);
		rank_offset |= ((bank_addr & bank_mask) << (column_bits));
		rank_offset |= ((rank_addr & rank_mask) << (bank_bits + column_bits));
		rank_offset |= ((row_addr & row_mask)   << (rank_bits + bank_bits + column_bits));
		break;
	case 0x2: /* BR Rank, Bank, Row, Column */
		rank_offset |= (column_addr & column_mask);
		rank_offset |= ((row_addr & row_mask)   << column_bits);
		rank_offset |= ((bank_addr & bank_mask) << (row_bits + column_bits));
		rank_offset |= ((rank_addr & rank_mask) << (row_bits + column_bits + bank_bits));
		break;
	}

	rank_offset = rank_offset << 3;
	data = dmc_read_reg(0, ADDRESS_MAP_NOW_ADDR);
	apm_map_mode = FIELD_DMC520_ADDR_MAP_MODE_NOW_RD(data);

	data = csw_read_reg(PCP_RB_CSW_CSWCR0_PAGE_OFFSET);
	mcb_intrlv = PCP_RB_CSW_CSWCR0_DUALMCB_RD(data);

	switch(apm_map_mode) {
	case 0x5: /* MCU0, MCU1 or MCU4 possible combinations */
		steer_bits = (mcu_id != 0);
		break;
	case 0x6:
		if (mcb_intrlv)/* MCU0,1 and MCU4,5 enabled */
			steer_bits = (((mcu_id & 0x1) << 1) | ((mcu_id >> 2) & 0x1));
		else /* MCU 0,1,2,3 */
			steer_bits = mcu_id;
		break;
	case 0x7:
		/* MCU0,1,2,3 and MCU4,5,6,7 enabled */
		steer_bits = (((mcu_id & 0x3) << 1) | ((mcu_id >> 2) & 0x1));
		break;
	}

	switch(apm_map_mode) {
	case 0x4: /* No bits, One MCU */
		sys_offset = rank_offset;
		break;
	case 0x5: /* Two MCUs */
		sys_offset |= (rank_offset & 0x7F);
		sys_offset |= (((rank_offset & ~0x7F) << 1) | ((steer_bits & 0x1) << 7));
		break;
	case 0x6: /* Four MCUs */
		sys_offset |= (rank_offset & 0x7F);
		sys_offset |= (((rank_offset & ~0x7F) << 2) | ((steer_bits & 0x3) << 7));
		break;
	case 0x7: /* Eight MCUs */
		sys_offset |= (rank_offset & 0x7F);
		sys_offset |= (((rank_offset & ~0x7F) << 3) | ((steer_bits & 0x7) << 7));
		break;
	default:
		sys_offset = rank_offset;
		break;
	}

	return sys_offset;
}

int ddr_address_map(void *p, unsigned int flag)
{
	struct apm_memc *ddr = (struct apm_memc *)p;
	unsigned long long mem_capacity, end_addr, sys_addr;
	unsigned int num_mem = 0;

	mem_capacity = ddr_mem_capacity();

	end_addr = mem_capacity - 1;

	/* Map to DDR System address */
	sys_addr = get_sys_addr(end_addr);

	/* Upto four regions are possible */
	/* 0x8000_0000 - 0xFFFF_FFFF - DRAM A */
	if (sys_addr >= 0x80000000) {
		ddr->memspace.str_addr[num_mem] = 0x80000000;
		/* Max check */
		if (sys_addr < 0x100000000)
			ddr->memspace.end_addr[num_mem] = sys_addr;
		else
			ddr->memspace.end_addr[num_mem] = 0xFFFFFFFF;
		num_mem += 1;
	}
	/* 0x8_8000_0000 - 0xF_FFFF_FFFF - DRAM B */
	if (sys_addr >= 0x880000000) {
		ddr->memspace.str_addr[num_mem] = 0x880000000;
		/* Max check */
		if (sys_addr < 0x1000000000)
			ddr->memspace.end_addr[num_mem] = sys_addr;
		else
			ddr->memspace.end_addr[num_mem] = 0xFFFFFFFFF;
		num_mem += 1;
	}
	/* 0x88_0000_0000 - 0xFF_FFFF_FFFF - DRAM C */
	if (sys_addr >= 0x8800000000) {
		ddr->memspace.str_addr[num_mem] = 0x8800000000;
		/* Max check */
		if (sys_addr < 0x10000000000)
			ddr->memspace.end_addr[num_mem] = sys_addr;
		else
			ddr->memspace.end_addr[num_mem] = 0xFFFFFFFFFF;
		num_mem += 1;
	}
	/* 0x880_0000_0000 - 0xFFF_FFFF_FFFF - DRAM D */
	if (sys_addr >= 0x88000000000) {
		ddr->memspace.str_addr[num_mem] = 0x88000000000;
		/* Max check */
		if (sys_addr < 0x100000000000)
			ddr->memspace.end_addr[num_mem] = sys_addr;
		else
			ddr->memspace.end_addr[num_mem] = 0xFFFFFFFFFFF;
		num_mem += 1;
	}
	ddr->memspace.num_mem_regions = num_mem;
	if (sys_addr >= 0x100000000000)
		return -1;

	return 0;
}
