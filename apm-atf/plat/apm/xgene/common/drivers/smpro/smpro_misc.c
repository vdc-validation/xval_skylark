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

#include <string.h>
#include <smpro_mbox.h>
#include <smpro_misc.h>

#define SMPRO_DBG_SUBTYPE_REGREAD	0x1
#define SMPRO_DBG_SUBTYPE_REGWRITE	0x2
#define SMPRO_DBG_SUBTYPE_REGREG_RESP	0x3
#define SMPRO_DBG_SUBTYPE_BMC_MSG	0xE

#define SMPRO_DEBUG_MSG			0x0
#define SMPRO_MSG_TYPE_SHIFT      	28
#define SMPRO_DBGMSG_TYPE_MASK		0x0F000000
#define SMPRO_DBGMSG_TYPE_SHIFT		24
#define SMPRO_DBGMSG_TYPE_MASK		0x0F000000
#define SMPRO_DBGMSG_CTRL_MASK		0x00FF0000
#define SMPRO_DBMMSG_CTRL_SHIFT		16
#define SMPRO_DBGMSG_P0_MASK		0x0000FF00
#define SMPRO_DBGMSG_P0_SHIFT		8
#define SMPRO_DBGMSG_P1_MASK		0x000000FF
#define SMPRO_DBGMSG_P1_SHIFT		0
#define SMPRO_ENCODE_DEBUG_MSG(type, cb, p0, p1) \
	((SMPRO_DEBUG_MSG << SMPRO_MSG_TYPE_SHIFT) | \
	(((type) << SMPRO_DBGMSG_TYPE_SHIFT) & SMPRO_DBGMSG_TYPE_MASK) | \
	(((cb) << SMPRO_DBMMSG_CTRL_SHIFT) & SMPRO_DBGMSG_CTRL_MASK) | \
	(((p0) << SMPRO_DBGMSG_P0_SHIFT) & SMPRO_DBGMSG_P0_MASK) | \
	(((p1) << SMPRO_DBGMSG_P1_SHIFT) & SMPRO_DBGMSG_P1_MASK))
#define SMPRO_DECODE_DBGMSG_P0(data)	\
	(((data) & SMPRO_DBGMSG_P0_MASK) >> SMPRO_DBGMSG_P0_SHIFT)
#define SMPRO_DECODE_DBGMSG_TYPE(data)	\
	(((data) & SMPRO_DBGMSG_TYPE_MASK) >> SMPRO_DBGMSG_TYPE_SHIFT)
#define SMPRO_MSG_CONTROL_URG_BIT	0x80
#define BMC_DIMM_REQ_CMD		0x3
#define ENCODE_BMC_CMD_REQ(cmd)		((cmd & 0x03) << 0)

#define SMPRO_USER_MSG			0x6
#define SMPRO_USER_MSG_HNDL_SHIFT 	24
#define SMPRO_USER_MSG_HNDL_MASK 	0x0F000000
#define SMPRO_MSG_CTRL_BYTE_SHIFT	16
#define SMPRO_MSG_CTRL_BYTE_MASK	0x00FF0000
#define SMPRO_USER_MSG_P0_SHIFT 	8
#define SMPRO_USER_MSG_P0_MASK 		0x0000FF00
#define SMPRO_USER_MSG_P1_MASK 		0x000000FF
#define SMPRO_ENCODE_USER_MSG(hndl, cb, p0, p1) \
	((SMPRO_USER_MSG << SMPRO_MSG_TYPE_SHIFT) | \
	(((hndl) << SMPRO_USER_MSG_HNDL_SHIFT) & SMPRO_USER_MSG_HNDL_MASK) | \
	(((cb) << SMPRO_MSG_CTRL_BYTE_SHIFT) & SMPRO_MSG_CTRL_BYTE_MASK) | \
	(((p0) << SMPRO_USER_MSG_P0_SHIFT) & SMPRO_USER_MSG_P0_MASK) | \
	((p1) & SMPRO_USER_MSG_P1_MASK))
#define SMPRO_CONFIG_SET_HDLR		2
#define SMPRO_FAILSAFE_HDLR		3
#define SMPRO_CONFIG_GET_HDLR		4
#define SMPRO_DECODE_MSG_CTRL_BYTE(cb) \
	(((cb) & SMPRO_MSG_CTRL_BYTE_MASK) >> SMPRO_MSG_CTRL_BYTE_SHIFT)
#define SMPRO_DECODE_USER_MSG_P0(data)	(((data) & SMPRO_USER_MSG_P0_MASK) >> \
					SMPRO_USER_MSG_P0_SHIFT)
#define SMPRO_DECODE_USER_MSG_P1(data)	((data) & SMPRO_USER_MSG_P1_MASK)

#define SMPRO_RAS_MSG			0xB
#define SMPRO_RAS_MSG_HDLR		1
#define SMPRO_RAS_MSG_HNDL_MASK		0x0F000000U
#define SMPRO_RAS_MSG_HNDL_SHIFT	24
#define SMPRO_RAS_MSG_CMD_MASK		0x00F00000U
#define SMPRO_RAS_MSG_CMD_SHIFT		20
#define SMPRO_RAS_MSG_HDLR		1
#define SMPRO_RAS_MSG_START		2 /* Start RAS scanning */
#define SMPRO_RAS_MSG_STOP		3 /* Stop RAS scanning */

#define SMPRO_ENCODE_RAS_MSG(cmd, cb) \
	((SMPRO_RAS_MSG << SMPRO_MSG_TYPE_SHIFT) | \
	((SMPRO_RAS_MSG_HDLR << SMPRO_RAS_MSG_HNDL_SHIFT) & SMPRO_RAS_MSG_HNDL_MASK) | \
	(((cb) << SMPRO_MSG_CTRL_BYTE_SHIFT) & SMPRO_MSG_CTRL_BYTE_MASK) | \
	(((cmd) << SMPRO_RAS_MSG_CMD_SHIFT) & SMPRO_RAS_MSG_CMD_MASK))

#define MAILBOX_SECURE_INDEX		1	/* Secure mailbox 1 */
#define MAILBOX_NON_SECURE_INDEX	0	/* Non-Secure mailbox 0 */
#define MAILBOX_NON_SECURE_MULT		0x10	/* Non-Secure offset multiplier */

/*
 * The SMpro/PMpro information about build/cap/fw_version/console info
 * are required by ATF. The preferred solution is to read from
 * MPA_SCRATCH14/15. But this doesn't work with secure boot.
 * The second option is do an Mailbox reqest as shown in plat_smpro_get_info.
 * But this function relies on Mailbox protocol and will have issue
 * in ATF early boot sequence. The 3rd option is to have SMpro cache the
 * info in a location that is accessible by ATF in the secure domain. Given
 * that we have Mailbox registers, let us use secure Mailbox ID #3
 * for this.
 *
 * Mailbox #3 WORD 0 - SMpro SCRATCH14 0x1AC shadow copy
 * Mailbox #3 WORD 1 - SMpro SCRATCH15 0x1B0 shadow copy
 * Mailbox #3 WORD 2 - SMpro SCRATCH   0x360 shadow copy
 */
#define MAILBOX_SCRATCH_SHADOW		3
/*
 * SMpro sets the bit 31 and bit 29 in MPA_SCRATCH register
 * when it is ready for message processing.
 */
#define MAILBOX_SCRATCH_MSG_RDY		0xA0000000

int plat_smpro_read(uint32_t reg, uint32_t *val, int urgent)
{
	uint32_t msg[3];
	int32_t rc;
	/*
	 * If the urgent bit of a message is set then SMpro/PMpro
	 * will process the msg instantaneously in irq context.
	 */
	uint32_t urgent_msg = urgent ? SMPRO_MSG_CONTROL_URG_BIT : 0;

	msg[0] = SMPRO_ENCODE_DEBUG_MSG(SMPRO_DBG_SUBTYPE_REGREAD,
					urgent_msg, 0, 0);
	msg[1] = reg;
	msg[2] = 0;

	rc = plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	rc = plat_smpro_mbox_rx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;
	if (!SMPRO_DECODE_DBGMSG_TYPE(msg[0]))
		return -EINVAL;

	if (val)
		*val = msg[1];

	return rc;
}

int plat_smpro_write(uint32_t reg, uint32_t val, int urgent)
{
	uint32_t msg[3];
	/*
	 * If the urgent bit of a message is set then SMpro/PMpro
	 * will process the msg instantaneously in irq context.
	 */
	uint32_t urgent_msg = urgent ? SMPRO_MSG_CONTROL_URG_BIT : 0;

	msg[0] = SMPRO_ENCODE_DEBUG_MSG(SMPRO_DBG_SUBTYPE_REGWRITE,
					urgent_msg, 0, 0);
	msg[1] = reg;
	msg[2] = val;

	return plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
}

int plat_smpro_cfg_write(uint8_t cfg_type, uint8_t value)
{
	uint32_t msg[3];

	msg[0] = SMPRO_ENCODE_USER_MSG(SMPRO_CONFIG_SET_HDLR, 0,
				       cfg_type, value);
	msg[1] = 0;
	msg[2] = 0;

	return plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
}

int plat_smpro_cfg_read(uint8_t cfg_type, uint8_t *value)
{
	uint32_t msg[3];
	int32_t rc;

	msg[0] = SMPRO_ENCODE_USER_MSG(SMPRO_CONFIG_GET_HDLR, cfg_type, 0, 0);
	msg[1] = 0;
	msg[2] = 0;

	rc = plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	rc = plat_smpro_mbox_rx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;
	if (SMPRO_DECODE_MSG_CTRL_BYTE(msg[0]) == 1)
		return -EINVAL;

	if (value)
		*value = SMPRO_DECODE_USER_MSG_P1(msg[0]);

	return rc;
}

int plat_smpro_get_info(struct smpro_info *info)
{
	uint32_t msg[3];
	int rc;

	rc = plat_smpro_mbox_read(MAILBOX_SCRATCH_SHADOW, msg);
	if (rc < 0)
		return rc;

	info->cap.cap = msg[0];
	info->build.date = msg[1];
	info->fw_ver.version = msg[2];

	return 0;
}

int plat_smpro_early_get_info(struct smpro_info *info)
{
	uint32_t msg[3];

	memset(info, 0, sizeof(*info));
	/*
	 * The MAILBOX_SCRATCH_MSG_RDY signature is the last word
	 * written to the MAILBOX. So fixing the order of msg array.
	 * Mailbox #3 WORD 0 - msg[0]
	 * Mailbox #3 WORD 1 - msg[1]
	 * Mailbox #3 WORD 2 - msg[2]
	 *
	 * Wait for SMpro ready for message processing before
	 * continue.
	 */
	if (!XGENE_VHP) {
		do {
			plat_smpro_mbox_read(MAILBOX_SCRATCH_SHADOW, msg);
		} while ((msg[2] & MAILBOX_SCRATCH_MSG_RDY) !=
			 MAILBOX_SCRATCH_MSG_RDY);

		info->cap.cap = msg[0];
		info->build.date = msg[1];
	} else {
		info->cap.cap = 0;
		info->build.date = 0;
	}

	return 0;
}

int plat_smpro_failsafe_get_info(uint8_t info_type, uint32_t *value)
{
	uint32_t msg[3];
	int32_t rc;

	msg[0] = SMPRO_ENCODE_USER_MSG(SMPRO_FAILSAFE_HDLR,
					SMPRO_MSG_CONTROL_URG_BIT,
					info_type, 0);
	msg[1] = 0;
	msg[2] = 0;

	rc = plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	rc = plat_smpro_mbox_rx(MAILBOX_SECURE_INDEX, msg);
	if (rc < 0)
		return rc;

	if (SMPRO_DECODE_MSG_CTRL_BYTE(msg[0]) == 1)
		return -EINVAL;

	if (value)
		*value = SMPRO_DECODE_USER_MSG_P0(msg[0]);

	return rc;
}

int plat_smpro_failsafe_set_info(uint8_t info_type, uint32_t *value)
{
	uint32_t msg[3];

	msg[0] = SMPRO_ENCODE_USER_MSG(SMPRO_FAILSAFE_HDLR,
					SMPRO_MSG_CONTROL_URG_BIT,
					info_type, (value) ? *value : 0);
	msg[1] = 0;
	msg[2] = 0;

	return plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
}

int plat_enable_smpro_bmc_dimm_scan(void)
{
	uint32_t msg[3];
	msg[0] = SMPRO_ENCODE_DEBUG_MSG(SMPRO_DBG_SUBTYPE_BMC_MSG,
			0, 0, ENCODE_BMC_CMD_REQ(BMC_DIMM_REQ_CMD));

	msg[1] = 0;
	msg[2] = 0;

	return plat_smpro_mbox_tx(MAILBOX_SECURE_INDEX, msg);
}

int plat_smpro_ras_scan(uint8_t enable)
{
	int mbox = MAILBOX_NON_SECURE_INDEX + MAILBOX_NON_SECURE_MULT;
	uint32_t msg[3];

	if (enable)
		msg[0] = SMPRO_ENCODE_RAS_MSG(SMPRO_RAS_MSG_START, 0);
	else
		msg[0] = SMPRO_ENCODE_RAS_MSG(SMPRO_RAS_MSG_STOP, 0);

	msg[1] = 0;
	msg[2] = 0;

	return plat_smpro_mbox_tx(mbox, msg);
}

