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

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include <spi.h>
#include <stdint.h>

/**
 * struct spi_flash - SPI flash structure
 *
 * @spi:                SPI slave
 * @private_data:       Private structure of flash driver
 * @size:               Total flash size
 * @sector_size:        Sector size
 *
 * @read:               Flash read ops: Read len bytes at offset into buf
 * @write:              Flash write ops: Write len bytes from buf into offset
 * @erase:              Flash erase ops: Erase len bytes from offset
 */
typedef struct spi_flash {
	spi_slave_t spi;
	uint8_t *private_data;

	uint32_t size;
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t addr_mode;

	int (*read)(struct spi_flash *flash, uint32_t offset,
		    uint32_t len, uint8_t *buf);
	int (*write)(struct spi_flash *flash, uint32_t offset,
		     uint32_t len, uint8_t *buf);
	int (*erase)(struct spi_flash *flash, uint32_t offset, uint32_t len);
} spi_flash_t;

/*
 * Probe flash chip device. It is called by spi_flash_probe function.
 *
 * @bus:          bus number.
 * @cs:           cs number.
 * @max_hz:       Maximum speed supported by flash device.
 * @spi_mode:     SPI mode used by flash device.
 *
 * @return:       NULL for fail to recognize the flash device.
 *                Flash structure for success.
 */
spi_flash_t *spi_flash_chip_probe(uint32_t bus, uint32_t cs,
				  uint32_t max_hz, uint32_t spi_mode);

/*
 * Probe flash device.
 *
 * @return:       0 for success, otherwise error code.
 */
int spi_flash_probe(void);

/*
 * Initialize spi flash subsystem.
 */
void spi_flash_initialize(void);

/*
 * Return total size of flash device.
 *
 * @return:       size of flash device in bytes.
 */
uint32_t spi_flash_get_size(void);

/*
 * Return page size of flash device.
 *
 * @return:       page size of flash device in bytes.
 */
uint32_t spi_flash_get_page_size(void);

/*
 * Return sector size of flash device.
 *
 * @return:       sector size of flash device in bytes.
 */
uint32_t spi_flash_get_sector_size(void);

/*
 * Read number of bytes from flash device.
 *
 * @offset:       offset to read.
 * @len:          len to read.
 * @buf:          buffer to contain returned data.
 *
 * @return:       0 for success, otherwise error code.
 */
int spi_flash_read(uint32_t offset, uint32_t len, uint8_t *buf);

/*
 * Write number of bytes to flash device.
 *
 * @offset:       offset to write.
 * @len:          len to write.
 * @buf:          buffer that contain the data to be written.
 *
 * @return:       0 for success, otherwise error code.
 */
int spi_flash_write(uint32_t offset, uint32_t len, uint8_t *buf);

/*
 * Erase flash region. It requires offset and len to be multiple of sector
 * size.
 *
 * @offset:       offset where to erase. Must be multiple of sector size.
 * @len:          len to erase. Must be multiple of sector size.
 *
 * @return:       0 for success, otherwise error code.
 */
int spi_flash_erase(uint32_t offset, uint32_t len);

#endif /* __SPI_FLASH_H__ */
