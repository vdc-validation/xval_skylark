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

#define TIMEOUT				200000
#define TIMEOUT_SLEEP			10

#define DWSPI_TX_FIFO_DEPTH		256
#define DWSPI_CSR_SIZE			0x10000

#define DWSPI_CTRLR1_NDF_MAX		0xFFFF /* Maximum read 64K */
#define DWSPI_CTRLR0(base)		((base) + 0x00)
#define DWSPI_CTRLR1(base)		((base) + 0x04)
#define DWSPI_SSIENR(base)		((base) + 0x08)
#define DWSPI_MWCR(base)		((base) + 0x0C)
#define DWSPI_SER(base)			((base) + 0x10)
#define DWSPI_BAUDR(base)		((base) + 0x14)
#define DWSPI_TXFTLR(base)		((base) + 0x18)
#define DWSPI_RXFTLR(base)		((base) + 0x1C)
#define DWSPI_TXFLR(base)		((base) + 0x20)
#define DWSPI_RXFLR(base)		((base) + 0x24)
#define DWSPI_SR(base)			((base) + 0x28)
#define DWSPI_IMR(base)			((base) + 0x2C)
#define DWSPI_ISR(base)			((base) + 0x30)
#define DWSPI_RISR(base)		((base) + 0x34)
#define DWSPI_TXOICR(base)		((base) + 0x38)
#define DWSPI_RXOICR(base)		((base) + 0x3C)
#define DWSPI_RXUICR(base)		((base) + 0x40)
#define DWSPI_ICR(base)			((base) + 0x44)
#define DWSPI_DMACR(base)		((base) + 0x4C)
#define DWSPI_DMATDLR(base)		((base) + 0x50)
#define DWSPI_DMARDLR(base)		((base) + 0x54)
#define DWSPI_IDR(base)			((base) + 0x58)
#define DWSPI_SSI_VER(base)		((base) + 0x5C)
#define DWSPI_DR(base)			((base) + 0x60)

#define DWSPI_SSIENR_SSI_EN_MASK	0x1

#define DWSPI_CTRLR0_DFS_MASK		0x000f
#define DWSPI_CTRLR0_DFS_VAL		0x7
#define DWSPI_CTRLR0_SCPOL_MASK		0x0080
#define DWSPI_CTRLR0_SCPH_MASK		0x0040
#define DWSPI_CTRLR0_TMOD_MASK		0x0300
#define DWSPI_CTRLR0_TMOD_SHIFT		8
#define DWSPI_CTRLR0_TMOD_TXRX		0x0
#define DWSPI_CTRLR0_TMOD_RX		0x1
#define DWSPI_CTRLR0_TMOD_TX		0x2
#define DWSPI_CTRLR0_TMOD_EEPROM	0x3

#define DWSPI_CTRLR1_MASK		0xFFFF

#define DWSPI_SR_BUSY_MASK		0x01
#define DWSPI_SR_TFNF_MASK		0x02
#define DWSPI_SR_TFE_MASK		0x04
#define DWSPI_SR_RFNE_MASK		0x08
#define DWSPI_SR_RFF_MASK		0x10

#define DWSPI_ISR_TXEIS_MASK		0x01
#define DWSPI_ISR_RXFIS_MASK		0x10
#define DWSPI_IMR_TXEIM_MASK		0x01
#define DWSPI_IMR_RXFIM_MASK		0x10
#define DWSPI_IMR_VAL			0x0

#define DWSPI_TUNNING_DELAY		0x100000

static
uint32_t dwspi_read32(uintptr_t addr)
{
	return mmio_read_32(addr);
}

static
void dwspi_write32(uintptr_t addr, uint32_t val)
{
	mmio_write_32(addr, val);
}

static
int dwspi_host_is_enable(uintptr_t base)
{
	uint32_t dwspi_stat = dwspi_read32(DWSPI_SSIENR(base));

	if (dwspi_stat & DWSPI_SSIENR_SSI_EN_MASK)
		return 1;

	return 0;
}
static
void dwspi_enable_host(uintptr_t base)
{
	uint32_t dwspi_stat = dwspi_read32(DWSPI_SSIENR(base));

	if (!(dwspi_stat & DWSPI_SSIENR_SSI_EN_MASK))
		dwspi_write32((DWSPI_SSIENR(base)),
			dwspi_stat | DWSPI_SSIENR_SSI_EN_MASK);
}

static
void dwspi_disable_host(uintptr_t base)
{
	uint32_t dwspi_stat = dwspi_read32(DWSPI_SSIENR(base));

	if ((dwspi_stat & DWSPI_SSIENR_SSI_EN_MASK))
		dwspi_write32((DWSPI_SSIENR(base)),
			dwspi_stat & ~DWSPI_SSIENR_SSI_EN_MASK);
}

void dwspi_host_set_mode(uintptr_t base, uint32_t mode)
{
	uint32_t ctrlr0;
	int is_enabled;

	/* make sure disable host */
	is_enabled = dwspi_host_is_enable(base);
	if (is_enabled)
		dwspi_disable_host(base);

	ctrlr0 = dwspi_read32(DWSPI_CTRLR0(base));
	ctrlr0 &= ~DWSPI_CTRLR0_TMOD_MASK;
	ctrlr0 |= (DWSPI_CTRLR0_TMOD_EEPROM << DWSPI_CTRLR0_TMOD_SHIFT);
	ctrlr0 &= ~DWSPI_CTRLR0_DFS_MASK;
	/* 8-bit serial data transfer */
	ctrlr0 |= DWSPI_CTRLR0_DFS_VAL;

	if (mode & SPI_CPHA)
		ctrlr0 |= DWSPI_CTRLR0_SCPH_MASK;
	else
		ctrlr0 &= ~DWSPI_CTRLR0_SCPH_MASK;

	if (mode & SPI_CPOL)
		ctrlr0 |= DWSPI_CTRLR0_SCPOL_MASK;
	else
		ctrlr0 &= ~DWSPI_CTRLR0_SCPOL_MASK;

	if (is_enabled)
		dwspi_enable_host(base);
}

static
void dwspi_host_set_baudrate(uintptr_t base, uint32_t speed_hz)
{
	uint32_t div;
	int is_enabled;

	/* make sure disable host */
	is_enabled = dwspi_host_is_enable(base);
	if (is_enabled)
		dwspi_disable_host(base);

	div = (speed_hz) ? SYS_CLK_FREQ / speed_hz : 0xFFFE;
	if (div % 2)
		div = div - div % 2;
	if (div == 0)
		div = 2;
	dwspi_write32(DWSPI_BAUDR(base), div);

	if (is_enabled)
		dwspi_enable_host(base);
}

static
void dwspi_host_enable_slave(uintptr_t base, uint32_t slave_cs)
{
	uint32_t ser;

	assert(slave_cs < SPI_MAX_CS);

	ser = dwspi_read32(DWSPI_SER(base));
	ser = ser | (0x1 << slave_cs);
	dwspi_write32(DWSPI_SER(base), ser);
}

static
void dwspi_initialize_bus(uint32_t bus)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * bus;

	/* disable host */
	dwspi_disable_host(base);

	/* disable all interrupt */
	dwspi_write32(DWSPI_IMR(base), DWSPI_IMR_VAL);

	/* Set TX full threshold */
	dwspi_write32(DWSPI_TXFTLR(base), DWSPI_TX_FIFO_DEPTH);

	/* Set RX empty threshold */
	dwspi_write32(DWSPI_RXFTLR(base), 0);

	/* Set default mode */
	dwspi_host_set_mode(base, SPI_MODE_3);

	/* Disable chip select for all Slaves */
	dwspi_write32(DWSPI_SER(base), 0x0);
}

static
int dwspi_xfer_out(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * slave->bus;
	uint8_t *ptr;
	uint32_t timeout;
	uint32_t pos, written, len;
	uint32_t sr, old_ser, old_ctrlr0;

	timeout = TIMEOUT;
	while (dwspi_read32(DWSPI_SR(base)) & DWSPI_SR_BUSY_MASK) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		udelay(TIMEOUT_SLEEP);
	}

	/* Save chip select and disable all Slaves */
	old_ser = dwspi_read32(DWSPI_SER(base));
	dwspi_write32(DWSPI_SER(base), 0x0);

	/* Save control register */
	old_ctrlr0 = dwspi_read32(DWSPI_CTRLR0(base));

	/* Overwrite control registers */
	dwspi_disable_host(base);
	dwspi_write32(DWSPI_CTRLR0(base),
		(old_ctrlr0 & ~DWSPI_CTRLR0_TMOD_MASK) |
		(DWSPI_CTRLR0_TMOD_TXRX << DWSPI_CTRLR0_TMOD_SHIFT));
	dwspi_enable_host(base);

	/* Fill the TX FIFO */
	written = 0;
	len = cmd_count + data_count;
	ptr = cmd;
	pos = 0;
	while (written < DWSPI_TX_FIFO_DEPTH && written < len) {
		sr = dwspi_read32(DWSPI_SR(base));
		if (sr & DWSPI_SR_TFNF_MASK) {
			if (written == cmd_count) {
				ptr = data;
				pos = 0;
			}
			dwspi_write32(DWSPI_DR(base), ptr[pos]);
			pos++;
			written++;
		} else
			break;
	}

	/* Enable slave to start transfer */
	dwspi_write32(DWSPI_SER(base), old_ser);

	/* Write remaining data to TX FIFO */
	timeout = TIMEOUT;
	while (written < len) {
		sr = dwspi_read32(DWSPI_SR(base));
		if (sr & DWSPI_SR_TFNF_MASK) {
			if (written == cmd_count) {
				ptr = data;
				pos = 0;
			}
			dwspi_write32(DWSPI_DR(base), ptr[pos]);
			pos++;
			written++;
		} else {
			timeout--;
			if (!timeout)
				return -EBUSY;
			udelay(TIMEOUT_SLEEP);
		}
	}

	/* Wait till TX FIFO is not empty */
	timeout = TIMEOUT;
	while (!(dwspi_read32(DWSPI_SR(base)) & DWSPI_SR_TFE_MASK)) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		udelay(TIMEOUT_SLEEP);
	}

	/* Wait for TX to complete */
	timeout = TIMEOUT;
	while (dwspi_read32(DWSPI_SR(base)) & DWSPI_SR_BUSY_MASK) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		udelay(TIMEOUT_SLEEP);
	}

	dwspi_disable_host(base);
	dwspi_write32(DWSPI_CTRLR0(base), old_ctrlr0);
	dwspi_enable_host(base);

	return 0;
}

static
int dwspi_xfer_in(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * slave->bus;
	uint32_t timeout;
	uint32_t pos;
	uint32_t sr, old_ser, old_ctrlr0, old_ctrlr1;

	assert(cmd_count <= DWSPI_TX_FIFO_DEPTH);

	timeout = TIMEOUT;
	while (dwspi_read32(DWSPI_SR(base)) & DWSPI_SR_BUSY_MASK) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		udelay(TIMEOUT_SLEEP);
	}

	/* Save chip select and disable all Slaves */
	old_ser = dwspi_read32(DWSPI_SER(base));
	dwspi_write32(DWSPI_SER(base), 0x0);

	/* Save control registers */
	old_ctrlr0 = dwspi_read32(DWSPI_CTRLR0(base));
	old_ctrlr1 = dwspi_read32(DWSPI_CTRLR1(base));

	/* Overwrite control registers */
	dwspi_disable_host(base);
	dwspi_write32(DWSPI_CTRLR0(base),
		(old_ctrlr0 & ~DWSPI_CTRLR0_TMOD_MASK) |
		(DWSPI_CTRLR0_TMOD_EEPROM << DWSPI_CTRLR0_TMOD_SHIFT));
	dwspi_write32(DWSPI_CTRLR1(base), (data_count) ? (data_count - 1) : 0);
	dwspi_enable_host(base);

	/* Fill the TX FIFO */
	pos = 0;
	timeout = TIMEOUT;
	while (pos < cmd_count) {
		sr = dwspi_read32(DWSPI_SR(base));
		if (sr & DWSPI_SR_TFNF_MASK) {
			dwspi_write32(DWSPI_DR(base), cmd[pos]);
			pos++;
		} else {
			timeout--;
			if (!timeout)
				return -EBUSY;
			udelay(TIMEOUT_SLEEP);
		}
	}

	/* Enable slave to start transfer */
	dwspi_write32(DWSPI_SER(base), old_ser);

	/* Read from RX FIFO */
	timeout = TIMEOUT;
	pos = 0;
	while (pos < data_count) {
		sr = dwspi_read32(DWSPI_SR(base));
		if (sr & DWSPI_SR_RFNE_MASK) {
			data[pos] = (uint8_t) (dwspi_read32(DWSPI_DR(base)) & 0xff);
			pos++;
		} else {
			timeout--;
			if (!timeout)
				return -EBUSY;
			udelay(TIMEOUT_SLEEP);
		}
	}

	/* Wait for transfer to complete */
	timeout = TIMEOUT;
	while (dwspi_read32(DWSPI_SR(base)) & DWSPI_SR_BUSY_MASK) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		udelay(TIMEOUT_SLEEP);
	}

	dwspi_disable_host(base);
	dwspi_write32(DWSPI_CTRLR0(base), old_ctrlr0);
	dwspi_write32(DWSPI_CTRLR1(base), old_ctrlr1);
	dwspi_enable_host(base);

	return 0;
}

void spi_claim_bus(spi_slave_t *slave)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * slave->bus;

	dwspi_enable_host(base);
}

void spi_release_bus(spi_slave_t *slave)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * slave->bus;

	dwspi_disable_host(base);
}

int spi_setup_slave(spi_slave_t *slave)
{
	uintptr_t base = SPI0_REG_BASE + DWSPI_CSR_SIZE * slave->bus;

	/* initialize bus */
	dwspi_initialize_bus(slave->bus);

	/* set mode */
	dwspi_host_set_mode(base, slave->mode);

	/* set speed */
	dwspi_host_set_baudrate(base, slave->max_hz);

	/* enable chip select */
	dwspi_host_enable_slave(base, slave->cs);

	return 0;
}

int spi_xfer_read(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count)
{
	return dwspi_xfer_in(slave, cmd, cmd_count, data, data_count);
}

int spi_xfer_write(spi_slave_t *slave, uint8_t *cmd, uint32_t cmd_count,
		uint8_t *data, uint32_t data_count)
{
	return dwspi_xfer_out(slave, cmd, cmd_count, data, data_count);
}

uint32_t spi_get_max_read_size(spi_slave_t *slave)
{
	return (DWSPI_CTRLR1_NDF_MAX + 1);
}
