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

SOC_DIR					:= plat/apm/xgene/soc/skylark
$(eval $(call add_define,SOC_DIR))

ERROR_DEPRECATED 		:=1
$(eval $(call add_define,ERROR_DEPRECATED))

PLATFORM_PRIMARY_CPU		:= 0
$(eval $(call add_define,PLATFORM_PRIMARY_CPU))

PLATFORM_CLUSTER_COUNT			:= 16
$(eval $(call add_define,PLATFORM_CLUSTER_COUNT))

PLATFORM_MAX_CPUS_PER_CLUSTER		:= 2
$(eval $(call add_define,PLATFORM_MAX_CPUS_PER_CLUSTER))

PLATFORM_EFUSE_CPU		:= 0
$(eval $(call add_define,PLATFORM_EFUSE_CPU))

ARM_GIC_ARCH		:=	3
$(eval $(call add_define,ARM_GIC_ARCH))

PLAT_INCLUDES		+=	-Iplat/apm/xgene/include/skylark

PLAT_BL_COMMON_SOURCES	+=	drivers/arm/pl011/pl011_console.S	\
						${SOC_DIR}/drivers/ahbc/ahbc.c	\
						${SOC_DIR}/plat_setup.c

BL31_SOURCES	+= drivers/arm/gic/v3/gicv3_helpers.c		\
				drivers/arm/gic/v3/gicv3_main.c		\
				plat/common/plat_gicv3.c			\
				${SOC_DIR}/plat_psci_handlers.c	\
				${SOC_DIR}/plat_psci_mbox.c

include ${SOC_DIR}/drivers/clk/clk.mk
include plat/apm/xgene/common/drivers/i2c/i2clib.mk
include plat/apm/xgene/common/drivers/smpro/smprolib.mk
include plat/apm/xgene/common/drivers/ras/ras.mk
include ${SOC_DIR}/drivers/ras/ras.mk
include plat/apm/xgene/common/drivers/wdt/wdt.mk
include ${SOC_DIR}/drivers/memctrl/ddrlib.mk
