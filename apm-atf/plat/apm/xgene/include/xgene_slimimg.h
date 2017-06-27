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
#ifndef _XGENE_SLIMIMG_H_
#define _XGENE_SLIMIMG_H_

#include <stdint.h>

#define SLIMIMG_SIGNATURE			0x43434D41
#define SLIMIMG_ENV_SIGNATURE			0x564E4541
#define SLIMIMG_MAX_FILENAME_SIZE		16
#define IPP_SLIMFS_MAX_FILE_HANDLES		4

#define SLIMIMG_HDR_RESERVED_WORDS		5
#define SLIMIMG_FILETBL_RESERVED_WORDS	7
#define SLIMIMG_ENV_RESERVED_WORDS		2

/** SlimImg header information */
__packed struct slimimg_header {
	uint32_t signature;
	uint32_t block_size;
	uint32_t file_count;
	uint32_t boot_fnum;
	uint32_t boot_roffset;
	uint32_t boot_rsize;
	uint32_t slimenv_a_ptr;
	uint32_t slimenv_b_ptr;
	uint32_t slimhdr_b_ptr;
	uint32_t dev_params;
	uint32_t reserved[SLIMIMG_HDR_RESERVED_WORDS];
	uint32_t header_crc32;
};
typedef struct slimimg_header slimimg_header_t;

/** file entry information for SlimImg */
__packed struct slimimg_file {
	char  file_name[SLIMIMG_MAX_FILENAME_SIZE];
	uint32_t file_offset;
	uint32_t file_size;
	uint32_t file_time_seconds:5;
	uint32_t file_time_minutes:6;
	uint32_t file_time_hours:5;
	uint32_t file_date_day:5;
	uint32_t file_date_month:4;
	uint32_t file_date_year:7;
	uint32_t counter;
	uint32_t file_reserved[SLIMIMG_FILETBL_RESERVED_WORDS];
	uint32_t file_crc32;
};
typedef struct slimimg_file slimimg_file_t;

#endif /* _XGENE_SLIMIMG_H_ */

