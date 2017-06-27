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
#ifndef _SMPRO_MISC_H_
#define _SMPRO_MISC_H_ 1

#define SMPRO_CFG_CONSOLE		16
#define SMPRO_CFG_CONSOLE_DISABLE	0
#define SMPRO_CFG_CONSOLE_ENABLE	1

#define SMPRO_CFG_I2C			17
#define SMPRO_CFG_I2C_DISABLE		0
#define SMPRO_CFG_I2C_ENABLE		1
#define SMPRO_CFG_I2C_INPROGRESS	2

/*
 * Read from SMpro register
 *
 * reg	- Absolute SMpro register to read from
 * val	- Pointer to an uint32_t
 * urgent - Urgent bit to process msg in irq context
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 -  -EINVAL	If operation not permitted
 *  0		Success
 *
 * NOTE: Not all registers are allowed.
 */
int plat_smpro_read(uint32_t reg, uint32_t *val, int urgent);

/*
 * Write to SMpro register
 *
 * reg	- Absolute SMpro register to write
 * val	- Pointer to an uint32_t
 * urgent - urgent bit to process msg in irq context
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  0		Success
 *
 * NOTE: To determine if an register is allowed, do an read operation first.
 */
int plat_smpro_write(uint32_t reg, uint32_t val, int urgent);

/*
 * Change SMpro configuration
 *
 * cfg_type - Configuration type as define above
 * value    - Value of configuration
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  0		Success
 */
int plat_smpro_cfg_write(uint8_t cfg_type, uint8_t value);

/*
 * Retrieve SMpro configuration
 *
 * cfg_type - Configuration type as define above
 * value    - Pointer to a value of configuration
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 -  -EINVAL	If operation not permitted
 *  0		Success
 */
int plat_smpro_cfg_read(uint8_t cfg_type, uint8_t *value);

/*
 * SMpro Information
 */
struct smpro_info {
	union {
		struct {
			unsigned int adv_mode		: 1;
			unsigned int tpc_en		: 1;
			unsigned int avs_en		: 1;
			unsigned int reset_cap		: 1;
			unsigned int poweroff_cap	: 1;
			unsigned int ipp_version	: 1;
			unsigned int console		: 1;
			unsigned int master_pmd		: 7;
			unsigned int rsvd1		: 18;
		} cap_bits;
		unsigned int cap;
	} cap;
	union {
		struct {
			unsigned int dd		: 8;
			unsigned int mm		: 8;
			unsigned int yyyy	: 16;
		} date_bits;
		unsigned int date;
	} build;
	union {
		struct {
			unsigned int minor : 4;
			unsigned int major : 4;
		}ver_bits;
		unsigned int version;
	}fw_ver;
};

/*
 * Retrieve SMpro Info
 *
 * info		- Pointer to an info structure
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 -  -EINVAL	If operation not permitted
 *  0		Success
 *
 * NOTE: This function requires timer support for delay. It can not be
 * used for early stage ATF boot whether the timer isn't setup. For early
 * ATF boot, please use plat_smpro_early_get_info instead.
 */
int plat_smpro_get_info(struct smpro_info *info);

/*
 * Early Retrieve SMpro Info
 *
 * info		- Pointer to an info structure.
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro if no actual HW (model)
 *  0		Success
 *
 * This function is for early boot code where the system is not fully
 * configured. Once the ATF subsystem is fully configured, use the function
 * plat_smpro_get_info instead.
 */
int plat_smpro_early_get_info(struct smpro_info *info);

/*
 * Retrieve SMpro failsafe info
 *
 * info_type - Info type
 * value     - Pointer to a value of failsafe info
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  -EINVAL	If operation not permitted
 *  0		Success
 */
int plat_smpro_failsafe_get_info(uint8_t info_type, uint32_t *value);

/*
 * Set SMpro failsafe info
 *
 * info_type - Info type
 * value     - Pointer to a value of failsafe info
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  0		Success
 */
int plat_smpro_failsafe_set_info(uint8_t info_type, uint32_t *value);

/*
 * Enable SMpro BMC DIMM temperature scanning
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  0		Success
 */
int plat_enable_smpro_bmc_dimm_scan(void);

/*
 * Enable/Disable SMpro RAS scanning for errors
 *
 * Return:
 *  -ETIMEDOUT	Operation failed to talk to SMpro
 *  0		Success
 */
int plat_smpro_ras_scan(uint8_t enable);
#endif
