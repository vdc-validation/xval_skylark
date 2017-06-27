#
# Copyright (c) 2016, Applied Micro Circuits Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

CRASH_REPORTING		:=	1
$(eval $(call add_define,CRASH_REPORTING))

ASM_ASSERTION		:=	1
$(eval $(call add_define,ASM_ASSERTION))

XGENE_PM_TRACER		:=	0
$(eval $(call add_define,XGENE_PM_TRACER))

USE_COHERENT_MEM	:=	0

COMMON_DIR		:=	plat/apm/xgene/common

PLAT_INCLUDES		+=	-Iinclude/bl1/tbbr	\
				-Iinclude/common/tbbr/	\
				-Iinclude/plat/common/	\
				-Iinclude/plat/arm/common/		\
				-Iinclude/plat/arm/common/aarch64/		\
				-Iplat/apm/xgene/include	\
				-Iplat/apm/xgene/include/drivers	\
				-Iplat/apm/xgene/include/services

PLAT_BL_COMMON_SOURCES	+=	lib/xlat_tables/xlat_tables_common.c	\
				lib/xlat_tables/aarch64/xlat_tables.c		\
				plat/common/aarch64/plat_common.c	\
				drivers/delay_timer/delay_timer.c	\
				${COMMON_DIR}/xgene_delay_timer.c	\
				${COMMON_DIR}/xgene_fip_helper.c			\
				${COMMON_DIR}/aarch64/xgene_common.c		\
				lib/cpus/aarch64/xgene.S		\
				plat/apm/xgene/common/xgene_nvparam.c

BL1_SOURCES		+=	drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				${COMMON_DIR}/aarch64/xgene_helpers.S		\
				${COMMON_DIR}/xgene_bl1_setup.c			\
				${COMMON_DIR}/xgene_io_storage.c		\
				${COMMON_DIR}/xgene_fwu.c			\
				bl1/tbbr/tbbr_img_desc.c			\
				${COMMON_DIR}/xgene_bl1_fwu.c		\
				${COMMON_DIR}/aarch64/xgene_mp_stack.S	\
				plat/apm/xgene/common/services/apm_oem_svc/failsafe_proxy/failsafe_proxy.c

BL2_SOURCES		+=	drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				${COMMON_DIR}/aarch64/xgene_helpers.S		\
				${COMMON_DIR}/xgene_bl2_setup.c			\
				${COMMON_DIR}/xgene_io_storage.c		\
				${COMMON_DIR}/xgene_fwu.c			\
				plat/common/aarch64/platform_up_stack.S

BL31_SOURCES		+=	drivers/arm/gic/common/gic_common.c		\
				plat/common/aarch64/plat_psci_common.c		\
				plat/common/aarch64/platform_mp_stack.S		\
				${COMMON_DIR}/aarch64/xgene_helpers.S		\
				${COMMON_DIR}/xgene_bl31_setup.c		\
				${COMMON_DIR}/xgene_pm.c			\
				${COMMON_DIR}/xgene_fwu.c			\
				${COMMON_DIR}/xgene_pm_tracer.c			\
				${COMMON_DIR}/xgene_gic.c			\
				${COMMON_DIR}/xgene_irq.c			\
				${COMMON_DIR}/xgene_topology.c

ifneq (${TRUSTED_BOARD_BOOT},0)

	# By default, XGene platforms use RSA keys
	KEY_ALG			:=	rsa

	# Include common TBB sources
	AUTH_SOURCES	:=	drivers/auth/auth_mod.c				\
				drivers/auth/crypto_mod.c			\
				drivers/auth/img_parser_mod.c		\
				drivers/auth/tbbr/tbbr_cot.c

	PLAT_INCLUDES	+=	-Iinclude/bl1/tbbr

	BL1_SOURCES		+=	${AUTH_SOURCES}

	BL2_SOURCES		+=	${AUTH_SOURCES}

	BL2U_SOURCES	+=	${COMMON_DIR}/xgene_bl2u_setup.c		\
				${COMMON_DIR}/xgene_fwu.c			\
				plat/common/aarch64/platform_up_stack.S

	BL1U_NS_SOURCES	+=	drivers/io/io_fip.c			\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				${COMMON_DIR}/xgene_bl1u_ns_setup.c	\
				${COMMON_DIR}/xgene_io_storage.c	\
				${COMMON_DIR}/xgene_fwu.c

    ifneq (${SCP_BL2U},)
    $(eval $(call FWU_FIP_ADD_IMG,SCP_BL2U,--scp-fwu-cfg))
    endif

	MBEDTLS_KEY_ALG	:=	${KEY_ALG}

	# We expect to locate the *.mk files under the directories specified below
	CRYPTO_LIB_MK := drivers/auth/mbedtls/mbedtls_crypto.mk
	IMG_PARSER_LIB_MK := drivers/auth/mbedtls/mbedtls_x509.mk

    $(info Including ${CRYPTO_LIB_MK})
    include ${CRYPTO_LIB_MK}

    $(info Including ${IMG_PARSER_LIB_MK})
    include ${IMG_PARSER_LIB_MK}

endif

$(info Including services/services.mk)
include ${COMMON_DIR}/services/services.mk
