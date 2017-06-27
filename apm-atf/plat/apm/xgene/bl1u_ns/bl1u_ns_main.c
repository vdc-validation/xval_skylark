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
#include <assert.h>
#include <auth_mod.h>
#include <bl_common.h>
#include <bl1.h>
#include <bl1u_ns_private.h>
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include <stdint.h>

static meminfo_t *ns_bl1u_dram_layout;

extern void ns_bl1u_platform_setup(void);
extern meminfo_t *plat_ns_bl1u_get_mem_layout(void);

void ns_bl1u_main(void)
{
	image_info_t		image_data;
	int					rc;
	uint64_t			smc_ret;

	NOTICE("NS_BL1U: %s\n", version_string);
	NOTICE("NS_BL1U: %s\n", build_message);

	/* Perform platform setup in NS_BL1U */
	ns_bl1u_platform_setup();

	ns_bl1u_dram_layout = plat_ns_bl1u_get_mem_layout();

	/* Use version 1 for image data */
	image_data.h.version = VERSION_1;

	/* Load FWU_CERT */
	VERBOSE("NS_BL1U: Loading FWU_CERT\n");
	rc = load_image(ns_bl1u_dram_layout, FWU_CERT_ID, NS_FWU_CERT_BASE,
				&image_data, NULL);
	if (rc) {
		ERROR("NS_BL1U: Failed to load FWU_CERT.\n");
		plat_error_handler(rc);
	}

	/* Copy FWU_CERT to BL1 */
	VERBOSE("NS_BL1U: Copying FWU_CERT\n");
	smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_COPY,
				(uint64_t)FWU_CERT_ID,
				(uint64_t)image_data.image_base,
				(uint64_t)image_data.image_size,
				(uint64_t)image_data.image_size);
	if (smc_ret) {
		ERROR("NS_BL1U: Failed to copy FWU_CERT.\n");
		plat_error_handler(smc_ret);
	}

	/* AUTH FWU_CERT */
	VERBOSE("NS_BL1U: Authorizing FWU_CERT\n");
	smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_AUTH,
				(uint64_t)FWU_CERT_ID,
				0,
				0,
				0);
	if (smc_ret) {
		ERROR("NS_BL1U: Failed to AUTH FWU_CERT.\n");
		plat_error_handler(smc_ret);
	}

	/* Load SCP_BL2U */
	VERBOSE("NS_BL1U: Loading SCP_BL2U\n");
	rc = load_image(ns_bl1u_dram_layout, SCP_BL2U_IMAGE_ID,
			NS_SCP_BL2U_BASE, &image_data, NULL);
	if (!rc) {
		/* Copy SCP_BL2U to BL1 */
		VERBOSE("NS_BL1U: Copying SCP_BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_COPY,
					(uint64_t)SCP_BL2U_IMAGE_ID,
					(uint64_t)image_data.image_base,
					(uint64_t)image_data.image_size,
					(uint64_t)image_data.image_size);
		if (smc_ret) {
			ERROR("NS_BL1U: Failed to copy SCP_BL2U.\n");
			plat_error_handler(smc_ret);
		}

		/* AUTH SCP_BL2U */
		VERBOSE("NS_BL1U: Authorizing SCP_BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_AUTH,
					(uint64_t)SCP_BL2U_IMAGE_ID,
					0,
					0,
					0);
		if (smc_ret) {
			ERROR("NS_BL1U: Failed to AUTH SCP_BL2U.\n");
			plat_error_handler(smc_ret);
		}
	} else
		NOTICE("NS_BL1U: Failed to load SCP_BL2U.\n");

	/* Load BL2U */
	VERBOSE("NS_BL1U: Loading BL2U\n");
	rc = load_image(ns_bl1u_dram_layout, BL2U_IMAGE_ID,
			NS_BL2U_BASE, &image_data, NULL);
	if (!rc) {
		/* Copy BL2U to BL1 */
		VERBOSE("NS_BL1U: Copying BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_COPY,
					(uint64_t)BL2U_IMAGE_ID,
					(uint64_t)image_data.image_base,
					(uint64_t)image_data.image_size,
					(uint64_t)image_data.image_size);
		if (smc_ret) {
			ERROR("NS_BL1U: Failed to copy BL2U.\n");
			plat_error_handler(smc_ret);
		}

		/* AUTH BL2 */
		VERBOSE("NS_BL1U: Authorizing BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_AUTH,
					(uint64_t)BL2U_IMAGE_ID,
					0,
					0,
					0);
		if (smc_ret) {
			ERROR("NS_BL1U: Failed to AUTH BL2U.\n");
			plat_error_handler(smc_ret);
		}

		VERBOSE("NS_BL1U: Executing BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_EXECUTE,
					(uint64_t)BL2U_IMAGE_ID,
					0,
					0,
					0);
		if (smc_ret) {
			ERROR("NS_BL1U: Executed BL2U failed.\n");
			plat_error_handler(smc_ret);
		}
	} else
		NOTICE("NS_BL1U: Failed to load BL2U.\n");

#if 0
	/* Not need NS_BL2U for now */
	/* Load NS_BL2U */
	VERBOSE("NS_BL1U: Loading BL2U\n");
	rc = load_image(ns_bl1u_dram_layout, NS_BL2U_IMAGE_ID,
			NS_BL2U_BASE, &image_data, NULL);
	if (!rc) {
		/* AUTH NS_BL2U */
		VERBOSE("NS_BL1U: Authorizing NS_BL2U\n");
		smc_ret = smc_request((uint64_t)FWU_SMC_IMAGE_AUTH,
					(uint64_t)NS_BL2U_IMAGE_ID,
					(uint64_t)image_data.image_base,
					(uint64_t)image_data.image_size,
					0);
		if (smc_ret) {
			ERROR("NS_BL1U: Failed to AUTH NS_BL2U.\n");
			plat_error_handler(smc_ret);
		}

		VERBOSE("NS_BL1U: Jump to  NS_BL2U\n");
		/* now jump to NS_BL2U in the same exception level */
		((void (*)(void))(image_data.image_base))();

		/* Shouldn't come here */
		ERROR("NS_BL1U: Failed to run NS_BL2U.\n");
		plat_error_handler(smc_ret);
	} else
		NOTICE("NS_BL1U: Failed to load NS_BL2U.\n");
#endif

	VERBOSE("NS_BL1U: FWU DONE");
	smc_ret = smc_request((uint64_t)FWU_SMC_SEC_IMAGE_DONE,
				0,
				0,
				0,
				0);
}
