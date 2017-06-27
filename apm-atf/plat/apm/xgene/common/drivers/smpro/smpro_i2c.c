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
#include <bl_common.h>
#include <clk.h>
#include <config.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <i2c.h>
#include <mmio.h>
#include <platform_def.h>
#include <smpro_i2c.h>
#include <smpro_mbox.h>

#if SMPRO_DBG
#define DBG(...)			tf_printf("SMPRO: "__VA_ARGS__)
#else
#define DBG(...)
#endif

#define SMPRO_I2C_I2C_PROTOCOL		0
#define SMPRO_I2C_SMB_PROTOCOL		1

#define SMPRO_I2C_READ			0
#define SMPRO_I2C_WRITE			1

#define SMPRO_DEBUG_MSG			0
#define SMPRO_MSG_TYPE_SHIFT		28
#define SMPRO_DBG_SUBTYPE_I2C1READ	4
#define SMPRO_DBGMSG_TYPE_SHIFT		24
#define SMPRO_DBGMSG_TYPE_MASK		0x0F000000
#define SMPRO_I2C_DEV_SHIFT		23
#define SMPRO_I2C_DEV_MASK		0x00800000
#define SMPRO_I2C_DEVID_SHIFT		13
#define SMPRO_I2C_DEVID_MASK		0x007FE000
#define SMPRO_I2C_RW_SHIFT		12
#define SMPRO_I2C_RW_MASK		0x00001000
#define SMPRO_I2C_PROTO_SHIFT		11
#define SMPRO_I2C_PROTO_MASK		0x00000800
#define SMPRO_I2C_ADDRLEN_SHIFT		8
#define SMPRO_I2C_ADDRLEN_MASK		0x00000700
#define SMPRO_I2C_DATALEN_SHIFT		0
#define SMPRO_I2C_DATALEN_MASK		0x000000FF

/*
 * SLIMpro I2C message encode
 *
 * bus		 - Controller number (0-based)
 * chip_addr	 - I2C chip address
 * chip_addr_len - I2C chip address length (7/10 bit)
 * op		 - SMPRO_I2C_READ or SMPRO_I2C_WRITE
 * proto	 - SMPRO_I2C_SMB_PROTOCOL or SMPRO_I2C_I2C_PROTOCOL
 * addr		 - Address location inside the chip.
 * addr_len	 - Length of the addr field.
 * data_len	 - Length of the data field.
 */
#define SMPRO_I2C_ENCODE_MSG(dev, chip, op, proto, addrlen, datalen) \
	((SMPRO_DEBUG_MSG << SMPRO_MSG_TYPE_SHIFT) | \
	((SMPRO_DBG_SUBTYPE_I2C1READ << SMPRO_DBGMSG_TYPE_SHIFT) & \
	SMPRO_DBGMSG_TYPE_MASK) | \
	((dev << SMPRO_I2C_DEV_SHIFT) & SMPRO_I2C_DEV_MASK) | \
	((chip << SMPRO_I2C_DEVID_SHIFT) & SMPRO_I2C_DEVID_MASK) | \
	((op << SMPRO_I2C_RW_SHIFT) & SMPRO_I2C_RW_MASK) | \
	((proto << SMPRO_I2C_PROTO_SHIFT) & SMPRO_I2C_PROTO_MASK) | \
	((addrlen << SMPRO_I2C_ADDRLEN_SHIFT) & SMPRO_I2C_ADDRLEN_MASK) | \
	((datalen << SMPRO_I2C_DATALEN_SHIFT) & SMPRO_I2C_DATALEN_MASK))

/*
 * Encode upper address for block data address
 * Maximum address space is 48 bits. Not expect I2C offset is more than 0xFFFF.
 */
#define SMPRO_I2C_ENCODE_UPPER_DATABUF(a)	\
			((uint32_t)(((uint64_t)(a) >> 16) & 0xFFFF0000))
#define SMPRO_I2C_ENCODE_LOWER_DATABUF(a)	\
			((uint32_t)((uint64_t)(a) & 0xFFFFFFFF))
#define SMPRO_I2C_ENCODE_DATAADDR(a)		((uint64_t)(a) & 0xFFFF)

#define MAILBOX_SECURE_INDEX			1

int plat_smpro_i2c_read(struct smpro_i2c_op *op)
{
	uint32_t msg[3];
	uint32_t proto;
	int32_t rc;

	switch (op->proto) {
	case SMPRO_I2C_SMB:
		proto = SMPRO_I2C_SMB_PROTOCOL;
		break;
	case SMPRO_I2C_I2C:
		proto = SMPRO_I2C_I2C_PROTOCOL;
		break;
	default:
		return -EINVAL;
		break;
	}

	if (op->data_len > I2C_MAX_BLOCK_DATA || op->addr_len > 2)
		return -EINVAL;

	msg[0] = SMPRO_I2C_ENCODE_MSG(op->bus, op->chip_addr,
			SMPRO_I2C_READ, proto, op->addr_len,
			op->data_len);
	msg[1] = SMPRO_I2C_ENCODE_UPPER_DATABUF(op->data) |
			SMPRO_I2C_ENCODE_DATAADDR(op->addr);
	msg[2] = SMPRO_I2C_ENCODE_LOWER_DATABUF(op->data);

	rc = plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	rc = plat_smpro_mbox_rx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	if (!msg[0])
		/* Transfer succesfully */
		return 0;

	return -EBUSY;
}

int plat_smpro_i2c_write(struct smpro_i2c_op *op)
{
	uint32_t msg[3];
	uint32_t proto;
	int32_t rc;

	switch (op->proto) {
	case SMPRO_I2C_SMB:
		proto = SMPRO_I2C_SMB_PROTOCOL;
		break;
	case SMPRO_I2C_I2C:
		proto = SMPRO_I2C_I2C_PROTOCOL;
		break;
	default:
		return -EINVAL;
		break;
	}

	if (op->data_len > I2C_MAX_BLOCK_DATA || op->addr_len > 2)
		return -EINVAL;

	msg[0] = SMPRO_I2C_ENCODE_MSG(op->bus, op->chip_addr,
			SMPRO_I2C_WRITE, proto, op->addr_len,
			op->data_len);
	msg[1] = SMPRO_I2C_ENCODE_UPPER_DATABUF(op->data) |
			SMPRO_I2C_ENCODE_DATAADDR(op->addr);
	msg[2] = SMPRO_I2C_ENCODE_LOWER_DATABUF(op->data);

	rc = plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	rc = plat_smpro_mbox_rx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	if (!msg[0])
		/* Transfer succesfully */
		return 0;

	return -EBUSY;
}
