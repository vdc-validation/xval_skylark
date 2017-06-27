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
#include <xgene_nvparam.h>
#include <nvparam_proxy.h>
#include <xgene_fip_helper.h>
#include <xlat_tables.h>

#define XNV_PARAM_SMC_GET	0x00000001
#define XNV_PARAM_SMC_SET	0x00000002
#define XNV_PARAM_SMC_CLR	0x00000003
#define XNV_PARAM_SMC_CLR_ALL	0x00000004

#define ACL_RD(x) ((x) >> 16)
#define ACL_WR(x) ((x) & 0xFFFF)

static uint64_t nvparam_proxy_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	enum nvparam param;
	uint16_t acl_rd, acl_wr;
	uint32_t val = 0;
	int ret = 0;

	switch (x1) {
	case XNV_PARAM_SMC_GET:
		param = (enum nvparam) x2;
		acl_rd = ACL_RD(x3) & (NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC);
		ret = plat_nvparam_get(param, acl_rd, &val);
		SMC_RET3(handle, 0, ret, val);
		break;
	case XNV_PARAM_SMC_SET:
		param = (enum nvparam) x2;
		acl_rd = ACL_RD(x3) & (NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC);
		acl_rd |= NV_PERM_ATF;
		acl_wr = ACL_WR(x3) & (NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC);
		val = x4;
		ret = plat_nvparam_set(param, acl_rd, acl_wr, val);
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	case XNV_PARAM_SMC_CLR:
		param = (enum nvparam) x2;
		acl_wr = ACL_WR(x3) & (NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC);
		ret = plat_nvparam_clr(param, acl_wr);
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	case XNV_PARAM_SMC_CLR_ALL:
		ret = plat_nvparam_clr_all();
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	default:
		SMC_RET1(handle, (uint64_t) -EINVAL);
		break;
	}

	SMC_RET1(handle, (uint64_t) -EINVAL);
}

void nvparam_proxy_register(void)
{
	int ret;

	ret = smc64_oem_svc_register(SMC_XNV_PARAM, &nvparam_proxy_handler);
	if (!ret)
		INFO("NV_PARAM proxy service registered\n");
}
