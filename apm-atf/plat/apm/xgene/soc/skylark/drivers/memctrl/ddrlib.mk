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

DDR_LOG_LEVEL	:= 30
$(eval $(call add_define,DDR_LOG_LEVEL))

PLAT_MEM_DIR	:=	${SOC_DIR}/drivers/memctrl

BL1_SOURCES	+=	${PLAT_MEM_DIR}/memctrl.c			\
			${PLAT_MEM_DIR}/apm_ddr.c			\
			${PLAT_MEM_DIR}/ddrlib/ddr_addrmap.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_dmc_config.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_dmc_utils.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_main.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_mcu_bist_utils.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_mcu_config.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_mcu_utils.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_pcp_config.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_pcp_utils.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_config.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_hw_training.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_lpbk.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_sw_training.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_training.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_phy_utils.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_post_training.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_pre_training.c	\
			${PLAT_MEM_DIR}/ddrlib/ddr_spd_utils.c		\
			${PLAT_MEM_DIR}/ddrlib/ddr_ud_params.c
