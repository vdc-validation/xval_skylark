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

#ifndef __SKYLARK_SOC_H__
#define __SKYLARK_SOC_H__

#define PCP_RB_BASE_ADDR			0x7c000000
#define PCP_CPU_REG_BASE			0x7c0c0000
#define PCP_CPU_L2C_REG_BASE			0x7c0d0000
#define PCP_CSW_REG_BASE			0x7e200000
#define SCU_CSR_REG_BASE			0x1f000000
#define SMPRO_SCU_CSR_REG_BASE			0x1f000000
#define PMPRO_SCU_CSR_REG_BASE			0x1f040000

#define AHBC_CLKRST_REG_BASE			0x1f10c000
#define UART0_REG_BASE				0x12600000
#define UART1_REG_BASE				0x12610000
#define UART2_REG_BASE				0x12620000
#define UART3_REG_BASE				0x12630000
#define UART4_REG_BASE				0x12640000
#define I2C2_REG_BASE				0x12690000
#define I2C3_REG_BASE				0x126a0000
#define I2C4_REG_BASE				0x126b0000
#define I2C5_REG_BASE				0x126c0000
#define GICD_REG_BASE				0x78000000
#define GICR_REG_BASE				0x78400000
#define SPI0_REG_BASE				0x12670000
#define SPI1_REG_BASE				0x12680000
#define GT_CNTCTLBASE_REG_BASE			0x12700000
#define WDT_NS_REG_BASE				0x127c0000
#define SMPRO_SEC_MB_REG_BASE			0x10530000
#define PMPRO_SEC_MB_REG_BASE			0x11530000
#define OCM_BASE				0x1d000000
#define OCM_SIZE				0x80000
#define SOC_EFUSE_SHADOW_BASE			0x1054a000

#endif /* __SKYLARK_SOC_H__ */
