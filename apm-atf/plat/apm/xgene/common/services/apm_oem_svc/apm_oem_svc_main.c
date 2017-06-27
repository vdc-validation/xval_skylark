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
#include <debug.h>
#include <runtime_svc.h>
#include <uuid.h>

/* APM OEM Service UUID */
DEFINE_SVC_UUID(apm_oem_svc_uid,
		0x92396ffe, 0x2f63, 0x11e6, 0xac, 0x61,
		0x9e, 0x71, 0x12, 0x8c, 0xae, 0x77);

/* Setup APM OEM Services */
static int32_t apm_oem_svc_setup(void)
{
	smc32_oem_services_init();
	smc64_oem_services_init();
	return 0;
}

/*
 * Top-level APM OEM Service SMC handler. This handler will in turn dispatch
 * calls to APM SMC subsequence handler
 */
uint64_t apm_oem_svc_smc_handler(uint32_t smc_fid,
				uint64_t x1,
				uint64_t x2,
				uint64_t x3,
				uint64_t x4,
				void *cookie,
				void *handle,
				uint64_t flags)
{
	apm_oem_svc_handler_t handler;

	handler = get_smc_svc_hdlr(smc_fid);
	/* All returned registers x0-x3 are updated by the found handler */
	if (handler)
		return handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags);

	WARN("Unimplemented APM OEM Service Call: 0x%x\n", smc_fid);
	SMC_RET1(handle, SMC_UNK);
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		oem_svc,
		OEN_OEM_START,
		OEN_OEM_END,
		SMC_TYPE_FAST,
		apm_oem_svc_setup,
		apm_oem_svc_smc_handler
);
