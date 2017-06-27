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

BL1U_NS_DIR		:=	plat/apm/xgene/bl1u_ns

INCLUDES		+=	-I${BL1U_NS_DIR}

BL1U_NS_SOURCES		+=	${BL1U_NS_DIR}/bl1u_ns_main.c			\
				${BL1U_NS_DIR}/aarch64/bl1u_ns_request.S		\
				${BL1U_NS_DIR}/aarch64/bl1u_ns_entrypoint.S		\
				plat/common/aarch64/platform_up_stack.S			\
				lib/cpus/aarch64/cpu_helpers.S		\
				common/aarch64/early_exceptions.S

BL1U_NS_LINKERFILE	:=	${BL1U_NS_DIR}/bl1u_ns.ld.S
