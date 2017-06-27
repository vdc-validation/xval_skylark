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

#ifndef __SKYLARK_PCP_CSR_H__
#define __SKYLARK_PCP_CSR_H__

#define PCP_CPUX_REG_OFFSET		0x00100000

#define CPU_CRCR			0x00000004
#define  CRCR_CWRR_MASK			0x00000002

#define CPUX_CPU_REG(core, offset) \
	(PCP_CPU_REG_BASE + (core) * PCP_CPUX_REG_OFFSET + (offset))

#define CSW_P0CCR_REG_OFFSET		0x200
#define  CSW_PXCCR_CLKRATIO_MASK	0x00000040
#define  CSW_PXCCR_CLKRATIO_SHIFT	8
#define CSW_PxCCR_PMD_REG(pmd) \
	(PCP_CSW_REG_BASE + CSW_P0CCR_REG_OFFSET + (pmd) * 0x10)

#define PMD_L2C_L2CR			0x00000000
#define  L2CR_HIPDIS			0x00000020
#define  L2CR_HDPDIS			0x00000008

#define PMDX_L2C_REG(pmd, offset) \
	(PCP_CPU_L2C_REG_BASE + (pmd) * PCP_CPUX_REG_OFFSET * \
			PLATFORM_MAX_CPUS_PER_CLUSTER + (offset))

#endif /* __SKYLARK_PCP_CSR_H__ */
