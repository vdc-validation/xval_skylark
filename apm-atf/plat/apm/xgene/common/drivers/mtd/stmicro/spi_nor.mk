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

MTD_DBG		:= 0
$(eval $(call add_define,MTD_DBG))

COMMON_DRV_DIR	:=	plat/apm/xgene/common/drivers

MTD_SOURCE_FILES := ${COMMON_DRV_DIR}/mtd/stmicro/spi_nor.c	\
					${COMMON_DRV_DIR}/spi/spi.c	\
					${COMMON_DRV_DIR}/spi/spi_flash.c

BL1_SOURCES +=	${MTD_SOURCE_FILES}
BL31_SOURCES +=	${MTD_SOURCE_FILES}
