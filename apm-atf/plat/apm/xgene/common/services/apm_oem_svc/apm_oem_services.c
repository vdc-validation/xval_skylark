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
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
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
#include <errno.h>
#include <failsafe_proxy.h>
#include <i2c_proxy.h>
#include <nvparam_proxy.h>
#include <platform_info.h>
#include <ras_proxy.h>
#include <smpmpro_proxy.h>
#include <spi_nor_proxy.h>
#include <sys_mem_info.h>

/* SMC APM OEM service table */
apm_oem_svc_handler_t smc32_svc_hdlr_tbl[SMC32_OEM_SVC_HNDL_CNT];
apm_oem_svc_handler_t smc64_svc_hdlr_tbl[SMC64_OEM_SVC_HNDL_CNT];

static uint32_t smc32_fid_to_hdlr_id(uint32_t fid)
{
	/* NOTE: fid 0xff00 - 0xffff are mapped to handler 0x0000 - 0x00ff */
	if ((fid & 0xff00) == 0xff00)
		return (fid & 0x00ff);
	return fid;
}

apm_oem_svc_handler_t get_smc32_svc_hdlr(uint32_t fid)
{
	uint32_t hid = smc32_fid_to_hdlr_id(fid);

	return (smc32_svc_hdlr_tbl[hid]) ? smc32_svc_hdlr_tbl[hid] : NULL;
}

int smc32_oem_svc_register(uint32_t fid, apm_oem_svc_handler_t handler)
{
	uint32_t hid = smc32_fid_to_hdlr_id(fid);

	if (smc32_svc_hdlr_tbl[hid])
		return -EBUSY;

	smc32_svc_hdlr_tbl[hid] = handler;
	return 0;
}

void smc32_oem_svc_unregister(uint32_t fid)
{
	uint32_t hid = smc32_fid_to_hdlr_id(fid);

	smc32_svc_hdlr_tbl[hid] = NULL;
}

apm_oem_svc_handler_t get_smc64_svc_hdlr(uint32_t fid)
{
	return (smc64_svc_hdlr_tbl[fid]) ? smc64_svc_hdlr_tbl[fid] : NULL;
}


int smc64_oem_svc_register(uint32_t fid, apm_oem_svc_handler_t handler)
{
	if (smc64_svc_hdlr_tbl[fid])
		return -EBUSY;

	smc64_svc_hdlr_tbl[fid] = handler;
	return 0;
}

void smc64_oem_svc_unregister(uint32_t fid)
{
	smc64_svc_hdlr_tbl[fid] = NULL;
}

apm_oem_svc_handler_t get_smc_svc_hdlr(uint32_t fid)
{
	if ((fid & SMC64_OEM_SVC_ID) == SMC64_OEM_SVC_ID)
		return get_smc64_svc_hdlr(fid & SMC64_OEM_SVC_CALL_MASK);
	else
		return get_smc32_svc_hdlr(fid & SMC32_OEM_SVC_CALL_MASK);
}

static uint64_t smc32_oem_svc_call_revision(uint32_t smc_fid, uint64_t x1,
			uint64_t x2, uint64_t x3, uint64_t x4,
			void *cookie, void *handle, uint64_t flags)
{
	SMC_RET1(handle, APM_OEM_SVC_VERSION);
}

void smc32_oem_services_init(void)
{
	/* All SMC32 APM OEM services need to be registered here */
	smc32_oem_svc_register(SMC32_OEM_SVC_CALL_REV,
				&smc32_oem_svc_call_revision);
}

void smc64_oem_services_init(void)
{
	/* All SMC64 APM OEM services need to be registered here */
	spi_nor_proxy_register();
	mem_svc_register();
	svc_platform_info_register();
	smpmpro_proxy_register();
	nvparam_proxy_register();
	i2c_proxy_register();
	ras_proxy_register();
	svc_failsafe_proxy_register();
}
