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
#include <mmio.h>
#include <platform_def.h>
#include <spi_flash.h>

static spi_flash_t *spi_flash;

int spi_flash_probe(void)
{
	spi_flash = spi_flash_chip_probe(SPI_NOR_BUS, SPI_NOR_CS,
					SPI_NOR_MAX_HZ, SPI_NOR_MODE);
	if (!spi_flash)
		return -EINVAL;

	return 0;
}

uint32_t spi_flash_get_size(void)
{
	assert(spi_flash);
	return spi_flash->size;
}

uint32_t spi_flash_get_page_size(void)
{
	assert(spi_flash);
	return spi_flash->page_size;
}

uint32_t spi_flash_get_sector_size(void)
{
	assert(spi_flash);
	return spi_flash->sector_size;
}

int spi_flash_read(uint32_t offset, uint32_t len, uint8_t *buf)
{
	assert(spi_flash);
	return spi_flash->read(spi_flash, offset, len, buf);
}

int spi_flash_write(uint32_t offset, uint32_t len, uint8_t *buf)
{
	assert(spi_flash);
	return spi_flash->write(spi_flash, offset, len, buf);
}

int spi_flash_erase(uint32_t offset, uint32_t len)
{
	assert(spi_flash);
	return spi_flash->erase(spi_flash, offset, len);
}
