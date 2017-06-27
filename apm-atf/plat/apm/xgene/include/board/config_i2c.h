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

#ifndef __CONFIG_I2C_H__
#define __CONFIG_I2C_H__

#include <skylark_soc.h>

#define I2C_MAX_BUS			6

#define I2C_SAR				0x52
#define I2C_SPD_BUS_NUM			2

#define I2C_SPEED_100KHZ		100000
#define I2C_SPEED_400KHZ		400000

#define I2C_BUS_DATA \
	{ \
		I2C_SPD_BUS_NUM, \
		{ \
			{0x0}, \
			{0x0}, \
			{I2C2_REG_BASE, I2C_SPEED_400KHZ}, \
			{0x0}, \
			{0x0}, \
			{0x0} \
		} \
	}

#define I2C_MAX_BLOCK_DATA		8

/* EEPROM I2C */
#define I2C_EEPROM_BUS_NUM		1
#define I2C_EEPROM_SMPRO_ADDR		0x52
#define I2C_EEPROM_PMPRO_ADDR		0x53
#define I2C_EEPROM_MAX_SIZE		0x10000 /* 64 KB */

#endif /* __CONFIG_I2C_H__ */
