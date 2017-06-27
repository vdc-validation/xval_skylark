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
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <errno.h>
#include <uuid.h>
#include <platform_def.h>
#include <smpro_i2c.h>
#include <spi_flash.h>
#include <string.h>
#include <xgene_fwu.h>
#include <xgene_private.h>
#include <xgene_slimimg.h>

#define FWU_RECOVERY_SIGNATURE		0xfeedfeed
#define FWU_HEADER_SIGNATURE		0x46550000	/* 'F' 'W' 'U' '' */

/* FW image in slimimg format to be upgraded*/
#define UUID_XGENE_UPDATE_FW \
{0xea7b1ffc, 0x47a3, 0x11e6, 0xbe, 0xb8, {0x9e, 0x71, 0x12, 0x8c, 0xae, 0x77} }

/* SCP (SMPMPro) firmware to be upgraded */
#define UUID_XGENE_UPDATE_SCP \
{0xde5ca148, 0x47a6, 0x11e6, 0xbe, 0xb8, {0x9e, 0x71, 0x12, 0x8c, 0xae, 0x77} }

#define SLIMIMG_FWU_FIP_NAME		"fwu_fip"
#define SLIMIMG_FIP_NAME			"fip"

/*
 * Memory layout in case of recovery mode
 * -----------------------------------------
 * | FIP image                              |
 * -----------------------------------------
 */

/*
 * Memory layout for FWU description
 * ------------------------------------------
 * | Number of FWU request (4 bytes)        |
 * ------------------------------------------
 * | Total length of all FWU content        |
 * ------------------------------------------
 * | FWU header description for request 1st |
 * ------------------------------------------
 * | FWU content of request 1st             |
 * ------------------------------------------
 * .........................................
 * ------------------------------------------
 * | FWU header description for request Nth |
 * ------------------------------------------
 * | FWU content of request Nth             |
 * ------------------------------------------
 */

/*
 * FWU description format
 */
typedef struct {
	uint32_t	num;
	uint32_t	length;
} fwu_desc_t;

/*
 * FWU item header format
 */
typedef struct {
	uuid_t		name_uuid;
	uint32_t	fw_size;
} fwu_item_header_t;

static const uuid_t uuid_update_fw = UUID_XGENE_UPDATE_FW;
static const uuid_t uuid_update_scp = UUID_XGENE_UPDATE_SCP;

/*
 * Check if need to jump to FW upgrade process in trusted boot mode.
 */
int plat_xgene_is_fw_upgrade_mode(void)
{
#if TRUSTED_BOARD_BOOT
	uint32_t val = *(uint32_t *)PLAT_XGENE_FW_UPGRADE_REQUEST_BASE;

	return (val == FWU_HEADER_SIGNATURE) ? 1:0;
#else
	return 0;
#endif
}

/*
 * Check if we are in recovery mode
 */
int plat_xgene_is_recovery_mode(void)
{
	uint32_t val = *(uint32_t *)PLAT_XGENE_FW_UPGRADE_REQUEST_BASE;

	return (val == FWU_RECOVERY_SIGNATURE) ? 1:0;
}

/*
 * Set FW upgrade mode in trusted boot mode.
 */
void plat_xgene_set_fw_upgrade_mode(void)
{
#if TRUSTED_BOARD_BOOT
	*(uint32_t *)PLAT_XGENE_FW_UPGRADE_REQUEST_BASE = FWU_HEADER_SIGNATURE;
#endif
}

void xgene_init_fwu(void)
{
	fwu_desc_t *fwu_desc = (fwu_desc_t *)NS_FWU_BASE;

	fwu_desc->num = 0;
	fwu_desc->length = 0;

	spi_flash_probe();
}

/*
 * Add new FW upgrade request to pending list
 */
int xgene_add_fw_upgrade_request(uuid_t *uuid, uint32_t offset,
				uint8_t *base, uint32_t size)
{
	int					idx;
	uint8_t				*ptr;
	fwu_desc_t			*fwu_desc;
	fwu_item_header_t	fwu_item, *item_ptr;

	assert(base);
	if (size == 0)
		return -EINVAL;

	if (!plat_xgene_is_fw_upgrade_mode())
		plat_xgene_set_fw_upgrade_mode();

	fwu_desc = (fwu_desc_t *)NS_FWU_BASE;
	item_ptr = (fwu_item_header_t *)
			((uint8_t *)fwu_desc + sizeof(fwu_desc_t));
	for (idx = 0; idx < fwu_desc->num; idx++) {
		if (strncmp((const char *)&item_ptr->name_uuid,
				(const char *)uuid,
				sizeof(uuid_t)) == 0) {
			if (idx != (fwu_desc->num - 1))
				return -EINVAL;
			break;
		}

		item_ptr = (fwu_item_header_t *)
				((uint8_t *)item_ptr +
				sizeof(fwu_item_header_t) + item_ptr->fw_size);
	}

	if (idx == fwu_desc->num) {
		/* Create new entry */
		if (offset != 0)
			return -EINVAL;

		ptr = (uint8_t *)fwu_desc +
				sizeof(fwu_desc_t) + fwu_desc->length;
		memcpy((void *)&fwu_item.name_uuid,
				(void *)uuid, sizeof(uuid_t));
		fwu_item.fw_size = size;
		memcpy((void *)ptr, (void *)&fwu_item,
				sizeof(fwu_item_header_t));
		ptr += sizeof(fwu_item_header_t);
		memcpy((void *)ptr, base, size);

		fwu_desc->num += 1;
		fwu_desc->length += size + sizeof(fwu_item_header_t);
	} else {
		/* Append data */
		if (offset != item_ptr->fw_size)
			return -EINVAL;

		ptr = (uint8_t *)item_ptr +  item_ptr->fw_size;
		memcpy((void *)ptr, (void *)base, size);
		item_ptr->fw_size += size;
	}

	return 0;
}

/*
 * Search for FIP with name inside slimimg format
 */
int xgene_search_fip_name_slimimg(uint64_t slimimg_base,
						const char *name,
						uint64_t *base, uint32_t *len)
{
	slimimg_header_t	*slimimg_header_ptr;
	slimimg_file_t		*slimimg_file_ptr;
	int					idx;

	slimimg_header_ptr = (slimimg_header_t *)slimimg_base;
	if (slimimg_header_ptr->signature != SLIMIMG_SIGNATURE)
		return -1;

	slimimg_file_ptr =
		(slimimg_file_t *)(slimimg_base + sizeof(slimimg_header_t));

	for (idx = 0; idx < slimimg_header_ptr->file_count; idx++) {
		if (strcmp(slimimg_file_ptr->file_name,
				name) == 0)
			break;
		slimimg_file_ptr++;
	}

	if (idx == slimimg_header_ptr->file_count)
		return -1;

	/* found fip image */
	*base = (uint64_t)(slimimg_base + slimimg_file_ptr->file_offset);
	*len = slimimg_file_ptr->file_size;

	return 0;
}


/*
 * Search for FWU FIP image inside slimimg format
 */
int xgene_search_fwu_fip_slimimg(uint64_t slimimg_base,
						uint64_t *base, uint32_t *len)
{
	return xgene_search_fip_name_slimimg(slimimg_base,
				SLIMIMG_FWU_FIP_NAME, base, len);
}

/*
 * Search for FIP image inside slimimg format
 */
int xgene_search_fip_slimimg(uint64_t slimimg_base,
						uint64_t *base, uint32_t *len)
{
	return xgene_search_fip_name_slimimg(slimimg_base,
				SLIMIMG_FIP_NAME, base, len);
}

/*
 *  Search for FWU FIP image from pending list
 */
int xgene_search_fwu_fip(uint64_t *base, uint32_t *len)
{
	fwu_item_header_t	*item_ptr;
	fwu_desc_t			*fwu_desc;
	int					idx;

	fwu_desc = (fwu_desc_t *)NS_FWU_BASE;
	item_ptr = (fwu_item_header_t *)
			((uint8_t *)fwu_desc + sizeof(fwu_desc_t));
	for (idx = 0; idx < fwu_desc->num; idx++) {
		if (strncmp((const char *)&item_ptr->name_uuid,
				(const char *)&uuid_update_fw,
				sizeof(uuid_update_fw)) == 0) {
			return xgene_search_fwu_fip_slimimg((uint64_t)item_ptr +
						sizeof(fwu_item_header_t), base, len);
		}

		item_ptr = (fwu_item_header_t *)
		((uint8_t *)item_ptr + sizeof(fwu_item_header_t) + item_ptr->fw_size);
	}

	return -1;
}

int xgene_erase_fw_slimimg_block(uint32_t offset, uint32_t len)
{
#if !TRUSTED_BOARD_BOOT
	return spi_flash_erase(offset, len);
#else
	return 0;
#endif
}

int xgene_flash_fw_slimimg_block(uint32_t offset,
				uint8_t *buffer, uint32_t len)
{
#if !TRUSTED_BOARD_BOOT
	return spi_flash_write(offset, len, buffer);
#else
	/* Add update request to pending list. */
	return xgene_add_fw_upgrade_request(&uuid_update_fw,
					offset, buffer, len);
#endif
}

int xgene_fw_slimimg_read_block(uint32_t offset,
				uint8_t *buffer, uint32_t len)
{
	/* Firmware slimimg always start from 0 */
	return spi_flash_read(offset, len, buffer);
}

/*
 * Flash FW image in slimimg format
 */
int xgene_flash_fw_slimimg(uint8_t *slimimg_base, uint32_t len)
{
	int ret;
	uint32_t offset;

	VERBOSE("FWU: Upgrading FW image at 0x%p, size 0x%x\n",
			slimimg_base, len);

	offset = 0;
	ret = xgene_erase_fw_slimimg_block(offset, len);
	if (ret) {
		ERROR("FWU: Failed to erase FW image at 0x%x len %d\n",
			offset, len);
		return ret;
	}

	ret = xgene_flash_fw_slimimg_block(offset, slimimg_base, len);
	if (ret)
		ERROR("FWU: Failed to write FW image to 0x%x len %d\n",
			offset, len);

	return ret;
}

int xgene_fw_slimimg_copy_to_mem(void)
{
#if XGENE_VHP
	return 0;
#else
	int		ret;
	uint8_t	*buf;

	/* Copy FW image from non volatile mem to RAM */
	buf = (uint8_t *)PLAT_XGENE_FW_RAM_BASE;
	ret = spi_flash_probe();
	if (!ret)
		ret = spi_flash_read(0, PLAT_XGENE_FW_MAX_SIZE, buf);

	return ret;
#endif
}

int xgene_flash_scp_block(uint32_t offset, uint8_t *buffer, uint32_t len)
{
#if !TRUSTED_BOARD_BOOT
	struct smpro_i2c_op i2c_op;
	uint32_t write_len;

	/* SmPro I2C protocol */
	i2c_op.proto = SMPRO_I2C_I2C;
	i2c_op.bus = I2C_EEPROM_BUS_NUM;
	i2c_op.chip_addr = I2C_EEPROM_SMPRO_ADDR;
	/* Byte access */
	i2c_op.chip_addr_len = 0;
	while (len > 0) {
		write_len = (len < I2C_MAX_BLOCK_DATA)
					? len : I2C_MAX_BLOCK_DATA;
		if (offset >= I2C_EEPROM_MAX_SIZE) {
			/* Switch page to PMPro */
			i2c_op.chip_addr = I2C_EEPROM_PMPRO_ADDR;
			offset -= I2C_EEPROM_MAX_SIZE;
		}
		i2c_op.addr_len = (offset <= 0xFF) ? 1 :
				(offset <= 0xFFFF ? 2 : 4);
		i2c_op.addr = offset;
		i2c_op.data = (uint64_t) buffer;
		i2c_op.data_len = write_len;

		if (plat_smpro_i2c_write(&i2c_op)) {
			/* Failed to write */
			return -EBUSY;
		}
		offset += write_len;
		buffer += write_len;
		len -= write_len;
	}

	return 0;
#else
	/* Add update request to pending list. */
	return xgene_add_fw_upgrade_request(&uuid_update_scp,
					offset, buffer, len);
#endif
}

/*
 * Read SCP image
 */
int xgene_scp_read_block(uint32_t offset, uint8_t *buffer, uint32_t len)
{
	struct smpro_i2c_op i2c_op;
	uint32_t read_len;

	/* SmPro I2C protocol */
	i2c_op.proto = SMPRO_I2C_I2C;
	i2c_op.bus = I2C_EEPROM_BUS_NUM;
	i2c_op.chip_addr = I2C_EEPROM_SMPRO_ADDR;
	/* Byte access */
	i2c_op.chip_addr_len = 0;
	while (len > 0) {
		read_len = (len < I2C_MAX_BLOCK_DATA)
					? len : I2C_MAX_BLOCK_DATA;
		if (offset >= I2C_EEPROM_MAX_SIZE) {
			/* Switch page to PMPro */
			i2c_op.chip_addr = I2C_EEPROM_PMPRO_ADDR;
			offset -= I2C_EEPROM_MAX_SIZE;
		}
		i2c_op.addr_len = (offset <= 0xFF) ? 1 :
				(offset <= 0xFFFF ? 2 : 4);
		i2c_op.addr = offset;
		i2c_op.data = (uint64_t) buffer;
		i2c_op.data_len = read_len;

		if (plat_smpro_i2c_read(&i2c_op)) {
			/* Failed to read */
			return -EBUSY;
		}
		offset += read_len;
		buffer += read_len;
		len -= read_len;
	}

	return 0;
}

/*
 * Flash SCP image
 */
int xgene_flash_scp(uint8_t *buffer, uint32_t len)
{
	int ret;
	uint32_t offset;

	VERBOSE("FWU: Upgrading SCP image at 0x%p, size 0x%x\n",
			buffer, len);

	offset = 0;
	ret = xgene_flash_scp_block(offset, buffer, len);
	if (ret)
		ERROR("FWU: Failed to write SCP image to 0x%x len %d\n",
					offset, len);

	return ret;
}

/*
 * Flash images that be called from BL2U
 */
int xgene_flash_firmware(void)
{
	fwu_item_header_t	*item_ptr;
	fwu_desc_t			*fwu_desc;
	int					idx, rc;

	fwu_desc = (fwu_desc_t *)NS_FWU_BASE;
	item_ptr = (fwu_item_header_t *)
			((uint8_t *)fwu_desc + sizeof(fwu_desc_t));
	for (idx = 0; idx < fwu_desc->num; idx++) {
		if (strncmp((const char *)&item_ptr->name_uuid,
				(const char *)&uuid_update_fw,
				sizeof(uuid_update_fw)) == 0) {
			rc = xgene_flash_fw_slimimg((uint8_t *)item_ptr +
					sizeof(fwu_item_header_t), item_ptr->fw_size);
			if (rc) {
				ERROR("FWU: Flash Firmware failed\n");
				return rc;
			}
		}

		if (strncmp((const char *)&item_ptr->name_uuid,
				(const char *)&uuid_update_scp,
				sizeof(uuid_update_scp)) == 0) {
			rc = xgene_flash_scp((uint8_t *)item_ptr +
					sizeof(fwu_item_header_t), item_ptr->fw_size);
			if (rc) {
				ERROR("FWU: Flash SCP Firmware failed\n");
				return rc;
			}
		}

		item_ptr = (fwu_item_header_t *)
		((uint8_t *)item_ptr + sizeof(fwu_item_header_t) + item_ptr->fw_size);
	}

	return 0;
}
