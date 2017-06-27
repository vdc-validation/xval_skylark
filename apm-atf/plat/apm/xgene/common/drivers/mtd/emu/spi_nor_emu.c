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
#include <errno.h>
#include <platform_def.h>
#include <spi_flash.h>
#include <string.h>

#define SPI_NOR_EMU_BASE PLAT_XGENE_FW_RAM_BASE

#if MTD_DBG
#define DBG(...)	tf_printf("MTD(EMU): "__VA_ARGS__)
#else
#define DBG(...)
#endif

static spi_flash_t spi_flash;

int emu_read(spi_flash_t *flash, uint32_t offset,
			uint32_t len, uint8_t *buf)
{
	DBG("Request read at offset:0x%x, len:0x%x to buf:%p\n",
		offset, len, buf);
	memcpy((void *)buf, (void *)(uint64_t)(SPI_NOR_EMU_BASE + offset), len);

	return 0;
}

int emu_write(spi_flash_t *flash, uint32_t offset,
		uint32_t len, uint8_t *buf)
{
	DBG("Request write at offset:0x%x, len:0x%x from buf:%p\n",
		offset, len, buf);
	memcpy((void *)(uint64_t)(SPI_NOR_EMU_BASE + offset), (void *)buf, len);

	return 0;
}

int emu_erase(spi_flash_t *flash, uint32_t offset, uint32_t count)
{
	DBG("Request erase at offset:0x%x, len:0x%x\n",
		offset, count);
	memset((void *)(uint64_t)(SPI_NOR_EMU_BASE + offset), 0xFF, count);

	return 0;
}

spi_flash_t *spi_flash_chip_probe(uint32_t bus, uint32_t cs,
				  uint32_t max_hz, uint32_t spi_mode)
{
	spi_flash.spi.bus = bus;
	spi_flash.spi.cs = bus;
	spi_flash.spi.max_hz = max_hz;
	spi_flash.spi.mode = spi_mode;

	spi_flash.size = PLAT_XGENE_FW_MAX_SIZE;
	spi_flash.page_size = 256;
	spi_flash.sector_size = 65536;
	spi_flash.read = emu_read;
	spi_flash.write = emu_write;
	spi_flash.erase = emu_erase;

	return &spi_flash;
}
