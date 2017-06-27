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
 * This module provides access to the platform Non-volatile memory for
 * storing various parameters.
 */

#include <bl_common.h>
#include <debug.h>
#include <errno.h>
#include <failsafe_proxy.h>
#include <spi_flash.h>
#include <string.h>
#include <xgene_nvparam.h>

#define NV_PARAM_SIZE		NV_PARAM_BLOCKSIZE
#ifndef NV_PARAM_BASE
/* If platform does not define non-volatile flash address, use default */
#define NV_PARAM_BASE		0xFF0000
#endif

#ifndef NV_PARAM_BASE_LAST_GOOD_KNOWN
#define NV_PARAM_BASE_LAST_GOOD_KNOWN	(NV_PARAM_BASE - NV_PARAM_SIZE)
#endif

#if XGENE_VHP
/*
 * VHP model don't map the entire SPI-NOR. For testing purpose, adjust
 * to usable base address.
 */
#define VHP_NV_ADJUST		0x200000
#else
#define VHP_NV_ADJUST		0
#endif

/*
 * Parameter entry structure. Each entry musts be 8 bytes
 */
struct entry {
	uint32_t param1;
	uint32_t acl_rd : 8;	/* Read permission */
	uint32_t acl_wr : 7;	/* Write permission */
	uint32_t valid  : 1;	/* Valid bit */
	uint32_t crc16  : 16;
} __attribute__((__packed__));

static int crc16(uint8_t *ptr, int count)
{
	int crc = 0;
	int i;

	while (--count >= 0) {
		crc = crc ^ (int) *ptr++ << 8;
		for (i = 0; i < 8; ++i) {
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
		}
	}

	return crc & 0xffff;
}

static int __plat_nvparam_get_item(uint32_t nvram_base, enum nvparam param,
				  struct entry *item)
{
	uint32_t offset;

	/* Compute the offset */
	offset = (uint32_t) param;
	offset += nvram_base;

	/* Check for invalid range */
	if (offset < nvram_base ||
	    offset >= (nvram_base + NV_PARAM_SIZE))
		return -EINVAL;

	/* Adjust for VHP model */
	offset -= VHP_NV_ADJUST;

	/* Load the entry */
	return spi_flash_read(offset, sizeof(*item), (uint8_t *) item);
}

static int __plat_nvparam_get(uint32_t nvram_base, enum nvparam param,
			     uint16_t acl_rd, uint32_t *val)
{
	struct entry item;
	uint16_t crc;
	int ret;

	ret = __plat_nvparam_get_item(nvram_base, param, &item);
	if (ret < 0)
		return ret;

	/* Check the crc16 and valid bit */
	crc = item.crc16;
	item.crc16 = 0;
	if (crc != crc16((uint8_t *) &item, sizeof(item)))
		return ENO_PARAM;
	if (!item.valid)
		return ENO_PARAM;

	/* Check for read permission */
	if (!(item.acl_rd == 0 ||
	     (item.acl_rd & acl_rd)))
		return -EPERM;

	*val = item.param1;
	return 0;
}

int plat_nvparam_get(enum nvparam param, uint16_t acl_rd, uint32_t *val)
{
	enum failsafe_status	status;
	uint32_t		nvram_base;

	if (param >= NV_PREBOOT2_PARAM_START && param <= NV_PREBOOT2_PARAM_MAX)
		return __plat_nvparam_get(NV_PARAM_BASE, param, acl_rd, val);

	if (failsafe_proxy_get_status(&status)) {
		/* Fall back to normal boot */
		status = FAILSAFE_BOOT_NORMAL;
	}

	switch (status) {
	case FAILSAFE_BOOT_NORMAL:
	case FAILSAFE_BOOT_SUCCESSFUL:
		nvram_base = NV_PARAM_BASE;
		break;
	case FAILSAFE_BOOT_LAST_KNOWN_SETTINGS:
		nvram_base = NV_PARAM_BASE_LAST_GOOD_KNOWN;
		break;
	case FAILSAFE_BOOT_DEFAULT_SETTINGS:
	default:
		return -ENO_PARAM;
	}

	return __plat_nvparam_get(nvram_base, param, acl_rd, val);
}


static int __plat_nvparam_set_item(uint32_t nvram_base, enum nvparam param,
				   struct entry *item)
{
	int ret;
	uint32_t offset;

	/* Compute the offset */
	offset = (uint32_t) param;
	offset += nvram_base;

	/* Check for invalid range */
	if (offset < nvram_base ||
	    offset >= (nvram_base + NV_PARAM_SIZE))
		return -EINVAL;

	/* Adjust for VHP model */
	offset -= VHP_NV_ADJUST;

	/* Save the entry */
	ret = spi_flash_erase(offset, sizeof(item));
	if (ret)
		return ret;

	return spi_flash_write(offset, sizeof(item), (uint8_t *) item);
}

static int __plat_nvparam_set(uint32_t nvram_base, enum nvparam param,
			      uint16_t acl_rd, uint16_t acl_wr,
			      uint32_t val, int clr)
{
	struct entry item;
	uint16_t crc;
	int ret;

	ret = __plat_nvparam_get_item(nvram_base, param, &item);
	if (ret < 0)
		return ret;

	/* Check the crc16 and valid bit */
	crc = item.crc16;
	item.crc16 = 0;
	if (crc == crc16((uint8_t *) &item, sizeof(item))) {
		/*
		 * Check if entry actually set. If the valid bit is not set,
		 * it is not an valid entry.
		 */
		if (item.valid) {
			/* Check for write permission */
			if (!(item.acl_wr == 0 ||
			     (item.acl_wr & acl_wr)))
				return -EPERM;
		}
	}

	memset(&item, 0, sizeof(item));
	if (!clr) {
		/* Generate the crc16 and set value */
		item.crc16 = 0;
		if (acl_rd)
			item.acl_rd = acl_rd | NV_PERM_ATF;
		if (acl_wr)
			item.acl_wr = acl_wr | NV_PERM_ATF;
		item.valid = 1;
		item.param1 = val;
		item.crc16 = crc16((uint8_t *) &item, sizeof(item));
	}

	return __plat_nvparam_set_item(nvram_base, param, &item);
}

static int __plat_nvparam_save_setting(uint32_t nvram_dst, uint32_t nvram_src)
{
	uint32_t param;
	struct entry item_src, item_dst;
	int ret, ret1;

	for (param = NV_PREBOOT_PARAM_START;
		param < NV_PARAM_SIZE; param += sizeof(struct entry)) {
		if (param == (NV_PREBOOT_PARAM_MAX + NV_PARAM_SIZE)) {
			/* switch to manufacture/user params */
			param = NV_MANU_USER_PARAM_START;
		}

		if (param == (NV_MANU_USER_PARAM_MAX + NV_PARAM_SIZE)) {
			/* End existing params */
			break;
		}

		ret = __plat_nvparam_get_item(nvram_src,
					 param, &item_src);

		ret1 = __plat_nvparam_get_item(nvram_dst,
					 param, &item_dst);

		if (ret && ret1) {
			/* Ignore this param */
			continue;
		}

		if (memcmp((void *) &item_src, (void *) &item_dst,
			   sizeof(struct entry))) {
			/* Set current as last good known value */
			ret = __plat_nvparam_set_item(
					nvram_dst,
					param, &item_src);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int plat_nvparam_clr(enum nvparam param, uint16_t acl_wr)
{
	return __plat_nvparam_set(NV_PARAM_BASE, param, 0, acl_wr, 0, 1);
}

int plat_nvparam_clr_all(void)
{
	int ret;

	/* Remove the preboot region */
	ret = spi_flash_erase(NV_PARAM_BASE - VHP_NV_ADJUST +
			NV_PREBOOT_PARAM_START,
			NV_PREBOOT_PARAM_REGION_SIZE);
	if (ret)
		return ret;

	/* Remove the manufacturer/user region */
	ret = spi_flash_erase(NV_PARAM_BASE - VHP_NV_ADJUST +
			NV_MANU_USER_PARAM_START,
			NV_MANU_USER_PARAM_REGION_SIZE);

	return ret;
}

int plat_nvparam_set(enum nvparam param, uint16_t acl_rd, uint16_t acl_wr,
		     uint32_t val)
{
	return __plat_nvparam_set(NV_PARAM_BASE, param, acl_rd, acl_wr, val, 0);
}

int plat_nvparam_save_last_good_known_setting(void)
{
	return __plat_nvparam_save_setting(NV_PARAM_BASE_LAST_GOOD_KNOWN,
						NV_PARAM_BASE);
}

int plat_nvparam_restore_last_good_known_setting(void)
{
	return __plat_nvparam_save_setting(NV_PARAM_BASE,
						NV_PARAM_BASE_LAST_GOOD_KNOWN);
}
