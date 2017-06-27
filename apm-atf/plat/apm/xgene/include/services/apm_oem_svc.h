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

#ifndef __APM_OEM_SVC_H__
#define __APM_OEM_SVC_H__

#include <runtime_svc.h>

/*
 * SMC32 FID format:
 *   31:16 - APM OEM SMC SVC identifier
 *   15:0  - Service calls
 */
#define SMC32_OEM_SVC_ID			0x83000000
#define SMC32_OEM_SVC_CALL_MASK			0x0000ffff
#define SMC32_OEM_SVC_HNDL_CNT			512
#define SMC64_OEM_SVC_ID			0xc300ff00
#define SMC64_OEM_SVC_CALL_MASK			0x000000ff
#define SMC64_OEM_SVC_HNDL_CNT			256

/* APM OEM Service Calls version numbers */
#define APM_OEM_SVC_VERSION_MAJOR		(0x1 << 16)
#define APM_OEM_SVC_VERSION_MINOR		0x0
#define APM_OEM_SVC_VERSION \
	(APM_OEM_SVC_VERSION_MAJOR | APM_OEM_SVC_VERSION_MINOR)

typedef uint64_t (*apm_oem_svc_handler_t)(uint32_t smc_fid, uint64_t x1,
				uint64_t x2, uint64_t x3, uint64_t x4,
				void *cookie, void *handle, uint64_t flags);
apm_oem_svc_handler_t get_smc_svc_hdlr(uint32_t fid);

int smc32_oem_svc_register(uint32_t fid, apm_oem_svc_handler_t handler);
void smc32_oem_svc_unregister(uint32_t fid);
void smc32_oem_services_init(void);
int smc64_oem_svc_register(uint32_t fid, apm_oem_svc_handler_t handler);
void smc64_oem_svc_unregister(uint32_t fid);
void smc64_oem_services_init(void);


enum smc32_apm_oem_svc_call {
	/*
	 * 0x0000 - 0x00FF - reserved for mapping standard service
         * from SMC 0xFF00 - 0xFFFF
	 */
	/* Add new service call above this comment (0x0100 - 0xfeff) */
	SMC32_OEM_SVC_CALL_CNT		= 0xff00,
	SMC32_OEM_SVC_UID		= 0xff01,
	SMC32_OEM_RSVD			= 0xff02,
	SMC32_OEM_SVC_CALL_REV		= 0xff03,
	/*
	 * 0xff04 - 0xffff : reserved for future expansion
	 * NOTE: 0xff00 service call are mapped to (call_val & 0x00ff)
	 */
	SMC32_OEM_SVC_CALL_MAX		= 0xffff
};

enum smc64_apm_oem_svc_call {
	SMC_FLASH			= 0x00,
	SMC_MEM				= 0x01,
	SMC_PLATFORM_INFO		= 0x02,
	SMC_SMPMPRO_PROXY		= 0x03,
	SMC_XNV_PARAM			= 0x04,
	SMC_XI2C			= 0x05,
	SMC_FAILSAFE			= 0x06,
	SMC_XRAS			= 0x07,
	/* Add new service call above this comment */
	SMC64_OEM_SVC_CALL_MAX		= 0xff
};

#endif /* __APM_OEM_SVC_H__ */
