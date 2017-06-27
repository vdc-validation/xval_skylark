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

#include <assert.h>
#include <config.h>
#include <debug.h>
#include <delay_timer.h>
#include <errno.h>
#include <mmio.h>
#include <platform_def.h>
#include <spi.h>
#include <spi_flash.h>

#undef DBG
#undef ERROR
#if MTD_DBG
#define DBG(...)		tf_printf("MTD(STMICRO): "__VA_ARGS__)
#else
#define DBG(...)
#endif

#define ERROR(...)		tf_printf("MTD(STMICRO) Error: "__VA_ARGS__)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

#define SIZE_16MB		(16 * 1024 * 1024)

#define TIMEOUT			200000
#define TIMEOUT_SLEEP		10

#define CMD_READ_ID		0x9f

/* M25Pxx-specific commands */
#define CMD_M25PXX_WREN		0x06  /* Write Enable */
#define CMD_M25PXX_WRDI		0x04  /* Write Disable */
#define CMD_M25PXX_RDSR		0x05  /* Read Status Register */
#define CMD_M25PXX_WRSR		0x01  /* Write Status Register */
#define CMD_M25PXX_READ		0x03  /* Read Data Bytes */
#define CMD_M25PXX_FAST		0x0b  /* Read Data Bytes at Higher Speed */
#define CMD_M25PXX_PP		0x02  /* Page Program */
#define CMD_M25PXX_SE		0xd8  /* Sector Erase */
#define CMD_M25PXX_BE		0xc7  /* Bulk Erase */
#define CMD_M25PXX_DP		0xb9  /* Deep Power-down */
#define CMD_M25PXX_RES		0xab  /* Release from DP, and Read Signature */
#define CMD_M25PXX_RFSR		0x70
#define CMD_M25PXX_4BYTE_ADDR_EN	\
				0xB7  /* Enable 4-byte address mode */
#define CMD_M25PXX_4BYTE_ADDR_DIS	\
				0xE9  /* Disable 4-byte address mode */
#define STMICRO_SR_WIP		(1 << 0)  /* Write-in-Progress */
#define STMICRO_SR_WEL		(1 << 1)  /* Write-enable-latch */

#define STMICRO_MAX_NAME_SIZE	10

#define STM_ID_N25Q256		0x19
#define STM_ID_N25Q128		0x18

typedef struct flash_param {
	uint8_t id_code;
	uint16_t page_size;
	uint16_t page_per_sector;
	uint16_t num_sector;
	uint8_t name[STMICRO_MAX_NAME_SIZE];
} flash_param_t;

flash_param_t spi_flash_table[] = {
	{
		.id_code = STM_ID_N25Q256,
		.page_size = 256,
		.page_per_sector = 256,
		.num_sector = 512,
		.name = "N25Q256",
	},
	{
		.id_code = STM_ID_N25Q128,
		.page_size = 256,
		.page_per_sector = 256,
		.num_sector = 256,
		.name = "N25Q128",
	},
};

static spi_flash_t spi_flash;

int stmicro_set_write_mode(spi_flash_t *flash, int enable)
{
	uint8_t cmd;

	if (enable)
		cmd = CMD_M25PXX_WREN;
	else
		cmd = CMD_M25PXX_WRDI;

	return spi_xfer_write(&flash->spi, &cmd, 1, NULL, 0);
}

int stmicro_set_4byte_addr(spi_flash_t *flash, int enable)
{
	uint8_t cmd;
	int ret;

	ret = stmicro_set_write_mode(flash, 1);
	if (ret)
		return ret;

	if (enable)
		cmd = CMD_M25PXX_4BYTE_ADDR_EN;
	else
		cmd = CMD_M25PXX_4BYTE_ADDR_DIS;

	ret = spi_xfer_write(&flash->spi,
			&cmd, 1, NULL, 0);
	if (ret)
		return ret;

	return stmicro_set_write_mode(flash, 0);
}

int stmicro_wait_ready(spi_flash_t *flash, int flag)
{
	int ret;
	uint8_t cmd;
	uint8_t status;
	uint32_t timeout;
	uint32_t polling_mask;
	uint32_t status_mask;

	if (flag) {
		polling_mask = STMICRO_SR_WEL;
		status_mask = STMICRO_SR_WEL;
	} else {
		polling_mask = STMICRO_SR_WIP;
		status_mask = 0;
	}

	timeout = TIMEOUT;
	cmd = CMD_M25PXX_RDSR;
	do {
		ret = spi_xfer_read(&flash->spi, &cmd, 1, &status, 1);
		if (ret)
			return ret;

		if ((status & polling_mask) == status_mask)
			return 0;

		timeout--;
		udelay(TIMEOUT_SLEEP);
	} while (timeout);

	return -EBUSY;
}

int stmicro_read(spi_flash_t *flash, uint32_t offset,
			uint32_t len, uint8_t *buf)
{
	flash_param_t *param;
	uint8_t cmd[5];
	uint32_t read_limit, chunk, read;
	int ret = 0;

	DBG("Request read at offset 0x%x len 0x%x buf %p\n",
		offset, len, buf);
	param = (flash_param_t *) flash->private_data;
	read_limit = param->page_size * param->page_per_sector;

	if (spi_get_max_read_size(&flash->spi) < read_limit)
		read_limit = spi_get_max_read_size(&flash->spi);

	spi_claim_bus(&flash->spi);

	read = 0;
	while (read < len) {
		chunk = (len - read) > read_limit ? read_limit : (len - read);
		cmd[0] = CMD_M25PXX_READ;
		cmd[1] = offset >> (flash->addr_mode - 1) * 8;
		cmd[2] = offset >> (flash->addr_mode - 2) * 8;
		cmd[3] = offset >> (flash->addr_mode - 3) * 8;
		cmd[4] = offset;

		ret = spi_xfer_read(&flash->spi,
				cmd, flash->addr_mode + 1, buf + read, chunk);
		if (ret) {
			DBG("Read flash failed at offset %d\n", offset);
			break;
		}
		read += chunk;
		offset += chunk;
	}

	if (!ret)
		DBG("Read flash with length %d successfully\n", len);
	else
		DBG("Read flash with length %d failed\n", len);

	spi_release_bus(&flash->spi);

	return ret;
}

int stmicro_write(spi_flash_t *flash, uint32_t offset,
		uint32_t len, uint8_t *buf)
{
	flash_param_t *param;
	uint8_t cmd[5];
	uint32_t write_limit, chunk, write;
	int ret = 0;

	DBG("Request write at offset 0x%x len 0x%x buf %p\n",
		offset, len, buf);

	param = (flash_param_t *) flash->private_data;
	write_limit = param->page_size;

	spi_claim_bus(&flash->spi);

	write = 0;
	while (write < len) {
		write_limit = (offset % param->page_size) ?
			(param->page_size - (offset % param->page_size)) :
			param->page_size;
		chunk = (len - write) > write_limit ?
				write_limit : (len - write);

		ret = stmicro_set_write_mode(flash, 1);
		if (ret)
			break;

		ret = stmicro_wait_ready(flash, 1);
		if (ret)
			break;

		cmd[0] = CMD_M25PXX_PP;
		cmd[1] = offset >> (flash->addr_mode - 1) * 8;
		cmd[2] = offset >> (flash->addr_mode - 2) * 8;
		cmd[3] = offset >> (flash->addr_mode - 3) * 8;
		cmd[4] = offset;

		ret = spi_xfer_write(&flash->spi, cmd, flash->addr_mode + 1,
				buf + write, chunk);
		if (ret) {
			DBG("Write flash failed at offset %d\n", offset);
			break;
		}

		ret = stmicro_wait_ready(flash, 0);
		if (ret)
			break;

		write += chunk;
		offset += chunk;
	}

	if (!ret)
		DBG("Write flash with length %d successfully\n", len);
	else
		DBG("Write flash with length %d failed\n", len);

	spi_release_bus(&flash->spi);

	return ret;
}

int stmicro_erase(spi_flash_t *flash, uint32_t offset, uint32_t count)
{
	flash_param_t *param;
	uint8_t cmd[5];
	uint8_t *tmp_buf;
	uint32_t sector_size, erased, tmp_size, tmp_offset;
	int ret = 0;

	param = (flash_param_t *) flash->private_data;

	DBG("Request erase at offset 0x%x len 0x%x\n",
		offset, count);

	sector_size = param->page_per_sector * param->page_size;
	tmp_buf = (uint8_t *) PLAT_XGENE_FLASH_BUF_BASE;
	if (sector_size > PLAT_XGENE_FLASH_BUF_SIZE) {
		ERROR("Size of temp buffer is smaller than sector size\n");
		return -EINVAL;
	}

	spi_claim_bus(&flash->spi);

	if (offset % sector_size)
		DBG("Erase offset is not multiple of sector size\n");

	if (count % sector_size)
		DBG("Erase length is not multiple of sector size\n");

	erased = 0;
	while (erased < count) {
		tmp_size = offset % sector_size;
		if (tmp_size || ((count - erased) < sector_size)) {
			offset = (offset / sector_size) * sector_size;
			ret = stmicro_read(flash, offset, sector_size, tmp_buf);
			if (ret)
				break;
		}

		ret = stmicro_set_write_mode(flash, 1);
		if (ret)
			break;

		ret = stmicro_wait_ready(flash, 1);
		if (ret)
			break;

		cmd[0] = CMD_M25PXX_SE;
		cmd[1] = offset >> (flash->addr_mode - 1) * 8;
		cmd[2] = offset >> (flash->addr_mode - 2) * 8;
		cmd[3] = offset >> (flash->addr_mode - 3) * 8;
		cmd[4] = offset;

		ret = spi_xfer_write(&flash->spi, cmd,
					flash->addr_mode + 1, NULL, 0);
		if (ret) {
			DBG("Erase flash failed at offset %d\n", offset);
			break;
		}

		ret = stmicro_wait_ready(flash, 0);
		if (ret)
			break;

		if (tmp_size) {
			ret = stmicro_write(flash, offset,
						tmp_size, tmp_buf);
			if (ret)
				break;
		}

		if ((count - erased) < (sector_size - tmp_size)) {
			tmp_offset = count - erased + tmp_size;
			tmp_size = sector_size - tmp_offset;
			ret = stmicro_write(flash, offset + tmp_offset,
						tmp_size, tmp_buf + tmp_offset);
			break;
		}

		erased += (sector_size - tmp_size);
		offset += sector_size;
	}

	if (!ret)
		DBG("Erase flash with length %d successfully\n", count);
	else
		DBG("Erase flash with length %d failed\n", count);

	spi_release_bus(&flash->spi);

	return ret;
}

spi_flash_t *spi_flash_chip_probe(uint32_t bus, uint32_t cs,
				  uint32_t max_hz, uint32_t spi_mode)
{
	uint32_t i;
	uint8_t id_code[5], cmd;

	spi_flash.spi.bus = bus;
	spi_flash.spi.cs = bus;
	spi_flash.spi.max_hz = max_hz;
	spi_flash.spi.mode = spi_mode;

	if (spi_setup_slave(&spi_flash.spi)) {
		ERROR("Failed to setup SPI slave\n");
		return NULL;
	}

	spi_claim_bus(&spi_flash.spi);

	/* Read ID codes to make sure we are handling correct device */
	cmd = CMD_READ_ID;
	if (spi_xfer_read(&spi_flash.spi,
			  &cmd, 1, id_code, sizeof(id_code))) {
		ERROR("Failed to read ID code\n");
		return NULL;
	}

	DBG("Got Id Code: %02x %02x %02x %02x %02x\n", id_code[0],
		id_code[1], id_code[2], id_code[3], id_code[4]);

	if (id_code[0] != 0x20 && id_code[0] != 0xef) {
		ERROR("Unsupported manufacturer %02x\n", id_code[0]);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(spi_flash_table); i++) {
		if (spi_flash_table[i].id_code == id_code[2])
			break;
	}

	if (i == ARRAY_SIZE(spi_flash_table)) {
		ERROR("Unsupported STMicro ID %02x\n", id_code[1]);
		return NULL;
	}

	spi_flash.size = spi_flash_table[i].page_size *
			spi_flash_table[i].page_per_sector *
			spi_flash_table[i].num_sector;
	spi_flash.page_size = spi_flash_table[i].page_size;
	spi_flash.sector_size = spi_flash_table[i].page_size *
			spi_flash_table[i].page_per_sector;
	spi_flash.private_data = (uint8_t *)&spi_flash_table[i];
	spi_flash.read = stmicro_read;
	spi_flash.write = stmicro_write;
	spi_flash.erase = stmicro_erase;

	DBG("Detected %s with page size %u, total %u bytes\n",
		spi_flash_table[i].name,
		spi_flash_table[i].page_size, spi_flash.size);

	spi_flash.addr_mode = (spi_flash.size > SIZE_16MB) ? 4 : 3;
	stmicro_set_4byte_addr(&spi_flash, spi_flash.addr_mode == 4);

	return &spi_flash;
}
