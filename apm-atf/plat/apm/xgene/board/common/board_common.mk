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


PLAT_INCLUDES		+=	-Iplat/apm/xgene/include/	\
				-Iplat/apm/xgene/include/drivers/

PLAT_BL_COMMON_SOURCES	+=	plat/apm/xgene/board/common/board_common.c
PLAT_BL_COMMON_SOURCES	+=	plat/apm/xgene/board/common/board_err_common.c

ifneq (${TRUSTED_BOARD_BOOT},0)
    # ROTPK hash location
    ifeq (${XGENE_ROTPK_LOCATION}, regs)
        XGENE_ROTPK_LOCATION_ID = XGENE_ROTPK_REGS_ID
    else ifeq (${XGENE_ROTPK_LOCATION}, devel_rsa)
        XGENE_ROTPK_LOCATION_ID = XGENE_ROTPK_DEVEL_RSA_ID
    else
        $(error "Unsupported XGENE_ROTPK_LOCATION value")
    endif
    $(eval $(call add_define,XGENE_ROTPK_LOCATION_ID))

    # Certificate NV-Counters. Use values corresponding to tied off values in
    # XGene development platforms
    TFW_NVCTR_VAL	?=	31
    $(eval $(call add_define,TFW_NVCTR_VAL))
    NTFW_NVCTR_VAL	?=	223
    $(eval $(call add_define,NTFW_NVCTR_VAL))

    BL1_SOURCES		+=	plat/apm/xgene/board/common/board_xgene_trusted_boot.c
    BL2_SOURCES		+=	plat/apm/xgene/board/common/board_xgene_trusted_boot.c
endif
