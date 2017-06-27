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
#include <platform_def.h>
#include <smpmpro_proxy.h>
#include <smpro_query.h>
#include <smpro_misc.h>
#include <string.h>

enum {
	SMPRO_FUNC_GET_FW_VER = 0,
	SMPRO_FUNC_GET_FW_BUILD,
	SMPRO_FUNC_GET_FW_CAP,
	PMPRO_FUNC_GET_FW_VER,
	PMPRO_FUNC_GET_FW_BUILD,
	PMPRO_FUNC_GET_FW_CAP,
	SMPRO_FUNC_SET_CFG,
};

static uint64_t smpmpro_proxy_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	uint32_t length;
	uint8_t *buf;
	int rc = -EINVAL;

	buf = (uint8_t *) x2;
	length = (uint32_t) x3;
	switch (x1) {
	case SMPRO_FUNC_GET_FW_VER:
		rc = plat_smpro_get_smpro_fw_ver(buf, &length);
		break;
	case SMPRO_FUNC_GET_FW_BUILD:
		rc = plat_smpro_get_smpro_fw_build(buf, &length);
		break;
	case SMPRO_FUNC_GET_FW_CAP:
		rc = plat_smpro_get_smpro_fw_build(buf, &length);
		break;
	case PMPRO_FUNC_GET_FW_VER:
		rc = plat_smpro_get_smpro_fw_ver(buf, &length);
		break;
	case PMPRO_FUNC_GET_FW_BUILD:
		rc = plat_smpro_get_smpro_fw_build(buf, &length);
		break;
	case PMPRO_FUNC_GET_FW_CAP:
		rc = plat_smpro_get_smpro_fw_build(buf, &length);
		break;
	case SMPRO_FUNC_SET_CFG:
		rc = plat_smpro_cfg_write(x2, x3);
		break;
        default:
		SMC_RET1(handle, (uint64_t) rc);
		break;
	}

	if (rc)
		SMC_RET2(handle, 0, (uint64_t) rc);

	flush_dcache_range((uint64_t) buf, length);
	SMC_RET4(handle, 0, (uint64_t) rc, x2, length);
}

void smpmpro_proxy_register(void)
{
	int rc;

	rc = smc64_oem_svc_register(SMC_SMPMPRO_PROXY, &smpmpro_proxy_handler);
	if (!rc)
		INFO("SMPMPro Proxy service registered\n");
}
