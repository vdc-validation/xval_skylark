/*
 * Copyright (c) 2017, Applied Micro Circuits Corporation. All rights reserved.
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
#include <ras.h>
#include <ras_proxy.h>
#include <xgene_fip_helper.h>
#include <xlat_tables.h>

enum SMC_XRAS_FUNC_ID {
	SMC_XRAS_SET_APEI_PTR = 1,
	SMC_XRAS_GET_APEI_PTR,
	SMC_XRAS_ENABLE,
	SMC_XRAS_DISABLE
};

static uint64_t ras_proxy_handler(uint32_t smc_fid,
				  uint64_t x1,
				  uint64_t x2,
				  uint64_t x3,
				  uint64_t x4,
				  void *cookie,
				  void *handle,
				  uint64_t flags)
{
	uint32_t val = 0;
	int ret = 0;

	switch (x1) {
	case SMC_XRAS_SET_APEI_PTR:
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	case SMC_XRAS_GET_APEI_PTR:
		SMC_RET3(handle, 0, ret, val);
		break;
	case SMC_XRAS_ENABLE:
		xgene_ras_enable();
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	case SMC_XRAS_DISABLE:
		SMC_RET2(handle, 0, (uint64_t) ret);
		break;
	}

	SMC_RET1(handle, (uint64_t) -EINVAL);
}

void ras_proxy_register(void)
{
	int ret;

	ret = smc64_oem_svc_register(SMC_XRAS, &ras_proxy_handler);
	if (!ret)
		INFO("RAS proxy service registered\n");
}
