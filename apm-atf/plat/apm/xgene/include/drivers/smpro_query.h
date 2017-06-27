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
#ifndef _SMPRO_QUERY_H_
#define _SMPRO_QUERY_H_ 1

#include <arch.h>
#include <arch_helpers.h>
#include <apm_oem_svc.h>
#include <config.h>
#include <debug.h>
#include <errno.h>
#include <platform_def.h>

/*
 * Get the SMPro Runtime Firmware Version No
 *
 * buff   - A pointer to a string to store the version in format:
 *           MM.NN
 * length - Length of buffer on input. On output, length of the version
 *           text.
 *
 * Return: < 0 on error. Otherwise 0 for sucessess
 */
int plat_smpro_get_smpro_fw_ver(uint8_t *buff, uint32_t *length);

/* Get the SMPro Runtime Firmware Build Date
 *
 * buff   - A pointer to a string to store the build info in format:
 *           YYYYMMDD
 * length - Length of buffer on input. On output, length of the version
 *           text.
 *
 * Return: < 0 on error. Otherwise 0 for sucessess
 */
int plat_smpro_get_smpro_fw_build(uint8_t *buff, uint32_t *length);

#endif /* _SMPRO_QUERY_H_ */
