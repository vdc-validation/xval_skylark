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

#include "ddr_lib.h"

int mcu_phy_config_early_rddata_en(void *ptr)
{
  /*	struct apm_mcu *mcu = (struct apm_mcu *)ptr;
	mcu_param_t *mp     = &mcu->mcu_params;
	timing_params_t *tp = &mcu->timing_params;
	phy_param_t *p_phy = &mcu->phy_params;*/
	/* Set Phy early Read Data Enable if IE mode always on is zero */
	/* check if already set - TBD LRDIMM */
  //	if (p_phy->phy_ie_mode == 0) {
		/* Phy IO Pads (if not always one) must be enabled 7ns before
		 *  Phy internal Rd data enable is needed.
		 *  Read data Enable may be split between Mcu & Phy
		 *   whether phy_ie_mode is on/off
		 */
  /*		p_phy->phy_rddata_en_ie_dly = cdiv(7000, tp->tCKmin_X_ps);
	}
	if (p_phy->phy_rddata_en_ie_dly > mp->cl) {
		p_phy->phy_ie_mode = 1;
		p_phy->phy_rddata_en_ie_dly = 0;
	}
#if APM_DDRLIB_ALLOW_USERDEFS
	if (mcu->mcu_ud.ud_phy_ie_mode >= 0)
		p_phy->phy_ie_mode = mcu->mcu_ud.ud_phy_ie_mode;
	if (mcu->mcu_ud.ud_phy_rddata_en_ie_dly >= 0)
		p_phy->phy_rddata_en_ie_dly = mcu->mcu_ud.ud_phy_rddata_en_ie_dly;
                #endif*/
	return 0;
}           /* mcu_phy_config_early_rddata_en  */

int mcu_post_train_setup(void *ptr)
{
  //struct apm_mcu *mcu = (struct apm_mcu *)ptr;
  //	mcu_param_t *p = &mcu->mcu_params;
	unsigned int err = 0;

	/* Disable ECC error detection */
        /*	mcu->mcu_wr(mcu, MCU_REG_MCUGECR, 0x10000);

	mcu->mcu_wr(mcu, MCU_REG_MCULSCTL, p->lsleep_ctrl);
	mcu->mcu_wr(mcu, MCU_REG_MCUDSCTL, p->dsleep_ctrl);
	mcu->mcu_wr(mcu, MCU_REG_MCUSLEEPCTL, p->sleep_ctrl);
	mcu->mcu_wr(mcu, MCU_REG_RAMSLEEPCTL, p->sram_sleep_ctrl);
	*/
	/* Set to not timout and to stack 4 REFs */
        //	mcu->mcu_wr(mcu, MCU_REG_PHYUPDTCFG, 0x333);
	/* Set Mcu Ctrl Update to min value - done every tREFI in Cdn MC  */
	//mcu->mcu_wr(mcu, MCU_REG_CTLUPDTCFG, 0x121);

	return err;
}

int setOdtMR1MR2_wrlvl(void *ptr, unsigned int rank)
{
	int err = 0;
        /*	int unsigned fsm_init_seq = 0;
	int unsigned fsm_init_ctl = 0;
	int unsigned mr2val, mr1val, wdata, tgtrank_mask,
		s0_refl_mask, s1_refl_mask;
	int unsigned __attribute__ ((unused)) rddata;
	int unsigned iia;
	struct apm_mcu *mcu = (struct apm_mcu *)ptr;
	*/
	/*
	 *  PEVM: WR leveling
	 *
	 *  Must turn off MR2 dynamic odt before WrLvl
	 *  And only allow MR1 RttNom { RZQ/2 or RZQ/4 or RZQ/6 }
	 *
	 *  And must limit ODT used to only few ranks (target & reflection)
	 *
	 */
        /*	ddr_info("\nMCU[%d] Setting custom MR-RTT values for WR-LVL mode\n",
		   mcu->id);
	fsm_init_ctl =
	    (1U << MCU_DDR_INIT_CTL_CALIBGO_LSB) |
	    ((0x4U & 0xFU) << MCU_DDR_INIT_CTL_RCWAIT_LSB) |
	    ((0x1U & 0x7U) << MCU_DDR_INIT_CTL_REFRESHCNT_LSB);
        */
    /* MR1 & MR3 per rank level values
     * Pick target rank and reflection rank ODT for Wr Lvl
     *    based on populated ranks
     */
        /*	s0_refl_mask = 0;
	s1_refl_mask = 0;
	tgtrank_mask = (0x1U << rank);
        if (tgtrank_mask & 0xCC) {
                s0_refl_mask = 0x1;
        }
        if (tgtrank_mask & 0x33) {
                s1_refl_mask = 0x4;
        }

        for (iia = 0; iia < MCU_SUPPORTED_RANKS; iia++) {
                mr1val = mcu->mcu_rd(mcu, (MCU_REG_MRS_1_0_VALUE + iia *4) );

                if ( (0x1U << iia) == tgtrank_mask) {
                        wdata = putRttNomintoMR1(mr1val, mcu->mcu_params.rttnom_wrlvl);
                } else if ((0x1U << iia) & s0_refl_mask) {
                        wdata = putRttNomintoMR1(mr1val, mcu->mcu_params.rttnom_wrlvl_refl);
                } else if ((0x1U << iia) & s1_refl_mask) {
                        wdata = putRttNomintoMR1(mr1val, mcu->mcu_params.rttnom_wrlvl_refl);
                } else {
                        wdata = putRttNomintoMR1(mr1val, RttNomZero);
		}
	        wdata |= 0x80; *//* Put all ranks in WriteLeveling mode */
        /*		mcu->mcu_wr(mcu, (MCU_REG_MRS_1_0_VALUE + iia *4), wdata);
		ddr_info("\tWR-LVL Rank[%d]: MCU FSM Setting MR1-RTTnom = 0x%x (%dohm)\n",
			   iia, wdata, rttnom_inohm(wdata));

		mr2val =mcu->mcu_rd(mcu, (MCU_REG_MRS_2_0_VALUE + iia *4) );
		wdata = mr2val & 0xFFFFF9FF;
		mcu->mcu_wr(mcu, (MCU_REG_MRS_2_0_VALUE + iia *4), wdata);
		ddr_info("\tWR-LVL Rank[%d]: MCU FSM Setting MR2-RTTwr = 0x%x Off\n",
			   iia, wdata);

	}

	fsm_init_seq = (1U << MCU_DDR_INIT_SEQ_MRENAB_LSB) |
		(1U << MCU_DDR_INIT_SEQ_TXPRENAB_LSB)|
		(DO_SW_CTLUPDATE << MCU_DDR_INIT_SEQ_CTLUPDATE_LSB);
	mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_SEQ, fsm_init_seq);
	mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_CTL, fsm_init_ctl);*/	/* Kick off FSM */

	return err;
}

int setOdtMR1MR2_funcmode(void *ptr)
{
  /*	int err = 0;
	int unsigned fsm_init_seq = 0;
	int unsigned fsm_init_ctl = 0;
	int unsigned mr2val, mr1val, wdata;
	int unsigned __attribute__ ((unused)) rddata;
	int unsigned iia;
	struct apm_mcu *mcu = (struct apm_mcu *)ptr;

	ddr_info("\nMCU[%d] Setting custom MR-RTT values for Func mode\n",
		   mcu->id);

	for (iia = 0; iia < MCU_SUPPORTED_RANKS; iia++) {
		mr1val = mcu->mcu_rd(mcu, (MCU_REG_MRS_1_0_VALUE + iia *4) );
		wdata = putRttNomintoMR1(mr1val, RttNom40ohm);
		mcu->mcu_wr(mcu, (MCU_REG_MRS_1_0_VALUE + iia *4), wdata);
		ddr_info("\tWR-LVL Rank[%d]: MCU FSM Setting MR1-RTTnom = 0x%x (%dohm)\n",
			   iia, wdata, rttnom_inohm(wdata));

		mr2val =mcu->mcu_rd(mcu, (MCU_REG_MRS_2_0_VALUE + iia *4) );
		wdata = mr2val & 0xFFFFF9FF;
		mcu->mcu_wr(mcu, (MCU_REG_MRS_2_0_VALUE + iia *4), wdata);
		ddr_info("\tWR-LVL Rank[%d]: MCU FSM Setting MR2-RTTwr  = 0x%x Off\n",
			   iia, wdata);
	}
	fsm_init_seq = (1U << MCU_DDR_INIT_SEQ_MRENAB_LSB) |
		(1U << MCU_DDR_INIT_SEQ_TXPRENAB_LSB)|
		(DO_SW_CTLUPDATE << MCU_DDR_INIT_SEQ_CTLUPDATE_LSB);
	fsm_init_ctl =
	    (1U << MCU_DDR_INIT_CTL_CALIBGO_LSB) |
	    ((0x4U & 0xFU) << MCU_DDR_INIT_CTL_RCWAIT_LSB) |
	    ((0x1U & 0x7U) << MCU_DDR_INIT_CTL_REFRESHCNT_LSB);
	mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_SEQ, fsm_init_seq);
	mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_CTL, fsm_init_ctl);*/	/* Kick off FSM */

	return 0;
}


void mcu_bist_sm_config(struct apm_mcu *mcu, unsigned int rank, unsigned int physliceon)

{
  /*
    struct mcu_param *p_mcp = &(mcu->mcu_params);
    unsigned int row, col, bank, fsm_init_ctl, fsm_init_seq;
    unsigned int regd, foo, bctrl;

    row = MCU_BIST_ROW;
    col = MCU_BIST_COL;
    bank= MCU_BIST_BANK;

    fsm_init_ctl =
        (0x1U << MCU_DDR_INIT_CTL_CALIBGO_LSB) |
        ((0x4U & 0xFU) << MCU_DDR_INIT_CTL_RCWAIT_LSB) |
        ((0x1U & 0x7U) << MCU_DDR_INIT_CTL_REFRESHCNT_LSB);

        foo = mcu->mcu_rd(mcu, MCU_REG_DDR_INIT_STATUS);*/
    /* Must turn off Mem_init_done to access DFI Csrs (Bist Wr data & Chk)*/
  /*    if ((foo >> MCU_DDR_INIT_SEQ_SETMEMINITDONE_LSB) & 0x1) {
        fsm_init_seq =
            (1 << MCU_DDR_INIT_SEQ_TXPRENAB_LSB) |
            (DO_SW_CTLUPDATE << MCU_DDR_INIT_SEQ_CTLUPDATE_LSB);
        mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_SEQ, fsm_init_seq);
        mcu->mcu_wr(mcu, MCU_REG_DDR_INIT_CTL, fsm_init_ctl);*/
        /* minitdone = 1; */
  /*    }

    mcu->mcu_wr(mcu, MCU_REG_BISTROWCOL,
            (((row & 0xFFFF) << 16) | (col & 0xFFFF)));

    regd = FIELD_BISTRDGAP_RD2WRGAP_WR(p_mcp->rank_rdwr) |
        FIELD_BISTRDGAP_RD2RDGAP_WR(p_mcp->rank_rdrd);
    mcu->mcu_wr(mcu, MCU_REG_BISTRDGAP,regd);

    regd = FIELD_BISTWRGAP_WR2RDGAP_WR(p_mcp->rank_wrrd) |
        FIELD_BISTWRGAP_WR2WRGAP_WR(p_mcp->rank_wrwr) ;
    mcu->mcu_wr(mcu, MCU_REG_BISTWRGAP,regd);

    bctrl = FIELD_BISTCTL_BANK_WR(bank) |
        FIELD_BISTCTL_WR0RANK_WR(rank) |
        FIELD_BISTCTL_WR1RANK_WR(rank) |
        FIELD_BISTCTL_RD0RANK_WR(rank) |
        FIELD_BISTCTL_RD1RANK_WR(rank) |
        FIELD_BISTCTL_PEEKPOKE_WR(0) |
        FIELD_BISTCTL_DOWR_WR(1) |
        FIELD_BISTCTL_DORD_WR(1) ;
        mcu->mcu_wr(mcu, MCU_REG_BISTCTL, bctrl);*/

    /* Adjust for macro enabled */
  /*    regd = FIELD_BISTCHKCTL_DATAMASK_WR( (0xFFFF & physliceon) ) |
        FIELD_BISTCHKCTL_ECCMASK_WR( ((physliceon >> 16) & 0x3) ) |
        FIELD_BISTCHKCTL_ALTERNATEEN_WR(0) |
        FIELD_BISTCHKCTL_STOPEN_WR(1) ;
        mcu->mcu_wr(mcu, MCU_REG_BISTCHKCTL, regd);*/

}
