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

#ifndef _XGENE_DIMM_INFO_H_
#define _XGENE_DIMM_INFO_H_

#include <stdint.h>

#define DIMM_MAX_SLOT		16
#define DIMM_PART_MAX_CHAR	32
#define DIMM_MFC_MAX_CHAR	2

/* DIMM type */
/** Types of dimm type */
enum {
	XUDIMM,
	XRDIMM,
	XSODIMM,
	XRSODIMM,
	XLRDIMM,
	XUNKNOWN
};

/* DIMM status */
enum {
	DIMM_INSTALLED = 0, /* installed and operational */
	DIMM_NOT_INSTALLED,
	DIMM_FAILED_DISABLED
};

struct xgene_plat_dimm_info {
	uint8_t		dimm_part_number[DIMM_PART_MAX_CHAR];
	uint64_t	dimm_size;
	uint8_t		dimm_nr_rank;
	uint8_t		dimm_type;
	uint8_t		dimm_status;
	uint16_t	dimm_mfc_id;
	uint8_t		dimm_dev_type;
};

/* Raw memory SPD Data structure */
struct xgene_mem_spd_data {
	uint8_t Byte2;			 /* Memory Type */
	uint8_t Byte5To8[9 - 5];	 /* Attribute, Total Width, Data Width (DDR2&3) */
	uint8_t Byte11To14[15 - 11]; 	 /* ECC Data Width, Data Width (DDR4) */
	uint8_t Byte64To71[72 - 64]; 	 /* Manufacturer (DDR2) */
	uint8_t Byte73To90[91 - 73]; 	 /* Part Number (DDR2) */
	uint8_t Byte95To98[99 - 95]; 	 /* Serial Number (DDR2) */
	uint8_t Byte117To118[119 - 117]; /* Manufacturer (DDR3) */
	uint8_t Byte122To125[126 - 122]; /* Serial Number (DDR3) */
	uint8_t Byte128To145[146 - 128]; /* Part Number (DDR3) */
	uint8_t Byte320To321[322 - 320]; /* Manufacturer (DDR4) */
	uint8_t Byte325To328[329 - 325]; /* Serial Number (DDR4) */
	uint8_t Byte329To348[349 - 329]; /* Part Number (DDR4) */
};

struct xgene_plat_dimm {
	struct xgene_plat_dimm_info info;
	struct xgene_mem_spd_data spd_data;
};

struct xgene_plat_dimm_list {
	uint32_t num_slot;
	struct xgene_plat_dimm dimm[DIMM_MAX_SLOT];
};

#endif /* _XGENE_DIMM_INFO_H_ */
