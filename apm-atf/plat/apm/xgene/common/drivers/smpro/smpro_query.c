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
#include <mmio.h>
#include <platform_def.h>
#include <smpro_query.h>
#include <smpro_misc.h>
#include <smpro_mbox.h>
#include <string.h>

int plat_smpro_get_smpro_fw_ver(uint8_t *buff, uint32_t *length)
{
	int rc;
	struct smpro_info info;

	memset(&info, 0, sizeof(info));

	rc = plat_smpro_get_info(&info);

	if (rc < 0)
		return rc;

	snprintf((char *) buff, *length, "%d.%02d", info.fw_ver.ver_bits.major,
		 info.fw_ver.ver_bits.minor);
	buff[*length - 1] = '\0';
	*length = strlen((char *) buff);
	return 0;
}

int plat_smpro_get_smpro_fw_build(uint8_t *buff, uint32_t *length)
{
	int rc;
	struct smpro_info info;

	memset(&info, 0, sizeof(info));

	rc = plat_smpro_get_info(&info);

	if (rc < 0)
		return rc;

	/* SMpro build id is in BCD format. So print as HEX */
	snprintf((char *) buff, *length, "%08X", info.build.date);
	buff[*length - 1] = '\0';
	*length = strlen((char *) buff);
	return 0;
}
