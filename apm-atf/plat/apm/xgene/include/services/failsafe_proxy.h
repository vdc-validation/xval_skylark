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

#ifndef _FAILSAFE_PROXY_H_
#define _FAILSAFE_PROXY_H_

#include <assert.h>

enum failsafe_status {
	FAILSAFE_BOOT_NORMAL = 0,
	FAILSAFE_BOOT_LAST_KNOWN_SETTINGS,
	FAILSAFE_BOOT_DEFAULT_SETTINGS,
	FAILSAFE_BOOT_SUCCESSFUL
};

/*
 * Retrieve failsafe status
 *
 * status: pointer to failsafe status value
 *
 * Return:
  * -ETIMEDOUT   Timeout when retrieve the stastus from SMPro
 *  -EINVAL      Parameter is invalid
 *  0            Success
 */
int failsafe_proxy_get_status(enum failsafe_status *status);

void svc_failsafe_proxy_register(void);

#endif /* _FAILSAFE_PROXY_H_ */
