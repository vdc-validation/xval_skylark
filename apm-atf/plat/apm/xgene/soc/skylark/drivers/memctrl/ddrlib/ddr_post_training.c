/*
 * Copyright (c) 2016 Applied Micro Circuits Corporation.
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED.
 */

/*
 * This is the main init function file. The init is attached to the
 * apm_memc structure. These are the top level init function used
 * to initialize the system's ddr controllers.
 */
#include "ddr_lib.h"

/*  Primary DDR initialization task */
int ddr_post_training(struct apm_memc *memc, unsigned int flag)
{
	struct apm_mcu *mcu;
	int err, iia;

	if (memc->p_memsys_addressmap != NULL) {
		ddr_info("\n------- MCU Address Map Config --------\n");
		err = memc->p_memsys_addressmap(memc, 0);
		if (err) {
			ddr_err("\nAddress Map ERROR [%d]\n", err);
			return err;
		}
	}

	if (memc->p_memsys_tlb_map != NULL) {
		ddr_info("\n------- DDR mem-space TLB Setup --------\n");
		err = memc->p_memsys_tlb_map(memc, 0);
		if (err) {
			ddr_err("\nTLB Map ERROR [%d]\n", err);
			return err;
		}
	}

        /* Update DMC and PHY post training */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		/* TODO */
#if 0
		err = mcu_post_train_setup(mcu);
		if (err) {
			ddr_err("MCU post train setup  ERROR [%d]\n", err);
			return err;
		}
#endif
		/* TODO Enable PHY IO Cal */
#if 0 //!XGENE_EMU
		phy_post_train_setup(mcu);
#endif
	}

	ddr_verbose("DRAM: memc post setup DONE!\n");

	/* Set RdPosionDisable, WrPoisonDisable in MCUECR as ECC init is not complete */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;

		if (!ecc_enable(mcu->id))
			continue;

		/* Set RdPosionDisable, WrPoisonDisable in MCUECR */
		mcu_poison_disable_set(mcu->id);
	}

        /* Configure DMC is READY state */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		err = dmc_ready_state(mcu->id);
		if (err) {
			ddr_err("MCU-DMC[%d]: DMC didn't entry Ready state\n", iia);
			return err;
		}
	}

	/* Enable MCB's to active MCU traffic */
	for_each_mcu(iia) {
		if (!memc->mcu[iia].enabled)
			continue;

		/* One control bit per MCB */
		if ((iia != 0) & (iia != 4U))
			continue;

		mcb_write_reg(memc->mcu[iia].mcb_id,
			      PCP_RB_MCBX_MCBDISABLE_PAGE_OFFSET, 0);
		ddr_verbose("MCB[%d]: Clear MCBDISABLE \n",
			    memc->mcu[iia].mcb_id);
	}

	/*
	 * MEMC - test memory
	 * initialize with ECC syndrome and enable ECC
	 */
#if !XGENE_VHP
	err = memc->p_memsys_mtest_ecc_init(memc, 0);
	if (err) {
		ddr_err("ECCinit ERROR [%d]\n", err);
		return err;
	}
#endif
	ddr_verbose("DRAM: memc init DONE!\n");

	return 0;
}

