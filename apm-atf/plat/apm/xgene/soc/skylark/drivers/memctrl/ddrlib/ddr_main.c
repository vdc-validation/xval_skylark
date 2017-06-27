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

static void ddr_handle_fatal_err(void* ctx, unsigned int flag)
{
	while(1);
}

/*  Primary Library Structure Initialization  */
int ddr_sys_setup(struct apm_memc *memc, unsigned int flag)
{
        unsigned int err = 0;
        struct apm_mcu *mcu;
        unsigned int iia;

        ddr_sys_default_params_setup(memc);

        if (memc->p_user_param_overrides != NULL)
                err = memc->p_user_param_overrides(memc, 0);

	memc->p_memsys_addressmap = ddr_address_map;

	memc->p_memsys_mtest_ecc_init = memc_ecc_init_en;

	if (!memc->p_handle_fatal_err)
		memc->p_handle_fatal_err = ddr_handle_fatal_err;

        /* Initialize DDR structure */
        ddr_info("DRAM: DDR Library setup\n");

        memc->mcu_mask = 0;

	for_each_mcu(iia) {
                mcu = &(memc->mcu[iia]);
                mcu->id = iia;
                mcu->enabled = (memc->memc_ud.ud_mcu_enable_mask >> iia) & 0x1;

                mcu->mcb_id = iia / CONFIG_SYS_NUM_MCU_PER_MCB;
                mcu->mcu_rb_base  = PCP_RB_MCU0_PAGE      + (iia * PCP_RB_MCUX_OFFSET);
                mcu->dmcl_rb_base = PCP_RB_MCU0_DMCL_PAGE + (iia * PCP_RB_MCUX_OFFSET);
                mcu->dmch_rb_base = PCP_RB_MCU0_DMCH_PAGE + (iia * PCP_RB_MCUX_OFFSET);
                mcu->phyl_rb_base = PCP_RB_MCU0_PHYL_PAGE + (iia * PCP_RB_MCUX_OFFSET);
                mcu->phyh_rb_base = PCP_RB_MCU0_PHYH_PAGE + (iia * PCP_RB_MCUX_OFFSET);

                mcu->mcu_rd  = mcu_read_reg;
                mcu->mcu_wr  = mcu_write_reg;
                mcu->dmc_rd  = dmc_read_reg;
                mcu->dmc_wr  = dmc_write_reg;
                mcu->phy_rd  = phy_read_reg;
                mcu->phy_wr  = phy_write_reg;
                mcu->mcb_rd  = mcb_read_reg;
                mcu->mcb_wr  = mcb_write_reg;
        }

        /* Setup apm_memc with generic values */
        err = populate_mc_default_params(memc);

        return err;
}


/*  Primary DDR initialization task */
int ddr_sys_init(struct apm_memc *memc, unsigned int flag)
{
	int err;

	err = ddr_pre_training(memc, 0);
	if (err) {
		ddr_err("PHY pre-training error [%d]\n", err);
		return err;
	}

#if (!XGENE_EMU && !XGENE_VHP)
	err = ddr_phy_training(memc, 0);
	if (err) {
		ddr_err("PHY training error [%d]\n", err);
		return err;
	}
#endif

	err = ddr_post_training(memc, 0);
	if (err) {
		ddr_err("PHY post-training error [%d]\n", err);
		return err;
	}
	return 0;
}
