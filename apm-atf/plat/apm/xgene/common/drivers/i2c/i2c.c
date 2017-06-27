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

#include <arch_helpers.h>
#include <bl_common.h>
#include <clk.h>
#include <config.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <i2c.h>
#include <mmio.h>
#include <platform_def.h>

#undef DBG
#undef ERROR

#if I2C_DBG
#define DBG(...)			tf_printf("I2C: "__VA_ARGS__)
#else
#define DBG(...)
#endif

#define ERROR(...)			tf_printf("I2C ERROR: "__VA_ARGS__)

#ifndef BIT
#define BIT(nr)				(1 << (nr))
#endif

/*
 * Registers
 */
#define DW_IC_CON			0x0
#define  DW_IC_CON_MASTER		BIT(0)
#define  DW_IC_CON_SPEED_STD		BIT(1)
#define  DW_IC_CON_SPEED_FAST		BIT(2)
#define  DW_IC_CON_10BITADDR_MASTER	BIT(4)
#define  DW_IC_CON_RESTART_EN		BIT(5)
#define  DW_IC_CON_SLAVE_DISABLE	BIT(6)
#define DW_IC_TAR			0x4
#define  DW_IC_TAR_10BITS		BIT(12)
#define DW_IC_SAR			0x8
#define DW_IC_DATA_CMD			0x10
#define  DW_IC_DATA_CMD_RESTART		BIT(10)
#define  DW_IC_DATA_CMD_STOP		BIT(9)
#define  DW_IC_DATA_CMD_CMD		BIT(8)
#define  DW_IC_DATA_CMD_DAT_MASK	0xFF
#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20
#define DW_IC_HS_SCL_HCNT		0x24
#define DW_IC_HS_SCL_LCNT		0x28
#define DW_IC_INTR_STAT			0x2c
#define DW_IC_INTR_MASK			0x30
#define  DW_IC_INTR_RX_UNDER		BIT(0)
#define  DW_IC_INTR_RX_OVER		BIT(1)
#define  DW_IC_INTR_TX_EMPTY		BIT(4)
#define  DW_IC_INTR_TX_ABRT		BIT(6)
#define  DW_IC_INTR_ACTIVITY		BIT(8)
#define  DW_IC_INTR_STOP_DET		BIT(9)
#define  DW_IC_INTR_START_DET		BIT(10)
#define  DW_IC_ERR_CONDITION \
	(DW_IC_INTR_RX_UNDER | DW_IC_INTR_RX_OVER | DW_IC_INTR_TX_ABRT)
#define DW_IC_RAW_INTR_STAT		0x34
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_RX_UNDER		0x44
#define DW_IC_CLR_RX_OVER		0x48
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_CLR_ACTIVITY		0x5c
#define DW_IC_CLR_STOP_DET		0x60
#define DW_IC_CLR_START_DET		0x64
#define DW_IC_ENABLE			0x6c
#define DW_IC_STATUS			0x70
#define  DW_IC_STATUS_ACTIVITY		BIT(0)
#define  DW_IC_STATUS_TFE		BIT(2)
#define  DW_IC_STATUS_RFNE		BIT(3)
#define  DW_IC_STATUS_MST_ACTIVITY	BIT(5)
#define DW_IC_TXFLR			0x74
#define DW_IC_RXFLR			0x78
#define DW_IC_SDA_HOLD			0x7c
#define DW_IC_TX_ABRT_SOURCE		0x80
#define DW_IC_ENABLE_STATUS		0x9c
#define DW_IC_COMP_PARAM_1		0xf4
#define DW_IC_COMP_TYPE			0xfc
#define SB_DW_IC_CON			0xa8
#define SB_DW_IC_SCL_TMO_CNT		0xac
#define SB_DW_IC_RX_PEC			0xb0
#define SB_DW_IC_ACK			0xb4
#define SB_DW_IC_FLG			0xb8
#define SB_DW_IC_FLG_CLR		0xbc
#define SB_DW_IC_INTR_STAT		0xc0
#define SB_DW_IC_INTR_STAT_MASK		0xc4
#define SB_DW_IC_DEBUG_SEL		0xec
#define SB_DW_IC_ACK_DEBUG		0xf0

#define DW_SIGNATURE			0x44570000  /* 'D' 'W' */
#define DW_LE_BUS			0x1234

/*
 * Status codes
 */
#define STATUS_IDLE			0x0
#define STATUS_WRITE_IN_PROGRESS	0x1
#define STATUS_READ_IN_PROGRESS		0x2

/* Timeout and retry values */
#define DW_BUS_WAIT_TIMEOUT		20
#define DW_RX_DATA_RETRY		100
#define DW_TX_DATA_RETRY		100
#define DW_STATUS_WAIT_RETRY		100
#define DW_BUS_WAIT_INACTIVE		10

#ifndef I2C_MAX_BUS
#define I2C_MAX_BUS			10
#endif
#define I2C_GET_BUS()			i2c_get_bus_num()
#define I2C_SET_BUS(a)			i2c_set_bus_num(a)

#define GET_BUS_NUM			i2c_ctx.bus_num
#define i2c_sys_base			(i2c_ctx.dw_i2c[GET_BUS_NUM].base)
#define i2c_bus_speed			(i2c_ctx.dw_i2c[GET_BUS_NUM].speed)
#define i2c_bus_endian			(i2c_ctx.dw_i2c[GET_BUS_NUM].bus_endian)
#define i2c_rx_buffer			(i2c_ctx.dw_i2c[GET_BUS_NUM].rx_buffer)
#define i2c_tx_buffer			(i2c_ctx.dw_i2c[GET_BUS_NUM].tx_buffer)
#define i2c_poll_time			(i2c_ctx.dw_i2c[GET_BUS_NUM].poll_time)
#define i2c_init_done			(i2c_ctx.dw_i2c[GET_BUS_NUM].init_done)
#define i2c_sync()			dmbish()

/* I2C SCL counter macros */
enum {
	I2C_SS = 0,
	I2C_FS,
	I2C_HS_400PF,
	I2C_HS_100PF,
};

enum {
	I2C_SCL_HIGH = 0,
	I2C_SCL_LOW,
	I2C_SCL_TF,
};

/* Bus specific values */
struct dw_i2c_s {
	uint32_t base;
	uint32_t speed;
	uint32_t bus_endian;
	uint32_t rx_buffer;
	uint32_t tx_buffer;
	uint32_t poll_time;
	uint32_t init_done;
};

struct i2c_ctx_s {
	uint32_t bus_num;
	struct dw_i2c_s dw_i2c[I2C_MAX_BUS];
};

static struct i2c_ctx_s i2c_ctx = I2C_BUS_DATA;
static const uint32_t i2c_scl_min[4][3] = { /* in nano seconds */
	/* High, Low, tf */
	[I2C_SS] = {4000, 4700, 300}, /* SS (Standard Speed) */
	[I2C_FS] = {600, 1300, 300}, /* FS (Fast Speed) */
	[I2C_HS_400PF] = {160, 320, 300}, /* HS (High Speed) 400pf */
	[I2C_HS_100PF] = {60, 120, 300}, /* HS (High Speed) 100pf */
};

static uint32_t read32(uint32_t addr)
{
	return mmio_read_32(addr);
}

static void write32(uint32_t addr, uint32_t val)
{
	mmio_write_32(addr, val);
}

uint32_t i2c_get_bus_num(void)
{
	return i2c_ctx.bus_num;
}

int32_t i2c_set_bus_num(uint32_t bus)
{
	if (bus >= I2C_MAX_BUS)
		return 1;

	if (i2c_ctx.dw_i2c[bus].base == 0x0)
		return 1;

	i2c_ctx.bus_num = bus;

	return 0;
}

static void i2c_hw_init(void)
{
	static uint32_t hw_init_done;
	uint32_t base, param, speed;
	int32_t i;

	if (hw_init_done)
		return;

	for (i = 0; i < I2C_MAX_BUS; i++) {
		base = i2c_ctx.dw_i2c[i].base;
		if (base == 0x0)
			continue;

		i2c_ctx.dw_i2c[i].bus_endian = DW_LE_BUS;
		speed = i2c_ctx.dw_i2c[i].speed;
		i2c_ctx.dw_i2c[i].poll_time = ((10 * 1000000) / speed);

		param = read32(base + DW_IC_COMP_PARAM_1);
		i2c_ctx.dw_i2c[i].rx_buffer = ((param >> 8) & 0xff) + 1;
		i2c_ctx.dw_i2c[i].tx_buffer = ((param >> 16) & 0xff) + 1;
		DBG("I2c_Hw_Init: Bus:%d Rx_Buffer:%d Tx_Buffer:%d\n",
			i, i2c_ctx.dw_i2c[i].rx_buffer,
			i2c_ctx.dw_i2c[i].tx_buffer);
	}

	hw_init_done = 1;
}

static void i2c_dw_enable(uint32_t base, uint32_t enable)
{
	uint32_t i2c_status_cnt = DW_STATUS_WAIT_RETRY;

	do {
		write32(base + DW_IC_ENABLE, enable);
		i2c_sync();
		if ((read32(base + DW_IC_ENABLE_STATUS) & 0x01) == enable)
			break;
		udelay(i2c_poll_time);
	} while (i2c_status_cnt--);

	if (i2c_status_cnt == 0)
		ERROR("Enable/disable timeout: ");

	if ((enable == 0) || (i2c_status_cnt == 0)) {
		/* Unset the target adddress */
		write32(base + DW_IC_TAR, 0);
	}

	i2c_init_done = enable;
}


/*
 * Check for errors on i2c bus
 */
static uint32_t i2c_dw_check_errors(uint32_t base, int32_t print_err)
{
	uint32_t status = read32(base + DW_IC_RAW_INTR_STAT);
	uint32_t tx_abrt = 0;

	if (status & DW_IC_INTR_RX_UNDER)
		read32(base + DW_IC_CLR_RX_UNDER);

	if (status & DW_IC_INTR_RX_OVER)
		read32(base + DW_IC_CLR_RX_OVER);

	if (status & DW_IC_INTR_TX_ABRT) {
		status = read32(base + DW_IC_TX_ABRT_SOURCE);
		read32(base + DW_IC_CLR_TX_ABRT);
		if (print_err)
			VERBOSE("TX_ABORT ");
	}

	if (status & DW_IC_ERR_CONDITION) {
		if (print_err && (status || tx_abrt))
			VERBOSE("Errors on i2c bus %08x %08x: ",
				status, tx_abrt);
		/* Disable the adapter */
		i2c_dw_enable(base, 0);
	}

	return (status & DW_IC_ERR_CONDITION);
}

/*
 * Waiting for bus not busy
 */
static uint32_t i2c_dw_wait_bus_not_busy(uint32_t base)
{
	int32_t timeout = DW_BUS_WAIT_TIMEOUT;

	while (read32(base + DW_IC_STATUS) & DW_IC_STATUS_MST_ACTIVITY) {
		if (timeout-- <= 0) {
			ERROR("Timeout waiting for bus ready\n");
			return 1;
		}
		/*
		 * A delay isn't absolutely necessary.
		 * But to ensure that we don't hammer the bus constantly,
		 * delay for 1ms as with other implementation.
		 */
		udelay(1000);
	}
	return 0;
}

/*
 * Waiting for TX FIFO buffer available
 */
static int32_t i2c_dw_wait_tx_data(uint32_t base)
{
	int32_t timeout = DW_TX_DATA_RETRY;

	while (read32(base + DW_IC_TXFLR) == i2c_tx_buffer) {
		if (timeout-- <= 0) {
			ERROR("Timeout waiting for TX buffer available\n");
			return 1;
		}
		udelay(i2c_poll_time);
	}

	return 0;
}

/*
 * Waiting for TX FIFO buffer empty
 */
static int32_t i2c_dw_wait_tx_fifo_empty(uint32_t base)
{
	int32_t timeout = DW_TX_DATA_RETRY;

	while (!(read32(base + DW_IC_STATUS) & DW_IC_STATUS_TFE)) {
		if (timeout-- <= 0) {
			ERROR("Timeout waiting for TX FIFO empty\n");
			return 1;
		}
		udelay(i2c_poll_time);
	}

	return 0;
}

/*
 * Waiting for RX FIFO buffer available
 */
static int32_t i2c_dw_wait_rx_data(uint32_t base)
{
	int32_t timeout = DW_RX_DATA_RETRY;

	while (!(read32(base + DW_IC_STATUS) & DW_IC_STATUS_RFNE)) {
		if (timeout-- <= 0) {
			VERBOSE("Timeout waiting for RX buffer available\n");
			return 1;
		}
		udelay(i2c_poll_time);
	}

	return 0;
}

static uint32_t i2c_dw_scl_hcnt(uint32_t ic_clk, uint32_t t_symbol,
				uint32_t tf, int32_t cond,
				int32_t offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT.Based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond) {
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is.Based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (((ic_clk * t_symbol + 500000) / 1000000) - 8 + offset);
	}

	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
	 *
	 * This is just experimental rule; the tHD;STA period turned
	 * out to be proportinal to (_HCNT + 3).  With this setting,
	 * we could meet both tHIGH and tHD;STA timing specs.
	 *
	 * If unsure, you'd better to take this alternative.
	 *
	 * The reason why we need to take into acCount "tf" here,
	 * is the same as described in I2c_Dw_Scl_Lcnt().
	 */
	return (((ic_clk * (t_symbol + tf) + 500000) / 1000000) - 3 + offset);
}

static uint32_t i2c_dw_scl_lcnt(uint32_t ic_clk, uint32_t t_low,
				uint32_t tf, int32_t offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts Counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * acCount the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return (((ic_clk * (t_low + tf) + 500000) / 1000000) - 1 + offset);
}

/*
 * Configure scl clock Count for SS, FS, and HS.
 * This function is called during I2C init function.
 */
static void i2c_scl_init(uint32_t base, uint32_t i2c_clk_freq,
			 uint32_t i2c_speed)
{
	uint32_t input_clock_khz = i2c_clk_freq / 1000;
	uint32_t i2c_speed_khz = i2c_speed / 1000;
	uint32_t ss_clock_khz = input_clock_khz;
	uint32_t fs_clock_khz = input_clock_khz;
	uint32_t hs_clock_khz = input_clock_khz;
	uint32_t hcnt, lcnt;

	if (i2c_speed_khz <= 100) {
		/* Standard speed mode */
		ss_clock_khz = (ss_clock_khz * 100) / i2c_speed_khz;
		hcnt = i2c_dw_scl_hcnt(ss_clock_khz,
				i2c_scl_min[I2C_SS][I2C_SCL_HIGH], /* tHD;STA = tHIGH = 4.0 us */
				i2c_scl_min[I2C_SS][I2C_SCL_TF], /* tf = 0.3 us */
				0, /* 0: DW default, 1: Ideal */
				0); /* No Offset */
		lcnt = i2c_dw_scl_lcnt(ss_clock_khz,
				i2c_scl_min[I2C_SS][I2C_SCL_LOW], /* tLOW = 4.7 us */
				i2c_scl_min[I2C_SS][I2C_SCL_TF], /* tf = 0.3 us */
				0); /* No Offset */
		write32(base + DW_IC_SS_SCL_HCNT, hcnt);
		write32(base + DW_IC_SS_SCL_LCNT, lcnt);
	} else if (i2c_speed_khz > 100 && i2c_speed_khz <= 400) {
		/* Fast speed mode */
		fs_clock_khz = (fs_clock_khz * 400) / i2c_speed_khz;
		hcnt = i2c_dw_scl_hcnt(fs_clock_khz,
				i2c_scl_min[I2C_FS][I2C_SCL_HIGH], /* tHD;STA = tHIGH = 0.6 us */
				i2c_scl_min[I2C_FS][I2C_SCL_TF], /* tf = 0.3 us */
				0, /* 0: DW default, 1: Ideal */
				0); /* No Offset */
		lcnt = i2c_dw_scl_lcnt(fs_clock_khz,
				i2c_scl_min[I2C_FS][I2C_SCL_LOW], /* tLOW = 1.3 us */
				i2c_scl_min[I2C_FS][I2C_SCL_TF], /* tf = 0.3 us */
				0); /* No Offset */
		write32(base + DW_IC_FS_SCL_HCNT, hcnt);
		write32(base + DW_IC_FS_SCL_LCNT, lcnt);
	} else if (i2c_speed_khz > 400 && i2c_speed_khz <= 3400) {
		/* High speed mode */
		hs_clock_khz = (hs_clock_khz * 3400) / i2c_speed_khz;
		hcnt = i2c_dw_scl_hcnt(hs_clock_khz,
				i2c_scl_min[I2C_HS_400PF][I2C_SCL_HIGH], /* )
				 i2c_hw_init();tHD;STA = tHIGH = 0.06 us for 100pf 0.16 for 400pf */
				i2c_scl_min[I2C_HS_400PF][I2C_SCL_TF], /* tf = 0.3 us */
				0, /* 0: DW default, 1: Ideal */
				0); /* No Offset */
		lcnt = i2c_dw_scl_lcnt(hs_clock_khz,
				i2c_scl_min[I2C_HS_400PF][I2C_SCL_LOW], /* tLOW = 0.12 us for 100pf 0.32 us for 400pf */
				i2c_scl_min[I2C_HS_400PF][I2C_SCL_TF], /* tf = 0.3 us */
				0); /* No Offset */
		write32(base + DW_IC_HS_SCL_HCNT, hcnt);
		write32(base + DW_IC_HS_SCL_LCNT, lcnt);
	}
}

/*
 * Configure and enable the I2C master.
 */
void i2c_init(int32_t speed, int32_t slaveadd)
{
	uint32_t i2c_clk_freq = get_i2c_clk(i2c_get_bus_num());
	uint16_t ic_con;
	uint32_t base;

	i2c_hw_init();

	base = i2c_sys_base;

	if (i2c_bus_speed != speed)
		i2c_bus_speed = speed;

	i2c_poll_time = ((10 * 1000000) / speed) * (i2c_clk_freq / 50000000);

	DBG("I2c_Init: Polling time:%d\n", i2c_poll_time);
	DBG("I2c_Init: I2c_Bus_Speed:%d\n", i2c_bus_speed);
	DBG("I2c_Init: Clock frequency:%d\n", i2c_clk_freq);

	/* Disable the adapter and interrupt */
	i2c_dw_enable(base, 0);
	write32(base + DW_IC_INTR_MASK, 0);
	/* Set standard and fast speed deviders for high/low periods */
	i2c_scl_init(base, i2c_clk_freq, speed);
	/* Write target Address */
	write32(base + DW_IC_TAR, slaveadd);
	/* Write slave Address */
	write32(base + DW_IC_SAR, I2C_SAR); /* Address */
	ic_con = DW_IC_CON_MASTER |
			DW_IC_CON_SLAVE_DISABLE | DW_IC_CON_RESTART_EN;

	if (speed > 400000 && speed <= 3400000)
		ic_con |= (DW_IC_CON_SPEED_STD | DW_IC_CON_SPEED_FAST);
	else if (speed > 100000 && speed <= 400000)
		ic_con |= DW_IC_CON_SPEED_FAST;
	else
		ic_con |= DW_IC_CON_SPEED_STD;

	write32(base + DW_IC_CON, ic_con);

	write32(base + DW_IC_SDA_HOLD, 0x4b);

	/* Enable the adapter */
	i2c_dw_enable(base, 1);

	i2c_set_bus_num(i2c_ctx.bus_num);

}

static int32_t i2c_set_target(uint8_t chip)
{
	uint32_t base = i2c_sys_base;
	int32_t rc = 0;

	if (!i2c_init_done || read32(base + DW_IC_TAR) != chip) {
		i2c_init(i2c_bus_speed, chip);

		/* Check if TAR is set */
		if (read32(base + DW_IC_TAR) != chip) {
			ERROR("Cannot set target on I2c bus to 0x%x\n", chip);
			rc = 1;
		}
	}

	return rc;
}

static int32_t __i2c_read(uint8_t chip, uint32_t addr,
			  int32_t alen, uint8_t *data,
			  int32_t length)
{
	uint32_t base = i2c_sys_base;
	uint32_t rx_limit, tx_limit;
	int32_t offset = 0;
	int32_t count = 0;
	uint32_t cmd = 0;
	uint32_t restart;
	int32_t i, ret = 0;

	if (i2c_dw_wait_bus_not_busy(base))
		return 1;

	i2c_sync();

	if (alen == 2) {
		write32(base + DW_IC_DATA_CMD,
			((addr >> 8) & DW_IC_DATA_CMD_DAT_MASK));
		i2c_sync();
	}

	write32(base + DW_IC_DATA_CMD,
		(addr & DW_IC_DATA_CMD_DAT_MASK) | DW_IC_DATA_CMD_STOP);
	i2c_sync();

	restart = read32(base + DW_IC_CON);
	restart &= DW_IC_CON_RESTART_EN;
	while (offset < length) {
		if (i2c_dw_wait_bus_not_busy(base))
			return 1;

		i2c_sync();
		if (i2c_dw_check_errors(base, 1)) {
			VERBOSE("Writing Chip %02x register Offset %04x\n",
				chip, addr);
			ret = 1;
			goto exit;
		}

		rx_limit = i2c_rx_buffer - read32(base + DW_IC_RXFLR);
		tx_limit = i2c_tx_buffer - read32(base + DW_IC_TXFLR);
		count = length - offset;
		count = (count > rx_limit) ? rx_limit : count;
		count = (count > tx_limit) ? tx_limit : count;

		for (i = 0; i < count; i++) {
			cmd = 0;
			if (restart && (offset == 0) && (i == 0))
				cmd |= DW_IC_DATA_CMD_RESTART;
			if ((i + 1) == count)
				cmd |= DW_IC_DATA_CMD_STOP;

			write32(base + DW_IC_DATA_CMD,
				cmd | DW_IC_DATA_CMD_CMD); /* Read Command */
			i2c_sync();

			if (i2c_dw_check_errors(base, 1)) {
				ERROR("Reading Chip %x register Offset %x "
					"start %x Length %d\n",
					chip, offset + i, addr, length);
				ret = 1;
				goto exit;
			}
		}

		for (i = 0; i < count; i++) {
			if (i2c_dw_check_errors(base, 1)) {
				ERROR("Reading Chip %x data "
					"Offset %x start %x Length %d\n",
					chip, offset + i, addr, length);
				ret = 1;
				goto exit;
			}

			/* Check if Rx data is available */
			if (i2c_dw_wait_rx_data(base)) {
				ret = 1;
				goto exit;
			}

			data[offset + i] = read32(base + DW_IC_DATA_CMD);
			i2c_sync();
		}

		offset += count;
	}

exit:
	i2c_dw_wait_bus_not_busy(base);
	return ret;
}

static int32_t __i2c_write(uint8_t chip, uint32_t addr,
			   int32_t alen, uint8_t *data,
			   int32_t length, uint32_t page_size)
{
	uint32_t base = i2c_sys_base;
	uint32_t cmd = 0x200;
	int32_t offset = 0;
	uint32_t temp;
	int32_t i, ret = 0;

	DBG("I2c_write: chip:%d addr:%x alen:%d len:%d\n",
		chip, addr, alen, length);
	DBG("I2c_write: page_size:%d\n", page_size);

	if (i2c_dw_wait_bus_not_busy(base))
		return 1;

	if (page_size == 0)
		page_size = i2c_tx_buffer;

	/* If addr isn't write page aligned, write the unaligned first bytes */
	if ((addr % page_size) != 0) {
		temp = (length < (page_size - (addr % page_size))) ?
				length : page_size - (addr % page_size);
		for (i = 0; i < temp; i++) {
			if (alen == 2) {
				/* Write Addr */
				write32(base + DW_IC_DATA_CMD,
				(((addr + i) >> 8) & DW_IC_DATA_CMD_DAT_MASK));
				i2c_sync();
			}

			/* Write Addr */
			write32(base + DW_IC_DATA_CMD,
				((addr + i) & DW_IC_DATA_CMD_DAT_MASK));
			i2c_sync();

			write32(base + DW_IC_DATA_CMD,
				(data[offset + i] & DW_IC_DATA_CMD_DAT_MASK) |
				cmd);
			i2c_sync();

			if (i2c_dw_wait_tx_data(base)) {
				ret = 1;
				goto exit;
			}
		}
		length -= i;
		addr += i;
		offset += i;
	}

	while (length > (page_size - 1)) {
		/* Write Addr */
		if (alen == 2) {
			write32(base + DW_IC_DATA_CMD, (((addr) >> 8) & 0xFF));
			i2c_sync();
		}

		/* Write Addr */
		write32(base + DW_IC_DATA_CMD, ((addr) & 0xFF));
		i2c_sync();

		for (i = 0; i < page_size; i++) {
			if ((i + 1) == page_size) {
				write32(base + DW_IC_DATA_CMD,
					(data[offset + i] &
					DW_IC_DATA_CMD_DAT_MASK) | cmd);
			} else {
				write32(base + DW_IC_DATA_CMD,
					data[offset + i] &
					DW_IC_DATA_CMD_DAT_MASK);
			}
			i2c_sync();

			if (i2c_dw_wait_tx_data(base)) {
				ret = 1;
				goto exit;
			}
		}

		length -= page_size;
		addr += page_size;
		offset += page_size;
	}

	for (i = 0; i < length; i++) {
		/* Write Addr */
		if (alen == 2) {
			write32(base + DW_IC_DATA_CMD,
				(((addr + i) >> 8) & DW_IC_DATA_CMD_DAT_MASK));
			i2c_sync();
		}

		/* Write Addr */
		write32(base + DW_IC_DATA_CMD,
			((addr + i) & DW_IC_DATA_CMD_DAT_MASK));
		i2c_sync();

		write32(base + DW_IC_DATA_CMD,
			(data[offset + i] & DW_IC_DATA_CMD_DAT_MASK) | cmd);
		i2c_sync();

		if (i2c_dw_wait_tx_data(base)) {
			ret = 1;
			goto exit;
		}
	}

	/* Need to wait TX FIFO empty */
	ret = i2c_dw_wait_tx_fifo_empty(base);

exit:
	i2c_dw_wait_bus_not_busy(base);
	return ret;
}

int32_t i2c_write(uint8_t chip, uint32_t addr, int32_t alen,
		  uint8_t *buf, int32_t len, uint32_t page_size)
{
	int32_t rc = 1; /* signal error */

	if (i2c_set_target(chip) == 0) {
		rc = __i2c_write(chip, addr, alen,
				 (uint8_t *) buf, len, page_size);
	}

	return rc;
}

int32_t i2c_read(uint8_t chip, uint32_t addr, int32_t alen,
		 uint8_t *buf, int32_t len)
{
	int32_t rc = 1; /* signal error */

	if (i2c_set_target(chip) == 0)
		rc = __i2c_read(chip, addr, alen, (uint8_t *) buf, len);

	return rc;
}

int32_t i2c_probe(uint8_t chip)
{
	uint8_t buf[1];

	i2c_init(i2c_bus_speed, chip);

	return i2c_read(chip, 0, 0, (uint8_t *) buf, 1);
}

uint32_t i2c_get_bus_speed(void)
{
	return i2c_bus_speed;
}

int32_t i2c_set_bus_speed(uint32_t speed)
{
	if (speed > 0 && speed <= 3400000) {
		if (i2c_bus_speed != speed)
			i2c_init(speed, 0x0);
		return 0;
	}

	return 1;
}
