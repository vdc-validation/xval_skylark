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

#ifndef _XGENE_FWU_H_
#define _XGENE_FWU_H_

#include <uuid.h>
#include <stdint.h>

int plat_xgene_is_fw_upgrade_mode(void);
int plat_xgene_is_recovery_mode(void);
void plat_xgene_set_fw_upgrade_mode(void);

void xgene_init_fwu(void);
int xgene_add_fw_upgrade_request(uuid_t *uuid, uint32_t offset,
						uint8_t *base, uint32_t size);
int xgene_search_fwu_fip(uint64_t *base, uint32_t *len);
int xgene_search_fip_slimimg(uint64_t slimimg_base,
						uint64_t *base, uint32_t *len);

int xgene_fw_slimimg_copy_to_mem(void);
int xgene_flash_fw_slimimg(uint8_t *slimimg_base, uint32_t len);
int xgene_erase_fw_slimimg_block(uint32_t offset, uint32_t len);
int xgene_flash_fw_slimimg_block(uint32_t offset,
						uint8_t *buffer, uint32_t len);
int xgene_fw_slimimg_read_block(uint32_t offset,
						uint8_t *buffer, uint32_t len);
int xgene_flash_scp(uint8_t *buffer, uint32_t len);
int xgene_flash_scp_block(uint32_t offset, uint8_t *buffer, uint32_t len);
int xgene_scp_read_block(uint32_t offset, uint8_t *buffer, uint32_t len);
int xgene_flash_firmware(void);

#endif /* _XGENE_FWU_H_ */
