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

#include <stdint.h>
#include <debug.h>
#include <uuid.h>
#include <spi_flash.h>
#include <string.h>
#include <platform_def.h>
#include <xgene_fip_helper.h>
#include <xgene_fwu.h>

int fip_mem_get_info(fip_mem_info_t *info)
{
#if XGENE_VHP
	info->base = PLAT_XGENE_FW_RAM_BASE;
	info->len = PLAT_XGENE_FW_MAX_SIZE;

	return 0;
#else
	uint64_t base;
	uint32_t len;
	int ret;

	ret = xgene_search_fip_slimimg(PLAT_XGENE_FW_RAM_BASE,
			&base, &len);
	if (!ret) {
		info->base = base;
		info->len = len;
	}

	return ret;
#endif
}

fip_toc_entry_t *fip_get_entry_from_uuid(const uuid_t *uuid)
{
	fip_mem_info_t fip_info;
	fip_toc_entry_t *entry;

	fip_mem_get_info(&fip_info);
	entry = (fip_toc_entry_t *)
		(fip_info.base + sizeof(fip_toc_header_t));
	while ((uintptr_t)entry < fip_info.base + fip_info.len) {
		if (memcmp((void *)&entry->uuid,
				(void *)uuid, sizeof(uuid_t)) == 0)
			return entry;
		entry += 1;
	}

	return NULL;
}
