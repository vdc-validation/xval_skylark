#
# Copyright (c) 2016 Applied Micro Circuits Corporation.
# All rights reserved.
#
# This software is available to you under a choice of one of two
# licenses.  You may choose to be licensed under the terms of the GNU
# General Public License (GPL) Version 2, available from the file
# COPYING in the main directory of this source tree, or the
# OpenIB.org BSD license below:
#
#     Redistribution and use in source and binary forms, with or
#     without modification, are permitted provided that the following
#     conditions are met:
#
#      - Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      - Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials
#        provided with the distribution.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED.
#

PLAT_SVC_DIR	:=	plat/apm/xgene/common/services

BL31_SOURCES	+=	${PLAT_SVC_DIR}/apm_oem_svc/apm_oem_svc_main.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/apm_oem_services.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/spi_nor_proxy/spi_nor_proxy.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/sys_mem_info/sys_mem_info.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/platform_info/platform_info.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/nvparam_proxy/nvparam_proxy.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/smpmpro_proxy/smpmpro_proxy.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/i2c_proxy/i2c_proxy.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/ras_proxy/ras_proxy.c	\
			${PLAT_SVC_DIR}/apm_oem_svc/failsafe_proxy/failsafe_proxy.c
