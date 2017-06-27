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

#include <arch.h>
#include <debug.h>
#include <xgene_pm_tracer.h>

#if XGENE_PM_TRACER
#define trace_log(...)		tf_printf("psci: " __VA_ARGS__)
#else
#define trace_log(...)
#endif

void trace_power_flow(unsigned long idx, unsigned char mode)
{
	switch (mode) {
	case CPU_ON:
		trace_log("core %ld ON\n", idx);
		break;
	case CPU_OFF:
		trace_log("core %ld OFF\n", idx);
		break;
	case CPU_STANDBY:
		trace_log("core %ld STANDBY\n", idx);
		break;
	case CPU_RETEN:
		trace_log("core %ld RETEN\n", idx);
		break;
	case CPU_PWRDN:
		trace_log("core %ld PWRDN\n", idx);
		break;
	case CLUSTER_RETEN:
		trace_log("cluster %ld RETEN\n", idx);
		break;
	case CLUSTER_PWRDN:
		trace_log("cluster %ld PWRDN\n", idx);
		break;
	case SOC_RETEN:
		trace_log("soc %ld RETEN\n", idx);
		break;
	case SOC_PWRDN:
		trace_log("soc %ld PWRDN\n", idx);
		break;
	case SYSTEM_RESET:
		trace_log("system RESET\n");
		break;
	case SYSTEM_OFF:
		trace_log("system OFF\n");
		break;
	default:
		trace_log("unknown power mode\n");
		break;
	}
}
