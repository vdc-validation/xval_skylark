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

PLAT_INCLUDES	+= -Iplat/apm/xgene/include/board/osprey/ \
		   -Iplat/apm/xgene/include/board/
BOARD_DIR	:= plat/apm/xgene/board/osprey

XGENE_VHP			:= 0
$(eval $(call add_define,XGENE_VHP))

XGENE_EMU			:= 0
$(eval $(call add_define,XGENE_EMU))

PLAT_BL_COMMON_SOURCES	+=

ifneq (${TRUSTED_BOARD_BOOT},0)
	include plat/apm/xgene/bl1u_ns/bl1u_ns.mk
endif

include plat/apm/xgene/soc/skylark/platform.mk
include plat/apm/xgene/board/common/board_common.mk
ifeq (${XGENE_VHP},1)
	include plat/apm/xgene/common/drivers/mtd/emu/spi_nor.mk
else
	include plat/apm/xgene/common/drivers/mtd/stmicro/spi_nor.mk
endif
