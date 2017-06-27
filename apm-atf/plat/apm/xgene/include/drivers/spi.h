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

#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

/* SPI mode flags */
#define SPI_CPHA	0x01	/* clock phase */
#define SPI_CPOL	0x02	/* clock polarity */
#define SPI_MODE_0	(0 | 0)	/* (original MicroWire) */
#define SPI_MODE_1	(0 | SPI_CPHA)
#define SPI_MODE_2	(SPI_CPOL | 0)
#define SPI_MODE_3	(SPI_CPOL | SPI_CPHA)

/**
 * struct spi_slave - Representation of a SPI slave
 *
 * @bus:                ID of the bus that the slave is attached to.
 * @cs:                 ID of the chip select connected to the slave.
 * @max_hz:             Maximum HZ that the device supported.
 * @mode:               SPI mode.
 */
typedef struct spi_slave {
	uint32_t bus;
	uint32_t cs;
	uint32_t max_hz;
	uint32_t mode;
} spi_slave_t;

void spi_claim_bus(spi_slave_t *slave);
void spi_release_bus(spi_slave_t *slave);
int spi_setup_slave(spi_slave_t *slave);
uint32_t spi_get_max_read_size(spi_slave_t *slave);
int spi_xfer_read(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count);
int spi_xfer_write(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count);

#endif /*_SPI_H_ */
