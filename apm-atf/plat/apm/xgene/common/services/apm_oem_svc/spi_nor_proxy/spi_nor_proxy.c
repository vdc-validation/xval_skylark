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
#include <arch.h>
#include <arch_helpers.h>
#include <apm_oem_svc.h>
#include <config.h>
#include <debug.h>
#include <errno.h>
#include <platform_def.h>
#include <spi_flash.h>
#include <spi_nor_proxy.h>
#include <xgene_fip_helper.h>
#include <xgene_fwu.h>
#include <xlat_tables.h>

enum {
	FLASH_FUNC_GET_INFO,
	FLASH_FUNC_READ,
	FLASH_FUNC_WRITE,
	FLASH_FUNC_ERASE
};
/* Currently only BL33 use this service */
static int convert_to_smpmpro_offset(uint64_t *offset, uint32_t size)
{
	uint64_t tmp;

	if (*offset >= XGENE_NS_BL33_IMAGE_OFFSET) {
		tmp = *offset - XGENE_NS_BL33_IMAGE_OFFSET;
		if (tmp >= NVRAM_SERVICE_SMPMPRO_OFFSET &&
			tmp < NVRAM_SERVICE_SMPMPRO_LIMIT) {
			*offset = tmp - NVRAM_SERVICE_SMPMPRO_OFFSET;
			return 0;
		}
	}

	return -EINVAL;
}

static int convert_to_atf_offset(uint64_t *offset, uint32_t size)
{
	uint64_t tmp;

	tmp = *offset - XGENE_NS_BL33_IMAGE_OFFSET;
	if (tmp >= NVRAM_SERVICE_ATF_OFFSET &&
			tmp < NVRAM_SERVICE_ATF_LIMIT) {
		*offset = tmp - NVRAM_SERVICE_ATF_OFFSET;
		return 0;
	}

	return -EINVAL;
}

static int convert_to_device_offset(uint64_t *offset, uint32_t size)
{
	fip_toc_entry_t		*fip_entry;
	fip_mem_info_t		fip_info;
	uint64_t		tmp;
	uuid_t			entry_uuid = UUID_NON_TRUSTED_FIRMWARE_BL33;

	fip_entry = fip_get_entry_from_uuid(&entry_uuid);
	if (!fip_entry) {
		ERROR("SPI-NOR Proxy: Failed to get FIP info\n");
		return -ENODEV;
	}

	fip_mem_get_info(&fip_info);

	tmp = *offset - XGENE_NS_BL33_IMAGE_OFFSET;
	if ((tmp + size) > (fip_entry->size)) {
		ERROR("SPI-NOR Proxy: Out of range 0x%lx 0x%x\n", tmp, size);
		return -ENODEV;
	}

	*offset = tmp + (fip_entry->offset_address +
			(fip_info.base - PLAT_XGENE_FW_RAM_BASE));

	return 0;
}

static uint64_t spi_nor_proxy_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	uint32_t size, page_size, sector_size, ret, device_base;
	uint64_t offset;
	uint8_t *buf;

	ret = 0;
	switch (x1) {
	case FLASH_FUNC_GET_INFO:
		offset = XGENE_NS_BL33_IMAGE_OFFSET;
		device_base = convert_to_device_offset(&offset, 1);
		page_size = spi_flash_get_page_size();
		sector_size = spi_flash_get_sector_size();
		SMC_RET4(handle, 0, device_base, page_size, sector_size);
		break;
	case FLASH_FUNC_READ:
		offset = x2;
		size = (uint32_t) x3;
		buf = (uint8_t *) x4;
		/* Check if the request is reading ATF or SMPMPro indirectly */
		if (!convert_to_atf_offset(&offset, size)) {
			/* ATF read request */
			ret = xgene_fw_slimimg_read_block(
					(uint32_t)offset, buf, size);
		} else if (!convert_to_smpmpro_offset(&offset, size)) {
			/* SMPMPro read request */
			ret = xgene_scp_read_block(
					(uint32_t)offset, buf, size);
		} else {
			ret = convert_to_device_offset(&offset, size);
			if (!ret)
				ret = spi_flash_read(
						(uint32_t)offset, size, buf);
		}
		flush_dcache_range((uint64_t)buf, size);
		break;
	case FLASH_FUNC_WRITE:
		offset = x2;
		size = (uint32_t) x3;
		buf = (uint8_t *) x4;
		flush_dcache_range((uint64_t)buf, size);

		/* Check if the request is writing ATF or SMPMPro indirectly */
		if (!convert_to_atf_offset(&offset, size)) {
			/* ATF write request */
			xgene_flash_fw_slimimg_block(
					(uint32_t)offset, buf, size);
		} else if (!convert_to_smpmpro_offset(&offset, size)) {
			/* SMPMPro write request */
			ret = xgene_flash_scp_block(
					(uint32_t)offset, buf, size);
		} else {
			ret = convert_to_device_offset(&offset, size);
			if (!ret)
				ret = spi_flash_write(
						(uint32_t)offset, size, buf);
		}
		break;
	case FLASH_FUNC_ERASE:
		offset = x2;
		size = (uint32_t) x3;

		/* Check if the request is erase ATF or SMPMPro indirectly */
		if (!convert_to_atf_offset(&offset, size))
			ret = xgene_erase_fw_slimimg_block(offset, size);
		else if (!convert_to_smpmpro_offset(&offset, size))
			/* No support but not an error */
			ret = 0;
		else {
			ret = convert_to_device_offset(&offset, size);
			if (!ret)
				ret = spi_flash_erase((uint32_t)offset, size);
		}
		break;
	}

	SMC_RET1(handle, (uint64_t) ((ret) ? -ETIMEDOUT : 0));
}

void spi_nor_proxy_register(void)
{
	int ret;

	ret = spi_flash_probe();
	if (ret)
		return;

	ret = smc64_oem_svc_register(SMC_FLASH, &spi_nor_proxy_handler);

	if (!ret)
		INFO("SPI NOR proxy service registered\n");
}
