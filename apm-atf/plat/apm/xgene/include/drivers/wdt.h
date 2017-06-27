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
 *
 * Skylark SoC has two SBSA watch dog timers - one secure and one non-secure.
 * The secure watch dog timer interrupt WS0 is routed to IRQ94 EL3 and WS1 is
 * routed to SMpro. The non-secure watch dog timer interrupt WS0 is routed
 * to IRQ92 for EL2 and WS1 to IRQ93 for EL3.
 *
 * Here is the processing logic for each IRQ's:
 *
 * Non-secure WS0 - EL2 or BIOS/OS
 * Non-secure WS1 - EL3 (ATF) - This driver
 * Secure WS0 - EL3 (ATF) - Handled else where as this is part of failsafe
 * Secure WS1 - SMpro
 *
 * This module will wire up the IRQ handled only. For non-secure WS1, the
 * BIOS or OS will enable the interrupt and configure the watch dog timer.
 */

#ifndef __WDT_H__
#define __WDT_H__

/*
 * Initialize watch dog timer
 */
void xgene_wdt_init(void);

#endif /* __WDT_H__ */
