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

#include <apm_oem_svc.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <smpro_mbox.h>
#include <smpro_misc.h>
#include <failsafe_proxy.h>
#include <xgene_nvparam.h>

enum failsafe_smc_func_id {
	FAILSAFE_SMC_GET_STATUS = 0,
	FAILSAFE_SMC_SET_BOOT_SUCCESSFUL,
};

enum smpro_failsafe_msg_type {
	SMPRO_FAILSAFE_MSG_UNUSED = 0,
	SMPRO_FAILSAFE_MSG_GET_STATUS,
	SMPRO_FAILSAFE_MSG_SET_BOOT_SUCCESSFUL,
	SMPRO_FAILSAFE_MSG_MAX = 0xf
};

int failsafe_proxy_get_status(enum failsafe_status *status)
{
	if (!status)
		return -EINVAL;

	return plat_smpro_failsafe_get_info(
		SMPRO_FAILSAFE_MSG_GET_STATUS, (uint32_t *) status);
}

static uint64_t failsafe_proxy_smc_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	enum failsafe_smc_func_id	func_id;
	enum failsafe_status		status;
	int				ret;

	func_id = x1;
	switch (func_id) {
	case FAILSAFE_SMC_GET_STATUS:
		if (XGENE_VHP) {
			SMC_RET2(handle, 0, FAILSAFE_BOOT_NORMAL);
			break;
		}

		ret = failsafe_proxy_get_status(&status);
		if (ret) {
			SMC_RET1(handle, -EPERM);
			break;
		}

		SMC_RET2(handle, 0, status);
		break;
	case FAILSAFE_SMC_SET_BOOT_SUCCESSFUL:
		if (XGENE_VHP) {
			SMC_RET1(handle, 0);
			break;
		}

		ret = failsafe_proxy_get_status(&status);
		if (ret) {
			SMC_RET1(handle, -EPERM);
			break;
		}

		switch ((int) status) {
		case FAILSAFE_BOOT_NORMAL:
			ret = plat_nvparam_save_last_good_known_setting();
			break;
		case FAILSAFE_BOOT_LAST_KNOWN_SETTINGS:
			ret = plat_nvparam_restore_last_good_known_setting();
			break;
		case FAILSAFE_BOOT_DEFAULT_SETTINGS:
			/* clear all NVPARAMs */
			ret = plat_nvparam_clr_all();
			break;
		}
		if (ret) {
			SMC_RET1(handle, -EPERM);
			break;
		}

		ret = plat_smpro_failsafe_set_info(
			SMPRO_FAILSAFE_MSG_SET_BOOT_SUCCESSFUL, 0);
		if (ret) {
			SMC_RET1(handle, -EPERM);
			break;
		}

		SMC_RET1(handle, 0);
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}

void svc_failsafe_proxy_register(void)
{
	int ret;

	ret = smc64_oem_svc_register(SMC_FAILSAFE, &failsafe_proxy_smc_handler);

	if (!ret)
		INFO("Failsafe proxy service registered\n");
}
