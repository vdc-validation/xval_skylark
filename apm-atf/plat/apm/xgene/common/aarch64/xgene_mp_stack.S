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
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>

	.local	platform_normal_stacks
	.local	platform_secondary_stacks
	.weak	plat_get_my_stack
	.weak	plat_set_my_stack
	.globl	platform_get_stack
	.globl	platform_set_stack

	/* ---------------------------------------------------------------------
	 * When the compatility layer is disabled, the new platform APIs
	 * viz plat_get_my_stack() and plat_set_my_stack() are
	 * supported by the platform and the previous APIs platform_get_stack()
	 * and platform_set_stack() are defined in terms of new APIs making use
	 * of the fact that they are only ever invoked for the current CPU.
	 * This is to enable components of Trusted Firmware like SPDs using the
	 * old platform APIs to continue to work.
	 * --------------------------------------------------------------------
	 */

	/* -------------------------------------------------------
	 * unsigned long platform_get_stack (unsigned long mpidr)
	 *
	 * For the current CPU, this function returns the stack
	 * pointer for a stack allocated in device memory. The
	 * 'mpidr' should correspond to that of the current CPU.
	 * This function is deprecated and plat_get_my_stack()
	 * should be used instead.
	 * -------------------------------------------------------
	 */
func_deprecated platform_get_stack
#if ASM_ASSERTION
	mrs	x1, mpidr_el1
	cmp	x0, x1
	ASM_ASSERT(eq)
#endif
	b	plat_get_my_stack
endfunc_deprecated platform_get_stack

	/* -----------------------------------------------------
	 * void platform_set_stack (unsigned long mpidr)
	 *
	 * For the current CPU, this function sets the stack pointer
	 * to a stack allocated in normal memory. The
	 * 'mpidr' should correspond to that of the current CPU.
	 * This function is deprecated and plat_get_my_stack()
	 * should be used instead.
	 * -----------------------------------------------------
	 */
func_deprecated platform_set_stack
#if ASM_ASSERTION
	mrs	x1, mpidr_el1
	cmp	x0, x1
	ASM_ASSERT(eq)
#endif
	b	plat_set_my_stack
endfunc_deprecated platform_set_stack

	/* -----------------------------------------------------
	 * unsigned long plat_get_my_stack ()
	 *
	 * For the current CPU, this function returns the stack
	 * pointer for a stack allocated in device memory.
	 *
	 * For the master core, stack allocates at platform_normal_stacks region with
	 * size is PLATFORM_STACK_SIZE.
	 * For the secondary cores, stack allocates at platform_secondary_stacks
	 * region with size is BL1_SECONDARY_STACK_SIZE.
	 * -----------------------------------------------------
	 */
func plat_get_my_stack
	mov	x10, x30 // lr
	bl	plat_is_my_cpu_primary
	cbz	x0, 1f
	ldr	x0, =(platform_normal_stacks + PLATFORM_STACK_SIZE)
	b	2f
1:
	bl	plat_my_core_pos
	sub x0, x0, #1
	ldr	x2, =(platform_secondary_stacks + BL1_SECONDARY_STACK_SIZE)
	mov	x1, #BL1_SECONDARY_STACK_SIZE
	madd	x0, x0, x1, x2
2:
	ret	x10
endfunc plat_get_my_stack

	/* -----------------------------------------------------
	 * void plat_set_my_stack ()
	 *
	 * For the current CPU, this function sets the stack
	 * pointer to a stack allocated in normal memory.
	 * -----------------------------------------------------
	 */
func plat_set_my_stack
	mov	x9, x30 // lr
	bl 	plat_get_my_stack
	mov	sp, x0
	ret	x9
endfunc plat_set_my_stack

	/* -----------------------------------------------------
	 * Per-cpu stacks in normal memory.
	 * Master core get a stack of PLATFORM_STACK_SIZE bytes.
	 * Seconcdary core get a stack of BL1_SECONDARY_STACK_SIZE bytes.
	 * -----------------------------------------------------
	 */
declare_stack platform_normal_stacks, tzfw_normal_stacks, \
		PLATFORM_STACK_SIZE, 1
declare_stack platform_secondary_stacks, tzfw_normal_stacks, \
		BL1_SECONDARY_STACK_SIZE, (PLATFORM_CORE_COUNT - 1)
