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
#include <apm_oem_svc.h>
#include <config.h>
#include <debug.h>
#include <errno.h>
#include <i2c.h>
#include <i2c_proxy.h>
#include <platform_def.h>
#include <smpro_i2c.h>
#include <xgene_fip_helper.h>
#include <xlat_tables.h>

#define SMC_XI2C_BUS_SHIFT		0
#define SMC_XI2C_BUS_MASK		0x000000FF
#define SMC_XI2C_CHIP_SHIFT		8
#define SMC_XI2C_CHIP_MASK		0x0000FF00
#define SMC_XI2C_OP_SHIFT		16
#define SMC_XI2C_OP_MASK		0x00FF0000
#define SMC_XI2C_PROTO_SHIFT		24
#define SMC_XI2C_PROTO_MASK		0xFF000000
#define SMC_XI2C_ADDRLEN_SHIFT		0
#define SMC_XI2C_ADDRLEN_MASK		0x000000FF
#define SMC_XI2C_ADDR_SHIFT		8
#define SMC_XI2C_ADDR_MASK		0x0000FF00
#define SMC_XI2C_RD 		      	0x3
#define SMC_XI2C_WR       		0x4

#define SMC_XI2C_GET_OP(_x_)		(((_x_) & SMC_XI2C_OP_MASK) \
					>> SMC_XI2C_OP_SHIFT)
#define SMC_XI2C_GET_BUS(_x_)		(((_x_) & SMC_XI2C_BUS_MASK) \
					>> SMC_XI2C_BUS_SHIFT)
#define SMC_XI2C_GET_CHIP_ADDR(_x_)	(((_x_) & SMC_XI2C_CHIP_MASK) \
					>> SMC_XI2C_CHIP_SHIFT)
#define SMC_XI2C_GET_ADDR(_x_)		(((_x_) & SMC_XI2C_ADDR_MASK) \
					>> SMC_XI2C_ADDR_SHIFT)
#define SMC_XI2C_GET_ADDR_LEN(_x_)	(((_x_) & SMC_XI2C_ADDRLEN_MASK) \
					>> SMC_XI2C_ADDRLEN_SHIFT)
#define SMC_XI2C_GET_PROTO(_x_)		(((_x_) & SMC_XI2C_PROTO_MASK) \
					>> SMC_XI2C_PROTO_SHIFT)
#define SMC_XI2C_GET_PROTO(_x_)		(((_x_) & SMC_XI2C_PROTO_MASK) \
					>> SMC_XI2C_PROTO_SHIFT)

enum i2c_bus {
	I2C_0,
	I2C_1,
	I2C_2,
	I2C_3,
	I2C_4,
	I2C_5,
};

static uint64_t i2c_proxy_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	int rc;
	int op = SMC_XI2C_GET_OP(x1);
	struct smpro_i2c_op i2c_op;
	uint8_t *udata;

	i2c_op.bus = SMC_XI2C_GET_BUS(x1);
	i2c_op.chip_addr = SMC_XI2C_GET_CHIP_ADDR(x1);
	i2c_op.addr = SMC_XI2C_GET_ADDR(x2);
	i2c_op.addr_len = SMC_XI2C_GET_ADDR_LEN(x2);
	i2c_op.data_len = x3;
	i2c_op.data = x4;

	switch (op) {
	case SMC_XI2C_RD:
		switch(i2c_op.bus) {
		case I2C_0:
		case I2C_1:
			rc = plat_smpro_i2c_read(&i2c_op);
			if (!rc) {
				udata = (uint8_t *)x4;
				udata[0] = i2c_op.data &  0x000000FF;
				udata[1] = (i2c_op.data & 0x0000FF00) >> 8;
				udata[2] = (i2c_op.data & 0x00FF0000) >> 16;
				udata[3] = (i2c_op.data & 0xFF000000) >> 24;
			}

			SMC_RET4(handle, 0, rc, x2, x3);
			break;
		case I2C_2:
		case I2C_3:
		case I2C_4:
		case I2C_5:
			rc = i2c_set_bus_num(i2c_op.bus);
			if (rc)
				SMC_RET2(handle, 0, -rc);
			
			rc = i2c_read(i2c_op.chip_addr, i2c_op.addr,
				i2c_op.addr_len, (uint8_t *)i2c_op.data,
				i2c_op.data_len);
			SMC_RET4(handle, 0, -rc, x2, x3);
			break;
		default:
			SMC_RET2(handle, 0, (uint64_t)-EINVAL);
		}
		break;
	case SMC_XI2C_WR:
		switch(i2c_op.bus) {
		case I2C_0:
		case I2C_1:
			if (i2c_op.data_len > 4) {
				rc = plat_smpro_i2c_write(&i2c_op);
			} else {	
				udata = (uint8_t *)x4;
				i2c_op.data = udata[0];
				i2c_op.data |= udata[1] << 8;
				i2c_op.data |= udata[2] << 16;
				i2c_op.data |= udata[3] << 24;
				rc = plat_smpro_i2c_write(&i2c_op);
			}

			SMC_RET4(handle, 0, rc, x2, x3);
			break;
		case I2C_2:
		case I2C_3:
		case I2C_4:
		case I2C_5:
			rc = i2c_set_bus_num(i2c_op.bus);
			if (rc)
				SMC_RET2(handle, 0, -rc);
			rc = i2c_write(i2c_op.chip_addr, i2c_op.addr,
					i2c_op.addr_len, (uint8_t *)i2c_op.data,
					i2c_op.data_len, 0);
			SMC_RET4(handle, 0, -rc, x2, x3);
			break;
		default:
			SMC_RET2(handle, 0, (uint64_t)-EINVAL);
		}
		break;
	default:
		SMC_RET1(handle, (uint64_t) -EINVAL);
		break;
	}

	SMC_RET1(handle, (uint64_t) -EINVAL);
}

void i2c_proxy_register(void)
{
	int rc;

	rc = smc64_oem_svc_register(SMC_XI2C,
			&i2c_proxy_handler);
	if (!rc)
		INFO("XI2C proxy service registered\n");
}
