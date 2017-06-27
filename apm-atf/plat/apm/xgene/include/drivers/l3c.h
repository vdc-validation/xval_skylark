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

/*************************************************************************
 * Provide the L3C functionality to compute the over all L3 cache enabled
 * on the SOC and to set the Snoop/Alloc Enable bits in all L3Cs.
 ************************************************************************/

#ifndef __L3C_H__
#define __L3C_H__

/*
 * Read Register Bus. Used to access L3C registers.
 *
 * addr:	L3C register bus physical address
 * @return:	Read value from RB address
 */
uint32_t l3c_read_rb(uintptr_t addr);

/*
 * Write Register Bus. Used to write to L3C registers.
 *
 * addr:	L3C register bus physical address
 * val:		Value to write to RB address
 */
void l3c_write_rb(uintptr_t addr, uint32_t val);

/*
 * Set the mask in Register Bus at specific physical address.
 *
 * addr:	L3C register bus physical address
 * mask:	Mask value to set in specific RB address
 */
void l3c_pcp_rb_reg_setmask(uintptr_t addr, uint32_t mask);

/*
 * Set the Snoop Enable & Alloc Enable bits in CSWSER1 for each
 * of the L3Cs.
 *
 * active_l3cs:	Mask indicating the snoop/alloc enable bits
 */
void l3cs_enable_snoop_alloc(uint32_t active_l3cs);

/*
 * Compute the over all L3C size configured.
 */
uint64_t l3cs_get_size(void);

/*
 * Enable all L3C caches.
 */
uint32_t l3cs_init(void);

#endif /* __L3C_H__ */
