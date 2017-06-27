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

int mcu_unreset(struct apm_memc *ddr)
{
        unsigned int data = 0;
        int err = 0;
        unsigned int iia, mcu_avail;

        /*
         * Prior to here we detect DIMM presence.
         */
        /* MCUs are unisolated by PMPro */

        /* Wait 20 ns to allow the MCB-MCU interface to become active */
        DELAYN(20);

        /* Write MCBn.MCUxPLLCR0 to set PD before configuring PLL */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_PD_SET(data, 1);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Write MCBn.MCUxPLLCR0 to set FBDIVC to 0x28 for stable power up */
        for_each_mcu(iia) {
               if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_FBDIVC_SET(data, 0x28);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Write MCBn.MCUxPLLCR0 to clear PD */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_PD_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Wait 2ms (PLL to stabilize) */
        DELAY(2000);

        /* Write MCBn.MCUxPLLCR0 to set ENABLE_CLK */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_PLLCLKEN_SET(data, 1);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Write MCBn.MCUxPLLCR0 to set FBDIVC to desired divide ratio (others unchanged) */
        for_each_mcu(iia) {
               if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                mcu_set_pll_params(&ddr->mcu[iia].mcu_pllctl, ddr->mcu[iia].mcu_ud, ddr->mcu[iia].ddr_info.t_ck_ps);
                data = PCP_RB_MCBX_MCU0PLLCR0_ODIV3_SET(data,  ddr->mcu[iia].mcu_pllctl.pllctl_outdiv3);
                data = PCP_RB_MCBX_MCU0PLLCR0_ODIV2_SET(data,  ddr->mcu[iia].mcu_pllctl.pllctl_outdiv2);
                data = PCP_RB_MCBX_MCU0PLLCR0_FBDIVC_SET(data, ddr->mcu[iia].mcu_pllctl.pllctl_fbdivc);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Write MCBn.MCUxPLLCR0 to set PLLRST */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_PLLRST_SET(data, 1);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Wait for 200ns (PLL minimum reset assertion time) */
        DELAYN(200);

        /* Write MCBn.MCUxPLLCR0 to clear PLLRST */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia));
                data = PCP_RB_MCBX_MCU0PLLCR0_PLLRST_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_pllcr0_page_offset(iia), data);
        }

        /* Wait for 250us (PLL lock time) */
        DELAY(250);

        /* Poll MCBn.MCUxPLLSR and wait for LOCK to be set */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                err = mcb_poll_reg(ddr->mcu[iia].mcb_id, get_mcu_pllsr_page_offset(iia), 1, 0xFFFFFFFF);
                if (err) {
			ddr_err("MCU[%d]: PLLSR LOCK not set\n", iia);
			return err;
                }
        }

        /* Wait for 16 MCU cycles (minimum agent macro reset assertion time) */
        DELAYN(50);

        /* Write MCBn.MCUxCCR to clear ClkPd */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia));
                data = PCP_RB_MCBX_MCU0CCR_CLKPD_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia), data);
        }

        /* Wait for 200ns for agent macro to power up and the DCC to establish lock */
        DELAYN(200);

        /* Write MCBn.MCUxCCR to clear ClkRst */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia));
                data = PCP_RB_MCBX_MCU0CCR_CLKRST_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia), data);
        }

        /* Wait for 16 MCU cycles (maximum agent macro reset deassertion time) */
        DELAYN(50);

        /* Write MCBn.MCUxCCR to set ClkEn */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia));
                data = PCP_RB_MCBX_MCU0CCR_CLKEN_SET(data, 1);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_ccr_page_offset(iia), data);
        }

        /* Wait for 25 cycles */
        DELAYN(100);

        /* Deassert AsyncResetMcux from the SOC */
        err = update_async_reset_mcu(ddr, 0);
        if (err) {
		ddr_err("MCU Async Reset not asserted\n");
		return err;
        }

        /* Wait for 20 ns for AsyncResetMcux deassertion to propagate [PCPIS] */
        DELAYN(20);

        /* Write MCBn.MCUxRCR to clear DMCAPBRST (others unchanged) */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_rcr_page_offset(iia));
                data = PCP_RB_MCBX_MCU0RCR_DMCAPBRST_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_rcr_page_offset(iia), data);
        }

        /* Wait for 2 MCU cycles (DMC reset requirement) [MCUDS] */
        DELAYN(1);

        /* Write MCBn.MCUxRCR to clear RST and INTFCRST */
        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                data = mcb_read_reg(ddr->mcu[iia].mcb_id, get_mcu_rcr_page_offset(iia));
                data = PCP_RB_MCBX_MCU0RCR_PHYRST_SET(data, 0);
                data = PCP_RB_MCBX_MCU0RCR_RST_SET(data, 0);
                data = PCP_RB_MCBX_MCU0RCR_INTFCRST_SET(data, 0);
                mcb_write_reg(ddr->mcu[iia].mcb_id, get_mcu_rcr_page_offset(iia), data);
        }

        /* Wait for 25 MCU cycles (maximum reset deassertion time) */
        DELAYN(10);

        /* Write RB_MASTER.RBASR1 to set McuxAvail (others unchanged) */
        data = iob_read_reg( PCP_RB_RBM_PAGE, PCP_SYSMAP_RB_RBM_RBASR1_OFFSET);
        mcu_avail = PCP_RB_RBM_RBASR1_MCUAVAIL_RD(data);

        for_each_mcu(iia) {
                if (!ddr->mcu[iia].enabled)
                        continue;
                mcu_avail |= (1 << iia);
                data = PCP_RB_RBM_RBASR1_MCUAVAIL_SET(data, mcu_avail);
        }
        iob_write_reg( PCP_RB_RBM_PAGE, PCP_SYSMAP_RB_RBM_RBASR1_OFFSET, data);

        DELAYP(10);
        return err;
}

/*
 * Perform clock init and reset initialization sequence
 * for an MCU.
 */
int change_mcu_phy_dll_reset(struct apm_mcu *mcu)
{
	unsigned int data;

	/* Write MCBn.MCUxRCR to clear RST and INTFCRST */
	data = mcb_read_reg(mcu->mcb_id, get_mcu_rcr_page_offset(mcu->id));
	data = PCP_RB_MCBX_MCU0RCR_PHYRST_SET(data, 0);
	data = PCP_RB_MCBX_MCU0RCR_RST_SET(data, 0);
	data = PCP_RB_MCBX_MCU0RCR_INTFCRST_SET(data, 0);
	data = PCP_RB_MCBX_MCU0RCR_PHYDLLRST_SET(data, 0);
	mcb_write_reg(mcu->mcb_id, get_mcu_rcr_page_offset(mcu->id), data);

	/* Poll for DFI complete */
	return dmc_poll_reg(mcu->id, USER_STATUS_ADDR, 1, 1 << APM_USER_STATUS_INIT_CMPLT_BIT);
}

/* Setup Memory for turning on ECC */
int memc_ecc_init_en(struct apm_memc *memc, unsigned int flag)
{
	struct apm_mcu *mcu;
	unsigned int iia;
	unsigned int errcnt = 0;
	unsigned long long *addr_ptr;
	unsigned long long data2;

	ddr_verbose("INFO: ECC initialization of ddr space starting...\n");
	ddr_info("\nStarting ECC Init...\n");

	/*
	 * Disable MCU RdPoison and WrPoison reporting
	 * Disable RAM and DRAM Correctable and Uncorrectable Error reporting
	 * Since, caches are enabled, this is done to avoid errors due to prefetching
	 */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;

		if (!ecc_enable(mcu->id))
			continue;

		/* ECC related Interrupt controls */
		dmc_ecc_interrupt_ctrl(mcu->id, 0, 0, 0, 0);

		/* Set RdPosionDisable, WrPoisonDisable in MCUECR */
		mcu_poison_disable_set(mcu->id);
        }

	/* Init ECC syndrom by partial write into each 64B location */
	memc->p_show_ecc_progress(memc, 0);
	DMB_SY_CALL;
	data2 = 0;
	for (iia = 0; iia < memc->memspace.num_mem_regions; iia++) {
		addr_ptr = (unsigned long long *)(memc->memspace.str_addr[iia]);
		ddr_verbose("\n Region[%d]: 0x%010llX - 0x%010llX\n", iia,
			   (unsigned long long)addr_ptr,
			   (unsigned long long)memc->memspace.end_addr[iia]);
		for (;addr_ptr < ((unsigned long long *)(memc->memspace.end_addr[iia]));
		      addr_ptr += 8) {
			__dc_zva(addr_ptr);

			if (!((unsigned long)addr_ptr & 0x7FFFFFF)) {
				memc->p_show_ecc_progress(memc, 1);
				data2++;
			}
			if (data2 == 16) {
				data2 = 0;
				memc->p_show_ecc_progress(memc, 2);
			}
		}
		ddr_verbose("\n");
	}
	DMB_SY_CALL;
	memc->p_cache_flush(memc, 0);
	DSB_SY_CALL;
	ISB;
	memc->p_show_ecc_progress(memc, 3);

	ddr_verbose("\nINFO: ECC init delay before enabled...\n");
	/* Clear ECC status registers and Enable ECC reporting */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		/* Clear ECC status registers */
		dmc_ecc_errc_clear(mcu->id);
		dmc_ecc_errd_clear(mcu->id);
		dmc_ram_err_clear(mcu->id);
		dmc_link_err_clear(mcu->id);
        }
	DMB_SY_CALL;

	ddr_verbose("INFO: ECC init delay after enabled...\n");
	DMB_SY_CALL;
	/* Spot check for ECC errors */
	for (iia = 0; iia < memc->memspace.num_mem_regions; iia++) {
		addr_ptr = (unsigned long long *)memc->memspace.str_addr[iia];
		ddr_verbose("\n Region[%d]: 0x%010llX - 0x%010llX\n", iia,
			   (unsigned long long)addr_ptr,
			   (unsigned long long)memc->memspace.end_addr[iia]);
		for (; addr_ptr <
		    (unsigned long long *)(memc->memspace.end_addr[iia] - 2 * 8 * 0x40008 + 1);
		    addr_ptr += 0x40008) {
			if (*addr_ptr != 0x0) {
				ddr_err("Read Data mismatch after ecc init sa=%p "
				"(act=0x%llx vs exp=0)\n",(void *) addr_ptr, *addr_ptr);
				errcnt++;
			}
		}
	}
	DMB_SY_CALL;

	if (errcnt)
		ddr_err("Spot check for ECC errors ... FAIL [%d]!\n", errcnt);
	memc->p_cache_flush(memc, 0);

	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;

		/* Check for Errors */
		errcnt += dmc_error_status(mcu->id);
        }
	if (errcnt) {
		ddr_err("ECC init [%d]!\n", errcnt);
		return (errcnt != 0) ? -1 : 0;
	}
	DSB_SY_CALL;
	ISB;

        /* Turn poison progation on */
	/* Clear ECC status registers and Enable ECC reporting */
	for_each_mcu(iia) {
		mcu = (struct apm_mcu *)&memc->mcu[iia];
		if (!mcu->enabled)
			continue;
		/* Clear ECC status registers */
		dmc_ecc_errc_clear(mcu->id);
		dmc_ecc_errd_clear(mcu->id);
		dmc_ram_err_clear(mcu->id);
		dmc_link_err_clear(mcu->id);

		/* Clear RdPosionDisable, WrPoisonDisable in MCUECR */
		mcu_poison_disable_clear(mcu->id);
        }
	return 0;
}
