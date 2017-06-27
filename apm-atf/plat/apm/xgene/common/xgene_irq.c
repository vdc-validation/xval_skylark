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

#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <errno.h>
#include <interrupt_mgmt.h>
#include <platform.h>
#include <platform_def.h>
#include <xgene_irq.h>

static interrupt_type_handler_t intr_func_handlers[MAX_IRQ_HANDLERS];
static uint32_t el3_intr_id_tbl[MAX_IRQ_HANDLERS];
static uint32_t num_irq_registered;

static uint64_t xgene_irq_main_handler(uint32_t id, uint32_t flags,
				    void *handle, void *cookie)
{
	uint32_t intrHandlerFlag;
	uint32_t irqnr;
	int i;

        /* Check the security state when the exception was generated */
        assert(get_interrupt_src_ss(flags) == NON_SECURE);

	/* Check if the incoming interrupt is a special interrupt */
	assert(!gicv3_is_intr_id_special_identifier);

	/* Obtain INTID and send the acknowledge for the interrupt */
	irqnr = plat_ic_acknowledge_interrupt();

	/* Check the INTID is same as the heighest pending interrupt */
	assert((irqnr == id));

	intrHandlerFlag = 1;
	for (i = 0; i < num_irq_registered; i++)
		if(id == el3_intr_id_tbl[i]) {
			intrHandlerFlag = intr_func_handlers[i](id, flags,
						handle, cookie);
			break;
		}

	/* Check if the interrupt handler is absent for the interrupt */
	assert(!intrHandlerFlag);

	/* Send completion of the interrupt to the cpu interface */
	plat_ic_end_of_interrupt(irqnr);

	return intrHandlerFlag;
}

/*
 * Register a interrupt handler
 *
 * @id:		Interrupt ID
 * @handler:	Function called when a interrupt happened.
 * @return:	-EINVAL if parameter is invalid
 *              -EPERM if permission not allowed
 *              Negative value if non-volatile operation write failed
 *              Otherwise, 0 for success
 */
int xgene_irq_register_interrupt(uint32_t id, interrupt_type_handler_t handler)
{
	if (num_irq_registered >= MAX_IRQ_HANDLERS)
		return -EPERM;

	if (!handler)
		return -EINVAL;

	el3_intr_id_tbl[num_irq_registered] = id;
	intr_func_handlers[num_irq_registered++] = handler;

	return 0;
}

void xgene_irq_init(void)
{
	uint64_t flags;

	/*
	 * Register an interrupt handler for EL3 interrupts
	 * when generated during code executing in the
	 * non-secure state.
	 */
	num_irq_registered = 0;
	flags = 0;
	set_interrupt_rm_flag(flags, NON_SECURE);
	register_interrupt_type_handler(INTR_TYPE_EL3, xgene_irq_main_handler,
					flags);
}
