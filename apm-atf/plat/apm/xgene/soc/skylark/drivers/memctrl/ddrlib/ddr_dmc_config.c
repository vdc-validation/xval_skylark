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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED.
 */

#include "ddr_lib.h"

static struct dmc_param dmc;

void config_dmc_timing_parameters(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int value, jedec_value, odt_value, t_rfc = 0;
	/* Always use slot 0 */
	generic_spd_eeprom_t *spd = &mcu->spd_info[0].spd_info;

	/* tck in ps */
	unsigned int t_ck_ps	    = mcu->ddr_info.t_ck_ps;
	unsigned int crc_enable     = mcu->mcu_ud.ud_crc_enable;
	unsigned int write_preamble = mcu->ddr_info.write_preamble;
	unsigned int read_preamble  = mcu->ddr_info.read_preamble;
	unsigned int read_latency   = mcu->ddr_info.read_latency;
	unsigned int write_latency  = mcu->ddr_info.write_latency;
	unsigned int read_postamble = 1;
	unsigned int write_postamble = 1;
	unsigned int burst_length = 8;

	/*
	 * Apply SPD timing parameters
	 * t_refi, DDR3 Spec, Page 158, Table 61 and
	 * DDR4 Spec, Page 36, Table 23 defines the value of tREFI as 7.8us
	 * the controller value is for between each refresh so should be divided by 8
	 */
	/* TODO: t_rfc_cs for 3DS */
	dmc->t_refi_next = cdiv(975000, t_ck_ps); /* 975ns = 7800ns/8 */

	switch (mcu->mcu_ud.ud_refresh_granularity) {
	case 0:
		t_rfc = compute_trfc1_timing(spd);
		break;
	case 1:
		t_rfc = compute_trfc2_timing(spd);
		break;
	case 2:
		t_rfc = compute_trfc4_timing(spd);
		break;
	}

	/* t_rfc */
	value = cdiv(t_rfc, t_ck_ps); /* t_rfc <= 8 * t_refi */
	dmc->t_rfc_next = FIELD_DMC520_T_RFC_NEXT_SET(dmc->t_rfc_next, value);

	/* t_rfcfg */
	value = cdiv(t_rfc, t_ck_ps); /* t_rfc <= 8 * t_refi */
	dmc->t_rfc_next = FIELD_DMC520_T_RFCFG_NEXT_SET(dmc->t_rfc_next, value);

	/* t_rfc_cs */
	/*
	 * t_rfc_cs must be less than or equal to t_rfc.
	 * • For x1 refresh_granularity t_rfc_cs must be less than or equal to t_refi.
	 * • For x2 refresh_granularity t_rfc_cs must be less than or equal to (t_refi / 2).
	 * • For x4 refresh_granularity t_rfc_cs must be less than or equal to (t_refi / 4).
	 */
	value = cdiv(975000 / (1 << mcu->mcu_ud.ud_refresh_granularity), t_ck_ps);
	dmc->t_rfc_next = FIELD_DMC520_T_RFC_CS_NEXT_SET(dmc->t_rfc_next, value);

	/* Refresh Granularity */
	dmc->refresh_control_next = FIELD_DMC520_REFRESH_GRANULARITY_NEXT_SET(dmc->refresh_control_next,
				    mcu->mcu_ud.ud_refresh_granularity);
	/* t_mrr */
	/* Should be greater than tMPRR which is 1 for both DDR3 and DDR4 */
	dmc->t_mrr_next = 2;

	/* t_mrw */
	/*
	 * DDR3 Spec., Page 171, Table 68 - tMOD = max(12nck, 15ns), tMRD = 4nck
	 * DDR4 Spec., Page 189, Table 191 - tMOD = max(24nck, 15ns), tMRD = 8nck
	 * Wait for tMOD and tMRD to expire
	 * DMC minimum is 12 cycles
	 */
	value = cdiv (15000, t_ck_ps);
#ifdef DDR3_MODE
	if (value < 12)
		value = 12;
#else
	if (value < 24)
		value = 24;
#endif
	dmc->t_mrw_next = FIELD_DMC520_T_MRW_NEXT_SET(dmc->t_mrw_next, value);

	/* t_mrw_cs */
	if(compute_package_type(spd) == SPD_3DS)
		dmc->t_mrw_next = FIELD_DMC520_T_MRW_CS_NEXT_SET(dmc->t_mrw_next, value);
	else
		dmc->t_mrw_next = FIELD_DMC520_T_MRW_CS_NEXT_SET(dmc->t_mrw_next, 2);

	/* t_rdpden = RL + 4 +1 as DDR4 JEDEC Spec*/
	dmc->t_rdpden_next = read_latency + 4 + 1;

	/* t_rcd */
	dmc->t_rcd_next = cdiv( compute_trcd_timing(spd), t_ck_ps);

	/* t_ras */
	dmc->t_ras_next = cdiv( compute_tras_timing(spd), t_ck_ps);

	/* t_rp */
	dmc->t_rp_next = cdiv( compute_trp_timing(spd), t_ck_ps);

	/* t_rpall */
	dmc->t_rpall_next = cdiv( compute_trp_timing(spd), t_ck_ps);

	/* t_rrd_s */
#ifdef DDR3_MODE
	value =  cdiv( compute_trrd_timing(spd), t_ck_ps);
#else
	value =  cdiv( compute_trrd_s_timing(spd), t_ck_ps);
#endif
	if (value < 4)
		value = 4;
	dmc->t_rrd_next = FIELD_DMC520_T_RRD_S_NEXT_SET(dmc->t_rrd_next, value);

	/* t_rrd_l */
#ifdef DDR3_MODE
	value =  cdiv( compute_trrd_timing(spd), t_ck_ps);
#else
	value =  cdiv( compute_trrd_l_timing(spd), t_ck_ps);
#endif
	if (value < 4)
		value = 4;
	dmc->t_rrd_next = FIELD_DMC520_T_RRD_L_NEXT_SET(dmc->t_rrd_next, value);

	/* t_rrd_cs */
	if (compute_package_type(spd) == SPD_3DS)
		value = cdiv( compute_trrd_l_timing(spd), t_ck_ps);
	else
		value = 0;
	dmc->t_rrd_next = FIELD_DMC520_T_RRD_CS_NEXT_SET(dmc->t_rrd_next, value);

	/* t_faw */
	value = cdiv( compute_tfaw_timing(spd), t_ck_ps);
	dmc->t_act_window_next = FIELD_DMC520_T_FAW_NEXT_SET(dmc->t_act_window_next, value);

	/*
 	 * t_mawi : Sets the value of the average delay required between ACTIVATE
 	 * commands to the same row to not violate tMAC in tMAW. Should be used only with trr_enable
 	 * else set it zero. DMC uses this independent of trr being enabled.
 	 * This can cause performance issue as activates are spread too far apart
 	 */
	dmc->t_act_window_next = FIELD_DMC520_T_MAWI_NEXT_SET(dmc->t_act_window_next, 0);

	/* t_rtr_s */
	dmc->t_rtr_next = FIELD_DMC520_T_RTR_S_NEXT_SET(dmc->t_rtr_next, (4 + mcu->mcu_ud.ud_rtr_s_margin)); /*Set to 4 */

	/* t_rtr_l */
	value = cdiv( compute_tccd_l_timing(spd), t_ck_ps);
	dmc->t_rtr_next = FIELD_DMC520_T_RTR_L_NEXT_SET(dmc->t_rtr_next, (value + mcu->mcu_ud.ud_rtr_l_margin));

	/* t_rtr_cs */
	/* As per DFI Spec: If single rank training, use optimal value */
	if (mcu->mcu_ud.ud_singlerank_train_mode) {
		odt_value = FIELD_DMC520_T_ODT_OFF_RD_NEXT_RD(dmc->odt_timing_next) -
			    FIELD_DMC520_T_ODT_ON_RD_NEXT_RD(dmc->odt_timing_next);
		jedec_value = cdiv( compute_tccd_l_timing(spd), t_ck_ps);

		value = odt_value > jedec_value ? odt_value : jedec_value;
	} else {
		value = (burst_length / 2) * 2 + read_latency + read_preamble -
			FIELD_DMC520_T_PHYRDCSLAT_NEXT_RD(dmc->t_rddata_en_next);
	}
	dmc->t_rtr_next = FIELD_DMC520_T_RTR_CS_NEXT_SET(dmc->t_rtr_next, (value + mcu->mcu_ud.ud_rtr_cs_margin));

	/* t_rtr_skip */
	dmc->t_rtr_next = FIELD_DMC520_T_RTR_SKIP_NEXT_SET(dmc->t_rtr_next, read_preamble - 1); /* Set to read_preamble - 1 */

	/* RL + BL/2 - WL + rd_postamble + wr_preamble */
	jedec_value = read_latency + burst_length / 2 + read_postamble + write_preamble - write_latency;
	odt_value = FIELD_DMC520_T_ODT_OFF_RD_NEXT_RD(dmc->odt_timing_next) - FIELD_DMC520_T_ODT_ON_WR_NEXT_RD(dmc->odt_timing_next);

	if (jedec_value > odt_value)
		value = jedec_value;
	else
		value = odt_value;
	/* t_rtw_s */
	dmc->t_rtw_next = FIELD_DMC520_T_RTW_S_NEXT_SET(dmc->t_rtw_next, (value + mcu->mcu_ud.ud_rtw_s_margin));

	/* t_rtw_l */
	dmc->t_rtw_next = FIELD_DMC520_T_RTW_L_NEXT_SET(dmc->t_rtw_next, (value + mcu->mcu_ud.ud_rtw_l_margin));

	/* t_rtw_cs */
	/* RL + BL/2 + write_preamble + turnaorund_time - wrcslat */
        value = (burst_length / 2) * 2 + read_latency + write_preamble -
		FIELD_DMC520_T_PHYWRCSLAT_NEXT_RD(dmc->t_phywrlat_next);
	dmc->t_rtw_next = FIELD_DMC520_T_RTW_CS_NEXT_SET(dmc->t_rtw_next, (value + mcu->mcu_ud.ud_rtw_cs_margin));

	/* t_wr */
	value = cdiv(15000, t_ck_ps);
	if (write_preamble == 2)
		value = value + 2;

	dmc->t_wr_next = FIELD_DMC520_T_WR_NEXT_SET(dmc->t_wr_next, value + write_latency + burst_length / 2);

	/* t_rtp */
	value = cdiv(value, 2);
	dmc->t_rtp_next = FIELD_DMC520_T_RTP_NEXT_SET(dmc->t_rtp_next, value);

	/* t_wtr_s */
	/* DDR4 Spec, Page 189, Table 101 */
	value = cdiv(2500, t_ck_ps);
	if (value < 2)
		value = 2;
	value  = value + (write_preamble - 1) + write_latency + burst_length / 2;
	dmc->t_wtr_next = FIELD_DMC520_T_WTR_S_NEXT_SET(dmc->t_wtr_next, (value + mcu->mcu_ud.ud_wtr_s_margin));

	/* t_wtr_l */
	/* DDR3 Spec, Page 171, Table 68, same for DDR4 */
	value = cdiv(7500, t_ck_ps);
	if (value < 4)
		value = 4;
	value  = value + (write_preamble - 1) + write_latency + burst_length / 2;
	dmc->t_wtr_next = FIELD_DMC520_T_WTR_L_NEXT_SET(dmc->t_wtr_next, (value + mcu->mcu_ud.ud_wtr_l_margin));

	/* t_wtr_cs */
	value = (write_preamble - 1) + burst_length / 2 + crc_enable + write_postamble + read_preamble;
	dmc->t_wtr_next = FIELD_DMC520_T_WTR_CS_NEXT_SET(dmc->t_wtr_next, (value + mcu->mcu_ud.ud_wtr_cs_margin));

	/* t_wtw_s */
	/* Set to burst_length/2 + crc_enable */
	dmc->t_wtw_next = FIELD_DMC520_T_WTW_S_NEXT_SET(dmc->t_wtw_next,
			  (burst_length / 2 + crc_enable + mcu->mcu_ud.ud_wtw_s_margin));

	/* t_wtw_l */
	value = cdiv( compute_tccd_l_timing(spd), t_ck_ps) + crc_enable;
	dmc->t_wtw_next = FIELD_DMC520_T_WTW_L_NEXT_SET(dmc->t_wtw_next, (value + mcu->mcu_ud.ud_wtw_l_margin)); /* Set to 4 */

	/* t_wtw_cs */
	/* As per DFI Spec: If single rank training use optimal value */
	if (mcu->mcu_ud.ud_singlerank_train_mode) {
		odt_value = FIELD_DMC520_T_ODT_OFF_WR_NEXT_RD(dmc->odt_timing_next) -
			    FIELD_DMC520_T_ODT_ON_WR_NEXT_RD(dmc->odt_timing_next);
		jedec_value = cdiv( compute_tccd_l_timing(spd), t_ck_ps) + crc_enable;

		value = odt_value > jedec_value ? odt_value : jedec_value;
	} else {
		value = crc_enable + FIELD_DMC520_T_PHYWRDATA_NEXT_RD(dmc->t_phywrlat_next) +
			FIELD_DMC520_T_PHYWRLAT_NEXT_RD(dmc->t_phywrlat_next) -
			FIELD_DMC520_T_PHYWRCSLAT_NEXT_RD(dmc->t_phywrlat_next) + write_preamble;
	}
	dmc->t_wtw_next = FIELD_DMC520_T_WTW_CS_NEXT_SET(dmc->t_wtw_next, (value + mcu->mcu_ud.ud_wtw_cs_margin));

	/* t_wtw_skip */
	/* Set to write_preamble - 1 */
	dmc->t_wtw_next = FIELD_DMC520_T_WTW_SKIP_NEXT_SET(dmc->t_wtw_next, write_preamble - 1);

	/* t_ep */
	/* t_ep is equal to t_cke(min) */
	value = cdiv(5000, t_ck_ps);
	if (value < 3)
		value = 3;
	dmc->t_ep_next = value;

	/* t_xp */
	value = cdiv(6000, t_ck_ps);
#ifdef DDR3_MODE
	if (value < 3)
		value = 3;
#else
	if (value < 4)
		value = 4;
#endif
	dmc->t_xp_next = FIELD_DMC520_T_XP_NEXT_SET(dmc->t_xp_next, value);

	/* t_xpdll */
	value = cdiv(24000, t_ck_ps);
	if (value < 10)
		value = 10;
	dmc->t_xp_next = FIELD_DMC520_T_XPDLL_NEXT_SET(dmc->t_xp_next, value);

	/* t_esr */
	/* t_esr is equal to t_cke(min) + 1nCK */
	value = cdiv(5000, t_ck_ps);
	if (value < 3)
		value = 3;
	dmc->t_esr_next = value + mcu->ddr_info.parity_latency;

	/* t_xsr */
	/* tRFC + 10ns */
	value  = cdiv (t_rfc + 10000, t_ck_ps);
	dmc->t_xsr_next = FIELD_DMC520_T_XSR_NEXT_SET(dmc->t_xsr_next, value);

	/* t_xsrdll */
	/* 597 <= 1866 Mbps, 768 <= 2400 Mbps, 1024 for rest */
	if (t_ck_ps >= 1071)
		value = 597;
	else if (t_ck_ps >= 833)
		value = 768;
	else
		value = 1024;
	dmc->t_xsr_next = FIELD_DMC520_T_XSRDLL_NEXT_SET(dmc->t_xsr_next, value);

	/* t_xmpd */
	/* tXS + tXSDLL */
	value = FIELD_DMC520_T_XSR_NEXT_RD(dmc->t_xsr_next) +
		FIELD_DMC520_T_XSRDLL_NEXT_RD(dmc->t_xsr_next);
	dmc->t_xmpd_next = value;

	/* t_esrck */
	value = cdiv(10000, t_ck_ps);
	if (value < 5)
		value = 5;
	/* CA Parity is enabled */
	dmc->t_esrck_next = value + mcu->ddr_info.parity_latency;

	/* t_ckxsr i.e tCKSRX*/
	value = cdiv(10000, t_ck_ps);
	if (value < 5)
		value = 5;
	dmc->t_ckxsr_next = value;

	/* t_zqcs */
#ifdef DDR3_MODE
	value = cdiv( 80000, t_ck_ps);
	if (value < 64)
		value = 64;
	dmc->t_zqcs_next = 64;
#else
	dmc->t_zqcs_next = 128;
#endif

}

/* Configure performance parameters */
void config_dmc_perf_parameters(struct dmc_param *dmc)
{
	/* Turnaround Control */
	dmc->turnaround_control_next = 0;

	/* Hit Turnaround Control */
	dmc->hit_turnaround_control_next = 0;

	/* QoS class Control */
	dmc->qos_class_control_next = 0;

	/* Escalation Control */
	/* Set escalation count to one prescalar period */
	dmc->escalation_control_next = FIELD_DMC520_ESCALATION_COUNT_NEXT_SET(dmc->escalation_control_next, 1);

	/* Set escalation prescalar to 16 DMC clock cyles */
	dmc->escalation_control_next = FIELD_DMC520_ESCALATION_PRESCALAR_NEXT_SET(dmc->escalation_control_next, 0);

	/* Set read backlog suppression */
	dmc->escalation_control_next = FIELD_DMC520_READ_BACKLOG_SUPPRESSION_NEXT_SET(dmc->escalation_control_next, 0xF);

	/* qv_control */
	dmc->qv_control_31_00_next = 0;
	dmc->qv_control_63_32_next = 0;

	/*
	 * CSW issues requests with priority 0 and 1.
	 * DMC has a 4-bit priroity field where 15 is the highest priority
	 * Remap priority using qv_control register
	 * Escalation count is enabled, set qos to 'hD instead 'hF (which is the highest priority)
	 */
	dmc->qv_control_31_00_next = FIELD_DMC520_QOS0_PRIORITY_NEXT_SET(dmc->qv_control_31_00_next, 13);
	dmc->qv_control_31_00_next = FIELD_DMC520_QOS1_PRIORITY_NEXT_SET(dmc->qv_control_31_00_next, 13);

	/* rt_control */
	dmc->rt_control_31_00_next = 0;
	dmc->rt_control_63_32_next = 0;

	/* timeout_control */
	dmc->timeout_control_next = FIELD_DMC520_TIMEOUT_PRESCALAR_NEXT_SET(dmc->timeout_control_next, 0);

	/* credit_control */
	dmc->credit_control_next = 0;

	/* Write Priority Control */
	dmc->write_priority_control_31_00_next = 0;
	dmc->write_priority_control_63_32_next = 0;

	/* Queue Threshold, flush requests as soon as possible */
	dmc->queue_threshold_control_31_00_next = 0;
	dmc->queue_threshold_control_63_32_next = 0;
}

/* Configure DFI parameters */
void config_dmc_dfi_parameters(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int value;
	/* tck in ps */
	unsigned int t_ck_ps	    = mcu->ddr_info.t_ck_ps;

	/* t_cmd_lat, t_cal */
	dmc->t_cmd_next = 0U;

	/* t_parin_lat */
	/* t_completion JEDEC DDR4 Spec Table 41 tCRC_ALERT */
	value = cdiv(13000, t_ck_ps) + 30;
	dmc->t_parity_next = FIELD_DMC520_T_COMPLETION_NEXT_SET(dmc->t_parity_next, value);

	/* t_phyrdlat Set it to max */
	dmc->t_phyrdlat_next = 64;

	/* t_phywrlat */
	/* Extra cycle added on the dfi bus for back-end timing closure */
	value = mcu->ddr_info.write_latency - 2 - 1;
	dmc->t_phywrlat_next = FIELD_DMC520_T_PHYWRLAT_NEXT_SET(dmc->t_phywrlat_next, value);

	/* t_phywrcslat */
	value  = (value > 7) ? (value - 7) : 0;
	dmc->t_phywrlat_next = FIELD_DMC520_T_PHYWRCSLAT_NEXT_SET(dmc->t_phywrlat_next, value);

	/* t_phywrlat_diff */
	dmc->t_phywrlat_next = FIELD_DMC520_T_PHYWRLAT_DIFF_NEXT_SET(dmc->t_phywrlat_next, 0U);

	/* t_phywrdata */
	dmc->t_phywrlat_next = FIELD_DMC520_T_PHYWRDATA_NEXT_SET(dmc->t_phywrlat_next, 1U);

	/* t_rddata_en */
	/* Extra cycle added on the DFI bus for back-end timing closure */
	value = mcu->ddr_info.read_latency - cdiv(IO_INPUT_ENABLE_DELAY * 10, t_ck_ps) - 1;
#if XGENE_EMU
	/* Emulator PHY model requires additional cycle on the read path */
	value = value + 1;
#endif
	dmc->t_rddata_en_next = FIELD_DMC520_T_RDDATA_EN_NEXT_SET(dmc->t_rddata_en_next, value);

	/* t_rddata_en_diff */
	dmc->t_rddata_en_next = FIELD_DMC520_T_RDDATA_EN_DIFF_NEXT_SET(dmc->t_rddata_en_next, 0U);

	/* t_phyrdcslat */
	value = (value > 5) ? (value - 5) : 0;
	dmc->t_rddata_en_next = FIELD_DMC520_T_PHYRDCSLAT_NEXT_SET(dmc->t_rddata_en_next, value);
}

/* Configure DMC Low Power parameters */
void config_dmc_low_power_parameters(struct dmc_param *dmc)
{
	/* No low power features enabled. */
	dmc->low_power_control_next = 0U;

}

/* Configure DMC ODT timing parameters */
void config_dmc_odt_timing_paramaters(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int odt_on	    = 0;
	unsigned int odt_off	    = 0;
	unsigned int burst_length   = 8; /* Bust length 8 is only supported */
	unsigned int crc_enable     = mcu->mcu_ud.ud_crc_enable;
	unsigned int write_latency  = mcu->ddr_info.write_latency;
	unsigned int write_preamble = mcu->ddr_info.write_preamble;
	unsigned int read_latency   = mcu->ddr_info.read_latency;
	unsigned int read_preamble  = mcu->ddr_info.read_preamble;

#ifdef DDR3_MODE
	odt_off = write_latency + burst_length / 2 - (write_latency - 2);
#else
	/*
	 * Refer JEDEC 4A, 5.3.2
	 * When operating in 2tCK Preamble Mode, The ODT latency must be 1 clock smaller
	 * than in 1tCK Preamble Mode; DODTLon =WL -
	 * 3; DODTLoff = WL - 3."(WL = CWL+AL+PL)
	 */
	odt_off = write_latency + crc_enable + burst_length / 2 +
		  (write_preamble - 1) - (write_latency - (write_preamble + 1));
#endif

	/* Config ODT Timing for Write */
	dmc->odt_timing_next = FIELD_DMC520_T_ODT_ON_WR_NEXT_SET(dmc->odt_timing_next, odt_on);
	dmc->odt_timing_next = FIELD_DMC520_T_ODT_OFF_WR_NEXT_SET(dmc->odt_timing_next, (odt_off + mcu->mcu_ud.ud_wrodt_off_margin));

#ifdef DDR3_MODE
	odt_on	= (read_latency == write_latency) ? 0 :
		  read_latency - (read_preamble + 1) - (write_latency - 2);
	odt_off = odt_on + 6;
#else
	/*
	 * Refer JEDEC 4A, 5.3.2
	 * - Data Termination Disable: DRAM driving data upon receiving READ
	 * command disables the termination after RL-X and stays off for
	 * a duration of BL/2 + X clock cycles.
	 * X is 2 for 1tCK and 3 for 2tCK preamble mode
	 */
	odt_on	= (read_latency == write_latency) ? 0 :
		  read_latency - (read_preamble + 1) - (write_latency - (write_preamble + 1));
	odt_off = odt_on + (read_preamble + 1) + burst_length / 2 + crc_enable;
#endif

	/* Config ODT Timing for Read */
	dmc->odt_timing_next = FIELD_DMC520_T_ODT_ON_RD_NEXT_SET(dmc->odt_timing_next, odt_on);
	dmc->odt_timing_next = FIELD_DMC520_T_ODT_OFF_RD_NEXT_SET(dmc->odt_timing_next, (odt_off + mcu->mcu_ud.ud_rdodt_off_margin));
}

/* Configure DMC Feature bits */
void config_dmc_feature_parameters(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int value, bank_group;
	unsigned int cs_mux, cke_mux, sparse_3ds_cs, cid_mask;

	/* Memory Type */
	if (mcu->ddr_info.ddr_type == SPD_MEMTYPE_DDR3) {
		value = 0x1U;
		bank_group = 0x0U;
	}
	if (mcu->ddr_info.ddr_type == SPD_MEMTYPE_DDR4) {
		value = 0x2U;
		/* X4/X8 devices supported */
		bank_group = 0x3U;
	}


	/* CKE, CS MUX and Sparse 3DS */
	/* DMC Spec. Table 2-9 */
	switch (mcu->ddr_info.stack_high) {
	case 2:
		cs_mux	= 1;
		cke_mux = 1;
		sparse_3ds_cs = 1;
		cid_mask = 1;
		break;
	case 4:
		cs_mux	= 3;
		cke_mux = 2;
		sparse_3ds_cs = mcu->ddr_info.two_dpc_enable;
		cid_mask = 3;
		break;
	case 8:
		cs_mux	= 2;
		cke_mux = 3;
		sparse_3ds_cs = 0;
		cid_mask = 7;
		break;
	default:
		cs_mux	= 0;
		cke_mux = 1;
		sparse_3ds_cs = 1;
		cid_mask = 0;
		break;
	}

	dmc->memory_type_next = FIELD_DMC520_MEMORY_TYPE_NEXT_SET(dmc->memory_type_next, value);

	/* Memory Device Width */
	dmc->memory_type_next = FIELD_DMC520_MEMORY_DEVICE_WIDTH_NEXT_SET(dmc->memory_type_next, mcu->ddr_info.device_type);

	/* Memory Bank Group bits */
	dmc->memory_type_next = FIELD_DMC520_MEMORY_BANK_GROUPS_NEXT_SET(dmc->memory_type_next, bank_group);

	/* CID MASK */
	dmc->mux_control_next = FIELD_DMC520_CID_MASK_NEXT_SET(dmc->mux_control_next, cid_mask);

	/* RST MUX */
	dmc->mux_control_next = FIELD_DMC520_RST_MUX_CONTROL_NEXT_SET(dmc->mux_control_next, 0U);

	/* CK MUX */
	dmc->mux_control_next = FIELD_DMC520_CK_MUX_CONTROL_NEXT_SET(dmc->mux_control_next, 0U);

	/* CKE MUX */
	dmc->mux_control_next = FIELD_DMC520_CKE_MUX_CONTROL_NEXT_SET(dmc->mux_control_next, cke_mux);

	/* CS MUX */
	dmc->mux_control_next = FIELD_DMC520_CS_MUX_CONTROL_NEXT_SET(dmc->mux_control_next, cs_mux);

	/* ECC Enable */
	dmc->feature_config = FIELD_DMC520_ECC_ENABLE_SET(dmc->feature_config, mcu->mcu_ud.ud_ecc_enable);

	/* Correctable Writeback */
	dmc->feature_config = FIELD_DMC520_CORRECTED_WRITEBACK_SET(dmc->feature_config, 0U);

	/* Uncorrectable Retry */
	dmc->feature_config = FIELD_DMC520_UNCORRECTED_RETRY_SET(dmc->feature_config, 0U);

	/* read-modify write Enable */
	dmc->feature_config = FIELD_DMC520_RMW_ENABLE_SET(dmc->feature_config, 1U);

	/* Clk Gate En */
	dmc->feature_config = FIELD_DMC520_CLK_GATE_EN_SET(dmc->feature_config, 1U);

	/* PMU Enable */
	dmc->feature_config = FIELD_DMC520_PMU_ENABLE_SET(dmc->feature_config, mcu->mcu_ud.ud_pmu_enable);

	/* Parity Mask */
	dmc->feature_config = FIELD_DMC520_PARITY_MASK_SET(dmc->feature_config, 0U);

	/* Si Clk gate Disable */
	dmc->feature_config = FIELD_DMC520_SI_CLK_GATE_DISABLE_SET(dmc->feature_config, 0U);

	/* DCB Clk gate Disable */
	dmc->feature_config = FIELD_DMC520_DCB_CLK_GATE_DISABLE_SET(dmc->feature_config, 0U);

	/* MI Clk gate Disable */
	dmc->feature_config = FIELD_DMC520_MI_CLK_GATE_DISABLE_SET(dmc->feature_config, 0U);

	/* Read-modify write Mode */
	dmc->feature_config = FIELD_DMC520_RMW_MODE_SET(dmc->feature_config, 0U);

	/* map cid to cs */
	dmc->feature_config = FIELD_DMC520_MAP_CID_TO_CS_SET(dmc->feature_config, (mcu->ddr_info.stack_high > 1));

	/* sparse 3ds cs */
	dmc->feature_config = FIELD_DMC520_SPARSE_3DS_CS_SET(dmc->feature_config, sparse_3ds_cs);

	/* Write DBI Enable */
	dmc->feature_control_next = FIELD_DMC520_WRITE_DBI_ENABLE_NEXT_SET(dmc->feature_control_next, mcu->mcu_ud.ud_wr_dbi_enable);

	/* CRC Enable */
	dmc->feature_control_next = FIELD_DMC520_CRC_ENABLE_NEXT_SET(dmc->feature_control_next, mcu->mcu_ud.ud_crc_enable);

	/* Read DBI Enable */
	dmc->feature_control_next = FIELD_DMC520_READ_DBI_ENABLE_NEXT_SET(dmc->feature_control_next, mcu->mcu_ud.ud_rd_dbi_enable);

	/* Two T timing */
	dmc->feature_control_next = FIELD_DMC520_TWO_T_TIMING_NEXT_SET(dmc->feature_control_next, mcu->mcu_ud.ud_en2tmode);

	/* ZQCS after n Refresh */
	dmc->feature_control_next = FIELD_DMC520_ZQCS_AFTER_N_REF_NEXT_SET(dmc->feature_control_next, 0U);

	/* ZQCS after X Self Refresh */
	dmc->feature_control_next = FIELD_DMC520_ZQCS_AFTER_XSREF_NEXT_SET(dmc->feature_control_next, 0U);

	/* Temperature Polling after n Refresh */
	dmc->feature_control_next = FIELD_DMC520_TEMP_POLL_AFTER_N_REF_NEXT_SET(dmc->feature_control_next, 0U);

	/* Temperature Polling after X Self Refresh */
	dmc->feature_control_next = FIELD_DMC520_TEMP_POLL_AFTER_XSREF_NEXT_SET(dmc->feature_control_next, 0U);

	/* TRR Eanbled, Defatured */
	dmc->feature_control_next = FIELD_DMC520_TRR_ENABLE_NEXT_SET(dmc->feature_control_next, 0U);

	/* Address mirroring */
	dmc->feature_control_next = FIELD_DMC520_ADDRESS_MIRRORING_NEXT_SET(dmc->feature_control_next, mcu->ddr_info.addr_mirror);

	/* MRS inversion */
	/* Non-UDIMM i.e RDIMM, LRDIMM have MRS inversion in DDR4 */
	value = ((mcu->ddr_info.ddr_type == SPD_MEMTYPE_DDR4) &
		 (mcu->ddr_info.package_type != UDIMM));
	dmc->feature_control_next = FIELD_DMC520_MRS_OUTPUT_INVERSION_NEXT_SET(dmc->feature_control_next, value);

	/* Lvl Wakeup En */
	dmc->feature_control_next = FIELD_DMC520_LVL_WAKEUP_EN_NEXT_SET(dmc->feature_control_next, 1U);

	/* DFI Err mode */
	dmc->feature_control_next = FIELD_DMC520_DFI_ERR_MODE_NEXT_SET(dmc->feature_control_next, 1U);

	/* Address mirroring mask */
	/* Enable Ranks that require mirroring */
	dmc->feature_control_next = FIELD_DMC520_ADDRESS_MIRRORING_MASK_NEXT_SET(dmc->feature_control_next, mcu->ddr_info.odd_ranks);

	/* Alert mode */
	dmc->feature_control_next = FIELD_DMC520_ALERT_MODE_NEXT_SET(dmc->feature_control_next, 0U);
}

/* Configure DMC Addressing bits */
void config_dmc_addressing_parameters(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int value;

	/* Always use slot 0 */
	generic_spd_eeprom_t *spd = &mcu->spd_info[0].spd_info;
	unsigned int bank_hash_enable = mcu->mcu_ud.ud_bank_hash_enable;
	unsigned int two_dpc_enable   = mcu->ddr_info.two_dpc_enable;

	/* Column bits - 0b10 - 10 bits, 0b11 - 11 bits, 0b100 - 12 bits */
	value  = compute_column_bits(spd) - 8U;
	dmc->address_control_next = FIELD_DMC520_COLUMN_BITS_NEXT_SET(dmc->address_control_next, value);

	/* Row bits - 0b1 - 12 bits, .... 0b111 - 18 bits */
	value  = compute_row_bits(spd) - 11U;
	dmc->address_control_next = FIELD_DMC520_ROW_BITS_NEXT_SET(dmc->address_control_next, value);

	/* Bank bits - 0b11 - 8 banks, 0b100 - 16 banks */
	value  = compute_bank_bits(spd);
	dmc->address_control_next = FIELD_DMC520_BANK_BITS_NEXT_SET(dmc->address_control_next, value);

	/* Rank bits = 0b0 - 1 logical ranks ... 0b11 - 8 logical ranks */
	value = compute_no_ranks(spd) * (two_dpc_enable + 1);
	value = ilog2(value);
	dmc->address_control_next = FIELD_DMC520_RANK_BITS_NEXT_SET(dmc->address_control_next, value);

	/* Bank hash Enable, if enabled lower 9 bits of the row address are used */
	dmc->address_control_next = FIELD_DMC520_BANK_HASH_ENABLE_NEXT_SET(dmc->address_control_next, bank_hash_enable);

	/* Address decode control */
	dmc->decode_control_next = FIELD_DMC520_ADDRESS_DECODE_NEXT_SET(dmc->decode_control_next, mcu->mcu_ud.ud_addr_decode);

	/* Stripe Decode */
	dmc->decode_control_next = FIELD_DMC520_STRIPE_DECODE_NEXT_SET(dmc->decode_control_next, mcu->mcu_ud.ud_stripe_decode);

	/* Format control, always x128 */
	dmc->format_control = FIELD_DMC520_MEMORY_WIDTH_SET(dmc->format_control, 0x3U);

	/* Address Map mode */
	dmc->address_map_next = 0U;
	dmc->address_map_next = FIELD_DMC520_ADDR_MAP_MODE_NEXT_SET(dmc->address_map_next, mcu->ddr_info.addr_map_mode);
}

/* Config Rank Remapping */
void config_dmc_rank_remapping(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int rank0, rank1, rank2, rank3;
	unsigned int rank4, rank5, rank6, rank7;

	/* Default mapping */
	rank0 = 0;
	rank1 = 1U;
	rank2 = 2U;
	rank3 = 3U;
	rank4 = 4U;
	rank5 = 5U;
	rank6 = 6U;
	rank7 = 7U;

	/* 2DPC enable Rank remapping */
	if ((mcu->ddr_info.two_dpc_enable == 1) & (mcu->ddr_info.max_ranks == 1)) {
		rank0 = 0;
		rank1 = 4U;
		rank4 = 1U;
	} else if ((mcu->ddr_info.two_dpc_enable == 1) & (mcu->ddr_info.max_ranks == 2)) {
		rank0 = 0;
		rank1 = 1U;
		rank2 = 4U;
		rank3 = 5U;
		rank4 = 2U;
		rank5 = 3U;
	} else if ((mcu->ddr_info.two_dpc_enable == 1) & (mcu->ddr_info.max_ranks == 2)
		   & (mcu->ddr_info.stack_high == 2)) {
		rank0 = 0;
		rank1 = 1U;
		rank2 = 4U;
		rank3 = 5U;
		rank4 = 2U;
		rank5 = 3U;
		rank6 = 6U;
		rank7 = 7U;
	}
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS0_NEXT_SET(dmc->rank_remap_control_next, rank0);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS1_NEXT_SET(dmc->rank_remap_control_next, rank1);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS2_NEXT_SET(dmc->rank_remap_control_next, rank2);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS3_NEXT_SET(dmc->rank_remap_control_next, rank3);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS4_NEXT_SET(dmc->rank_remap_control_next, rank4);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS5_NEXT_SET(dmc->rank_remap_control_next, rank5);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS6_NEXT_SET(dmc->rank_remap_control_next, rank6);
	dmc->rank_remap_control_next = FIELD_DMC520_RANK_REMAP_CS7_NEXT_SET(dmc->rank_remap_control_next, rank7);
}

/* Configure PHY Rd/Wr Data CS Control */
void config_dmc_phyrdwr_data_cs(struct dmc_param *dmc, struct apm_mcu *mcu)
{

	unsigned int rank0, rank1, rank2, rank3;
	unsigned int rank4, rank5, rank6, rank7;

	/* rdwrdata_cs */
	if ((mcu->ddr_info.max_ranks == 8) & (mcu->ddr_info.stack_high == 8)) {
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfeU;
		rank3 = 0xfeU;
		rank4 = 0xfeU;
		rank5 = 0xfeU;
		rank6 = 0xfeU;
		rank7 = 0xfeU;
	} else if ((mcu->ddr_info.max_ranks == 8) & (mcu->ddr_info.stack_high == 4)) {
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfeU;
		rank3 = 0xfeU;
		rank4 = 0xfdU;
		rank5 = 0xfdU;
		rank6 = 0xfdU;
		rank7 = 0xfdU;
	} else if ((mcu->ddr_info.max_ranks == 8) & (mcu->ddr_info.stack_high == 2)) {
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfdU;
		rank3 = 0xfdU;
		rank4 = 0xfbU;
		rank5 = 0xfbU;
		rank6 = 0xf7U;
		rank7 = 0xf7U;
	} else if ((mcu->ddr_info.max_ranks == 4) & (mcu->ddr_info.stack_high == 4)) { /* 2DPC Enabled */
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfeU;
		rank3 = 0xfeU;
		rank4 = 0xefU;
		rank5 = 0xefU;
		rank6 = 0xefU;
		rank7 = 0xefU;
	} else if ((mcu->ddr_info.max_ranks == 4) & (mcu->ddr_info.stack_high == 2)) { /* 2DPC Enabled */
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfdU;
		rank3 = 0xfdU;
		rank4 = 0xefU;
		rank5 = 0xefU;
		rank6 = 0xdfU;
		rank7 = 0xdfU;
	} else if ((mcu->ddr_info.max_ranks == 2) & (mcu->ddr_info.stack_high == 2)) { /* 2DPC Enabled */
		rank0 = 0xfeU;
		rank1 = 0xfeU;
		rank2 = 0xfeU;
		rank3 = 0xfeU;
		rank4 = 0xefU;
		rank5 = 0xefU;
		rank6 = 0xefU;
		rank7 = 0xefU;
	} else {
		rank0 = 0xfeU;
		rank1 = 0xfdU;
		rank2 = 0xfbU;
		rank3 = 0xf7U;
		rank4 = 0xefU;
		rank5 = 0xdfU;
		rank6 = 0xbfU;
		rank7 = 0x7fU;
	}
	dmc->phy_rdwrdata_cs_mask_31_00 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS0_SET(dmc->phy_rdwrdata_cs_mask_31_00, rank0);
	dmc->phy_rdwrdata_cs_mask_31_00 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS1_SET(dmc->phy_rdwrdata_cs_mask_31_00, rank1);
	dmc->phy_rdwrdata_cs_mask_31_00 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS2_SET(dmc->phy_rdwrdata_cs_mask_31_00, rank2);
	dmc->phy_rdwrdata_cs_mask_31_00 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS3_SET(dmc->phy_rdwrdata_cs_mask_31_00, rank3);
	dmc->phy_rdwrdata_cs_mask_63_32 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS4_SET(dmc->phy_rdwrdata_cs_mask_63_32, rank4);
	dmc->phy_rdwrdata_cs_mask_63_32 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS5_SET(dmc->phy_rdwrdata_cs_mask_63_32, rank5);
	dmc->phy_rdwrdata_cs_mask_63_32 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS6_SET(dmc->phy_rdwrdata_cs_mask_63_32, rank6);
	dmc->phy_rdwrdata_cs_mask_63_32 = FIELD_DMC520_PHY_RDWRDATA_CS_MASK_CS7_SET(dmc->phy_rdwrdata_cs_mask_63_32, rank7);
}

/* Configure ODT Write Control */
void config_dmc_odt_wr_control(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int rank0, rank1, rank2, rank3;
	unsigned int rank4, rank5, rank6, rank7;

	/* For software, ODT0 and ODT4 can be used. i.e ODT0 == ODT1, ODT4 == ODT5 */
	/* 2DPC enable Rank remapping */
	if (mcu->ddr_info.stack_high == 8) {
		rank0 = 0x1U;
		rank1 = 0x1U;
		rank2 = 0x1U;
		rank3 = 0x1U;
		rank4 = 0x1U;
		rank5 = 0x1U;
		rank6 = 0x1U;
		rank7 = 0x1U;
	} else if (mcu->ddr_info.stack_high == 4) {
		rank0 = 0x1U;
		rank1 = 0x1U;
		rank2 = 0x1U;
		rank3 = 0x1U;
		if (mcu->ddr_info.two_dpc_enable) {
			rank4 = 0x10U;
			rank5 = 0x10U;
			rank6 = 0x10U;
			rank7 = 0x10U;
		} else {
			rank4 = 0x2U;
			rank5 = 0x2U;
			rank6 = 0x2U;
			rank7 = 0x2U;
		}
	} else if (mcu->ddr_info.stack_high == 2) {
		rank0 = 0x1U;
		rank1 = 0x1U;
		rank2 = 0x2U;
		rank3 = 0x2U;
		rank4 = 0x10U;
		rank5 = 0x10U;
		rank6 = 0x20U;
		rank7 = 0x20U;
	} else {
		rank0 = 0x1U;
		rank1 = 0x2U;
		rank2 = 0x1U;
		rank3 = 0x2U;
		rank4 = 0x10U;
		rank5 = 0x20U;
		rank6 = 0x10U;
		rank7 = 0x20U;
	}

	dmc->odt_wr_control_31_00_next = FIELD_DMC520_ODT_MASK_WR_CS0_NEXT_SET(dmc->odt_wr_control_31_00_next, rank0);
	dmc->odt_wr_control_31_00_next = FIELD_DMC520_ODT_MASK_WR_CS1_NEXT_SET(dmc->odt_wr_control_31_00_next, rank1);
	dmc->odt_wr_control_31_00_next = FIELD_DMC520_ODT_MASK_WR_CS2_NEXT_SET(dmc->odt_wr_control_31_00_next, rank2);
	dmc->odt_wr_control_31_00_next = FIELD_DMC520_ODT_MASK_WR_CS3_NEXT_SET(dmc->odt_wr_control_31_00_next, rank3);
	dmc->odt_wr_control_63_32_next = FIELD_DMC520_ODT_MASK_WR_CS4_NEXT_SET(dmc->odt_wr_control_63_32_next, rank4);
	dmc->odt_wr_control_63_32_next = FIELD_DMC520_ODT_MASK_WR_CS5_NEXT_SET(dmc->odt_wr_control_63_32_next, rank5);
	dmc->odt_wr_control_63_32_next = FIELD_DMC520_ODT_MASK_WR_CS6_NEXT_SET(dmc->odt_wr_control_63_32_next, rank6);
	dmc->odt_wr_control_63_32_next = FIELD_DMC520_ODT_MASK_WR_CS7_NEXT_SET(dmc->odt_wr_control_63_32_next, rank7);
}

/* Configure ODT Read Control */
void config_dmc_odt_rd_control(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	unsigned int rank0, rank1, rank2, rank3;
	unsigned int rank4, rank5, rank6, rank7;

	/* For software, ODT0 and ODT4 can be used. i.e ODT0 == ODT1, ODT4 == ODT5 */
	/* 2DPC enable Rank remapping */
	if (mcu->ddr_info.stack_high == 8) {
		rank0 = 0x0U;
		rank1 = 0x0U;
		rank2 = 0x0U;
		rank3 = 0x0U;
		rank4 = 0x0U;
		rank5 = 0x0U;
		rank6 = 0x0U;
		rank7 = 0x0U;
	} else if (mcu->ddr_info.stack_high == 4) {
		rank0 = 0x32U;
		rank1 = 0x32U;
		rank2 = 0x32U;
		rank3 = 0x32U;
		if (mcu->ddr_info.two_dpc_enable) {
			rank4 = 0x23U;
			rank5 = 0x23U;
			rank6 = 0x23U;
			rank7 = 0x23U;
		} else {
			rank4 = 0x31U;
			rank5 = 0x31U;
			rank6 = 0x31U;
			rank7 = 0x31U;
		}
	} else if (mcu->ddr_info.stack_high == 2) {
		rank0 = 0x32U;
		rank1 = 0x32U;
		rank2 = 0x31U;
		rank3 = 0x31U;
		rank4 = 0x23U;
		rank5 = 0x23U;
		rank6 = 0x13U;
		rank7 = 0x13U;
	} else {
		rank0 = 0x32U;
		rank1 = 0x31U;
		rank2 = 0x32U;
		rank3 = 0x31U;
		rank4 = 0x23U;
		rank5 = 0x13U;
		rank6 = 0x23U;
		rank7 = 0x13U;
	}

	dmc->odt_rd_control_31_00_next = FIELD_DMC520_ODT_MASK_RD_CS0_NEXT_SET(dmc->odt_rd_control_31_00_next, rank0);
	dmc->odt_rd_control_31_00_next = FIELD_DMC520_ODT_MASK_RD_CS1_NEXT_SET(dmc->odt_rd_control_31_00_next, rank1);
	dmc->odt_rd_control_31_00_next = FIELD_DMC520_ODT_MASK_RD_CS2_NEXT_SET(dmc->odt_rd_control_31_00_next, rank2);
	dmc->odt_rd_control_31_00_next = FIELD_DMC520_ODT_MASK_RD_CS3_NEXT_SET(dmc->odt_rd_control_31_00_next, rank3);
	dmc->odt_rd_control_63_32_next = FIELD_DMC520_ODT_MASK_RD_CS4_NEXT_SET(dmc->odt_rd_control_63_32_next, rank4);
	dmc->odt_rd_control_63_32_next = FIELD_DMC520_ODT_MASK_RD_CS5_NEXT_SET(dmc->odt_rd_control_63_32_next, rank5);
	dmc->odt_rd_control_63_32_next = FIELD_DMC520_ODT_MASK_RD_CS6_NEXT_SET(dmc->odt_rd_control_63_32_next, rank6);
	dmc->odt_rd_control_63_32_next = FIELD_DMC520_ODT_MASK_RD_CS7_NEXT_SET(dmc->odt_rd_control_63_32_next, rank7);
}

/* Config DMC User Config regsiter Space */
void config_dmc_user_config(struct apm_mcu *mcu)
{
	unsigned int user_config2 = 0;

	user_config2 = FIELD_DMC520_MODULE_TYPE_SET(user_config2, mcu->ddr_info.package_type);
	user_config2 = FIELD_DMC520_LOGICAL_RANK_SET(user_config2, mcu->ddr_info.active_ranks);
	user_config2 = FIELD_DMC520_PHYSICAL_RANK_SET(user_config2, mcu->ddr_info.physical_ranks);

	dmc_write_reg(mcu->id, USER_CONFIG2_ADDR, user_config2);
}

/* Config DMC memory address space */
void config_dmc_memory_address(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	/* Default memory region has both secure/non-secure access */
	dmc->memory_address_max_31_00_next = FIELD_DMC520_REGION_NS_READ_EN_NEXT_SET(dmc->memory_address_max_31_00_next, 1U);
	dmc->memory_address_max_31_00_next = FIELD_DMC520_REGION_NS_WRITE_EN_NEXT_SET(dmc->memory_address_max_31_00_next, 1U);
	dmc->memory_address_max_31_00_next = FIELD_DMC520_REGION_S_READ_EN_NEXT_SET(dmc->memory_address_max_31_00_next, 1U);
	dmc->memory_address_max_31_00_next = FIELD_DMC520_REGION_S_WRITE_EN_NEXT_SET(dmc->memory_address_max_31_00_next, 1U);

	/* Max memory address set to system maximum */
	/* DRAM-C is the max memory usage 0x80_0000_0000 */
	dmc->memory_address_max_43_32_next = FIELD_DMC520_MEMORY_ADDRESS_MAX_43_32_NEXT_SET(dmc->memory_address_max_43_32_next, 0x80U);
}

/* Config DMC DQ mapping */
void config_dmc_dq_mapping(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	generic_spd_eeprom_t *spd = &mcu->spd_info[0].spd_info;

	/* Config DQ Mapping based of SPD Data */
	dmc->dq_map_control_15_00_next =  FIELD_DMC520_DQ_MAP_3_0_NEXT_SET(dmc->dq_map_control_15_00_next, spd->cntr_bit_map[0]);
	dmc->dq_map_control_15_00_next =  FIELD_DMC520_DQ_MAP_7_4_NEXT_SET(dmc->dq_map_control_15_00_next, spd->cntr_bit_map[1]);
	dmc->dq_map_control_15_00_next =  FIELD_DMC520_DQ_MAP_11_8_NEXT_SET(dmc->dq_map_control_15_00_next, spd->cntr_bit_map[2]);
	dmc->dq_map_control_15_00_next =  FIELD_DMC520_DQ_MAP_15_12_NEXT_SET(dmc->dq_map_control_15_00_next, spd->cntr_bit_map[3]);

	dmc->dq_map_control_31_16_next =  FIELD_DMC520_DQ_MAP_19_16_NEXT_SET(dmc->dq_map_control_31_16_next, spd->cntr_bit_map[4]);
	dmc->dq_map_control_31_16_next =  FIELD_DMC520_DQ_MAP_23_20_NEXT_SET(dmc->dq_map_control_31_16_next, spd->cntr_bit_map[5]);
	dmc->dq_map_control_31_16_next =  FIELD_DMC520_DQ_MAP_27_24_NEXT_SET(dmc->dq_map_control_31_16_next, spd->cntr_bit_map[6]);
	dmc->dq_map_control_31_16_next =  FIELD_DMC520_DQ_MAP_31_28_NEXT_SET(dmc->dq_map_control_31_16_next, spd->cntr_bit_map[7]);

	dmc->dq_map_control_47_32_next =  FIELD_DMC520_DQ_MAP_35_32_NEXT_SET(dmc->dq_map_control_47_32_next, spd->cntr_bit_map[10]);
	dmc->dq_map_control_47_32_next =  FIELD_DMC520_DQ_MAP_39_36_NEXT_SET(dmc->dq_map_control_47_32_next, spd->cntr_bit_map[11]);
	dmc->dq_map_control_47_32_next =  FIELD_DMC520_DQ_MAP_43_40_NEXT_SET(dmc->dq_map_control_47_32_next, spd->cntr_bit_map[12]);
	dmc->dq_map_control_47_32_next =  FIELD_DMC520_DQ_MAP_47_44_NEXT_SET(dmc->dq_map_control_47_32_next, spd->cntr_bit_map[13]);

	dmc->dq_map_control_63_48_next =  FIELD_DMC520_DQ_MAP_51_48_NEXT_SET(dmc->dq_map_control_63_48_next, spd->cntr_bit_map[14]);
	dmc->dq_map_control_63_48_next =  FIELD_DMC520_DQ_MAP_55_52_NEXT_SET(dmc->dq_map_control_63_48_next, spd->cntr_bit_map[15]);
	dmc->dq_map_control_63_48_next =  FIELD_DMC520_DQ_MAP_59_56_NEXT_SET(dmc->dq_map_control_63_48_next, spd->cntr_bit_map[16]);
	dmc->dq_map_control_63_48_next =  FIELD_DMC520_DQ_MAP_63_60_NEXT_SET(dmc->dq_map_control_63_48_next, spd->cntr_bit_map[17]);

	dmc->dq_map_control_71_64_next =  FIELD_DMC520_DQ_MAP_67_64_NEXT_SET(dmc->dq_map_control_71_64_next, spd->cntr_bit_map[8]);
	dmc->dq_map_control_71_64_next =  FIELD_DMC520_DQ_MAP_71_68_NEXT_SET(dmc->dq_map_control_71_64_next, spd->cntr_bit_map[9]);

	/* Rank DQ bit swap, for RDIMM only odd rank swap */
	dmc->dq_map_control_71_64_next =  FIELD_DMC520_RANK_DQ_BIT_SWAP_NEXT_SET(dmc->dq_map_control_71_64_next, mcu->ddr_info.odd_ranks);
}

/* Configure Write Leveling related setting */
void config_write_training(struct dmc_param *dmc)
{
	/* wrlvl_mode: PHY Evaluation mode */
	dmc->wrlvl_control_next = FIELD_DMC520_WRLVL_MODE_NEXT_SET(dmc->wrlvl_control_next, 0);

	/* wrlvl_setup */
	dmc->wrlvl_control_next = FIELD_DMC520_WRLVL_SETUP_NEXT_SET(dmc->wrlvl_control_next, 0);

	/* wrlvl_refresh: Enable refresh before training */
	dmc->wrlvl_control_next = FIELD_DMC520_WRLVL_REFRESH_NEXT_SET(dmc->wrlvl_control_next, 1);

	/* refresh_dur_wrlvl: Refresh during Write leveling is disabled */
	dmc->wrlvl_control_next = FIELD_DMC520_REFRESH_DUR_WRLVL_NEXT_SET(dmc->wrlvl_control_next, 0);

	/* wrlvl_err_en: dfi_err causes replayd during training */
	dmc->wrlvl_control_next = FIELD_DMC520_WRLVL_ERR_EN_NEXT_SET(dmc->wrlvl_control_next, 1);

	/* wrlvl_pattern: Write level Pattern Only 0 supported*/
	dmc->wrlvl_control_next =  FIELD_DMC520_WRLVL_PATTERN_NEXT_SET(dmc->wrlvl_control_next, 0);

	/* wrlvl_mrs */
	dmc->wrlvl_mrs_next = 0x181;

	/* t_wrlvl_en */
	dmc->t_wrlvl_en_next = 5;

	/* t_wrlvl_ww */
	dmc->t_wrlvl_ww_next = 20;
}

/* Configure Read Gate/Leveling related setting */
void config_read_training(struct dmc_param *dmc)
{
	/* rdlvl_mode: PHY Evaluation mode */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_MODE_NEXT_SET(dmc->rdlvl_control_next, 0);

	/* rdlvl_setup */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_SETUP_NEXT_SET(dmc->rdlvl_control_next, 0);

	/* rdlvl_command ba1_0: Only pattern 0 supported by Cadence PHY */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_COMMAND_BA1_0_NEXT_SET(dmc->rdlvl_control_next, 0);

	/* rdlvl_refresh: Enable refresh before training */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_REFRESH_NEXT_SET(dmc->rdlvl_control_next, 1);

	/* refresh_dur_rdlvl: Refresh during Read leveling is disabled */
	dmc->rdlvl_control_next = FIELD_DMC520_REFRESH_DUR_RDLVL_NEXT_SET(dmc->rdlvl_control_next, 0);

	/* rdlvl_err_en: dfi_err causes replays during training */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_ERR_EN_NEXT_SET(dmc->rdlvl_control_next, 1);

	/* rdlvl_pattern: Read level Pattern Only 0 supported*/
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_PATTERN_NEXT_SET(dmc->rdlvl_control_next, 0);

	/* rdlvl_preamble: Cadence PHY DDR4 requires preamble training */
	dmc->rdlvl_control_next = FIELD_DMC520_RDLVL_PREAMBLE_NEXT_SET(dmc->rdlvl_control_next, 1);

	/* rdlvl_mrs */
	/*
 	 * Program the Mode Register command the DMC uses to place the DRAM into training mode.
	 * Set address bits [2:0] for the Mode Register write to MR3
	 * Four MPR pages are provided in DDR4 SDRAM. Page 0 is for both read and write,
	 * and pages 1,2 and 3 are read-only. Any MPR location (MPR0-3) in page 0 can be readable
	 * through any of three readout modes (serial, parallel or staggered), but pages 1, 2 and
	 * 3 support only the serial readout mode. Page 0 is training mode
	*/
	dmc->rdlvl_mrs_next = 4;

	/* t_rdlvl_en */
	dmc->t_rdlvl_en_next = 15;

	/* t_rdlvl_rr */
	dmc->t_rdlvl_rr_next = 20;
}

/* Update DMC with Configuration parameters */
void update_dmc_config(struct dmc_param *dmc, struct apm_mcu *mcu)
{
	dmc_write_reg(mcu->id, ADDRESS_CONTROL_NEXT_ADDR, dmc->address_control_next);
	dmc_write_reg(mcu->id, DECODE_CONTROL_NEXT_ADDR, dmc->decode_control_next);
	dmc_write_reg(mcu->id, FORMAT_CONTROL_ADDR, dmc->format_control);
	dmc_write_reg(mcu->id, ADDRESS_MAP_NEXT_ADDR, dmc->address_map_next);
	dmc_write_reg(mcu->id, LOW_POWER_CONTROL_NEXT_ADDR, dmc->low_power_control_next);
	dmc_write_reg(mcu->id, TURNAROUND_CONTROL_NEXT_ADDR, dmc->turnaround_control_next);
	dmc_write_reg(mcu->id, HIT_TURNAROUND_CONTROL_NEXT_ADDR, dmc->hit_turnaround_control_next);
	dmc_write_reg(mcu->id, QOS_CLASS_CONTROL_NEXT_ADDR, dmc->qos_class_control_next);
	dmc_write_reg(mcu->id, QV_CONTROL_31_00_NEXT_ADDR, dmc->qv_control_31_00_next);
	dmc_write_reg(mcu->id, ESCALATION_CONTROL_NEXT_ADDR, dmc->escalation_control_next);
	dmc_write_reg(mcu->id, QV_CONTROL_63_32_NEXT_ADDR, dmc->qv_control_63_32_next);
	dmc_write_reg(mcu->id, RT_CONTROL_31_00_NEXT_ADDR, dmc->rt_control_31_00_next);
	dmc_write_reg(mcu->id, RT_CONTROL_63_32_NEXT_ADDR, dmc->rt_control_63_32_next);
	dmc_write_reg(mcu->id, TIMEOUT_CONTROL_NEXT_ADDR, dmc->timeout_control_next);
	dmc_write_reg(mcu->id, CREDIT_CONTROL_NEXT_ADDR, dmc->credit_control_next);
	dmc_write_reg(mcu->id, WRITE_PRIORITY_CONTROL_31_00_NEXT_ADDR, dmc->write_priority_control_31_00_next);
	dmc_write_reg(mcu->id, WRITE_PRIORITY_CONTROL_63_32_NEXT_ADDR, dmc->write_priority_control_63_32_next);
	dmc_write_reg(mcu->id, QUEUE_THRESHOLD_CONTROL_31_00_NEXT_ADDR, dmc->queue_threshold_control_31_00_next);
	dmc_write_reg(mcu->id, QUEUE_THRESHOLD_CONTROL_63_32_NEXT_ADDR, dmc->queue_threshold_control_63_32_next);
	dmc_write_reg(mcu->id, MEMORY_ADDRESS_MAX_31_00_NEXT_ADDR, dmc->memory_address_max_31_00_next);
	dmc_write_reg(mcu->id, MEMORY_ADDRESS_MAX_43_32_NEXT_ADDR, dmc->memory_address_max_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN0_31_00_NEXT_ADDR, dmc->access_address_min0_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN0_43_32_NEXT_ADDR, dmc->access_address_min0_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX0_31_00_NEXT_ADDR, dmc->access_address_max0_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX0_43_32_NEXT_ADDR, dmc->access_address_max0_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN1_31_00_NEXT_ADDR, dmc->access_address_min1_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN1_43_32_NEXT_ADDR, dmc->access_address_min1_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX1_31_00_NEXT_ADDR, dmc->access_address_max1_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX1_43_32_NEXT_ADDR, dmc->access_address_max1_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN2_31_00_NEXT_ADDR, dmc->access_address_min2_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN2_43_32_NEXT_ADDR, dmc->access_address_min2_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX2_31_00_NEXT_ADDR, dmc->access_address_max2_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX2_43_32_NEXT_ADDR, dmc->access_address_max2_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN3_31_00_NEXT_ADDR, dmc->access_address_min3_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN3_43_32_NEXT_ADDR, dmc->access_address_min3_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX3_31_00_NEXT_ADDR, dmc->access_address_max3_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX3_43_32_NEXT_ADDR, dmc->access_address_max3_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN4_31_00_NEXT_ADDR, dmc->access_address_min4_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN4_43_32_NEXT_ADDR, dmc->access_address_min4_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX4_31_00_NEXT_ADDR, dmc->access_address_max4_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX4_43_32_NEXT_ADDR, dmc->access_address_max4_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN5_31_00_NEXT_ADDR, dmc->access_address_min5_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN5_43_32_NEXT_ADDR, dmc->access_address_min5_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX5_31_00_NEXT_ADDR, dmc->access_address_max5_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX5_43_32_NEXT_ADDR, dmc->access_address_max5_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN6_31_00_NEXT_ADDR, dmc->access_address_min6_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN6_43_32_NEXT_ADDR, dmc->access_address_min6_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX6_31_00_NEXT_ADDR, dmc->access_address_max6_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX6_43_32_NEXT_ADDR, dmc->access_address_max6_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN7_31_00_NEXT_ADDR, dmc->access_address_min7_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MIN7_43_32_NEXT_ADDR, dmc->access_address_min7_43_32_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX7_31_00_NEXT_ADDR, dmc->access_address_max7_31_00_next);
	dmc_write_reg(mcu->id, ACCESS_ADDRESS_MAX7_43_32_NEXT_ADDR, dmc->access_address_max7_43_32_next);
	dmc_write_reg(mcu->id, REFRESH_CONTROL_NEXT_ADDR, dmc->refresh_control_next);
	dmc_write_reg(mcu->id, MEMORY_TYPE_NEXT_ADDR, dmc->memory_type_next);
	dmc_write_reg(mcu->id, FEATURE_CONFIG_ADDR, dmc->feature_config);
	dmc_write_reg(mcu->id, QUEUE_ALLOCATE_CONTROL_031_000_ADDR, dmc->queue_allocate_control_031_000);
	dmc_write_reg(mcu->id, QUEUE_ALLOCATE_CONTROL_063_032_ADDR, dmc->queue_allocate_control_063_032);
	dmc_write_reg(mcu->id, QUEUE_ALLOCATE_CONTROL_095_064_ADDR, dmc->queue_allocate_control_095_064);
	dmc_write_reg(mcu->id, QUEUE_ALLOCATE_CONTROL_127_096_ADDR, dmc->queue_allocate_control_127_096);
	dmc_write_reg(mcu->id, SCRUB_CONTROL0_NEXT_ADDR, dmc->scrub_control0_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN0_NEXT_ADDR, dmc->scrub_address_min0_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX0_NEXT_ADDR, dmc->scrub_address_max0_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL1_NEXT_ADDR, dmc->scrub_control1_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN1_NEXT_ADDR, dmc->scrub_address_min1_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX1_NEXT_ADDR, dmc->scrub_address_max1_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL2_NEXT_ADDR, dmc->scrub_control2_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN2_NEXT_ADDR, dmc->scrub_address_min2_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX2_NEXT_ADDR, dmc->scrub_address_max2_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL3_NEXT_ADDR, dmc->scrub_control3_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN3_NEXT_ADDR, dmc->scrub_address_min3_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX3_NEXT_ADDR, dmc->scrub_address_max3_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL4_NEXT_ADDR, dmc->scrub_control4_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN4_NEXT_ADDR, dmc->scrub_address_min4_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX4_NEXT_ADDR, dmc->scrub_address_max4_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL5_NEXT_ADDR, dmc->scrub_control5_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN5_NEXT_ADDR, dmc->scrub_address_min5_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX5_NEXT_ADDR, dmc->scrub_address_max5_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL6_NEXT_ADDR, dmc->scrub_control6_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN6_NEXT_ADDR, dmc->scrub_address_min6_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX6_NEXT_ADDR, dmc->scrub_address_max6_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL7_NEXT_ADDR, dmc->scrub_control7_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MIN7_NEXT_ADDR, dmc->scrub_address_min7_next);
	dmc_write_reg(mcu->id, SCRUB_ADDRESS_MAX7_NEXT_ADDR, dmc->scrub_address_max7_next);
	dmc_write_reg(mcu->id, FEATURE_CONTROL_NEXT_ADDR, dmc->feature_control_next);
	dmc_write_reg(mcu->id, MUX_CONTROL_NEXT_ADDR, dmc->mux_control_next);
	dmc_write_reg(mcu->id, RANK_REMAP_CONTROL_NEXT_ADDR, dmc->rank_remap_control_next);
	dmc_write_reg(mcu->id, SCRUB_CONTROL_NEXT_ADDR, dmc->scrub_control_next);
	dmc_write_reg(mcu->id, T_REFI_NEXT_ADDR, dmc->t_refi_next);
	dmc_write_reg(mcu->id, T_RFC_NEXT_ADDR, dmc->t_rfc_next);
	dmc_write_reg(mcu->id, T_MRR_NEXT_ADDR, dmc->t_mrr_next);
	dmc_write_reg(mcu->id, T_MRW_NEXT_ADDR, dmc->t_mrw_next);
	dmc_write_reg(mcu->id, T_RDPDEN_NEXT_ADDR, dmc->t_rdpden_next);
	dmc_write_reg(mcu->id, T_RCD_NEXT_ADDR, dmc->t_rcd_next);
	dmc_write_reg(mcu->id, T_RAS_NEXT_ADDR, dmc->t_ras_next);
	dmc_write_reg(mcu->id, T_RP_NEXT_ADDR, dmc->t_rp_next);
	dmc_write_reg(mcu->id, T_RPALL_NEXT_ADDR, dmc->t_rpall_next);
	dmc_write_reg(mcu->id, T_RRD_NEXT_ADDR, dmc->t_rrd_next);
	dmc_write_reg(mcu->id, T_ACT_WINDOW_NEXT_ADDR, dmc->t_act_window_next);
	dmc_write_reg(mcu->id, T_RTR_NEXT_ADDR, dmc->t_rtr_next);
	dmc_write_reg(mcu->id, T_RTW_NEXT_ADDR, dmc->t_rtw_next);
	dmc_write_reg(mcu->id, T_RTP_NEXT_ADDR, dmc->t_rtp_next);
	dmc_write_reg(mcu->id, T_WR_NEXT_ADDR, dmc->t_wr_next);
	dmc_write_reg(mcu->id, T_WTR_NEXT_ADDR, dmc->t_wtr_next);
	dmc_write_reg(mcu->id, T_WTW_NEXT_ADDR, dmc->t_wtw_next);
	dmc_write_reg(mcu->id, T_XMPD_NEXT_ADDR, dmc->t_xmpd_next);
	dmc_write_reg(mcu->id, T_EP_NEXT_ADDR, dmc->t_ep_next);
	dmc_write_reg(mcu->id, T_XP_NEXT_ADDR, dmc->t_xp_next);
	dmc_write_reg(mcu->id, T_ESR_NEXT_ADDR, dmc->t_esr_next);
	dmc_write_reg(mcu->id, T_XSR_NEXT_ADDR, dmc->t_xsr_next);
	dmc_write_reg(mcu->id, T_ESRCK_NEXT_ADDR, dmc->t_esrck_next);
	dmc_write_reg(mcu->id, T_CKXSR_NEXT_ADDR, dmc->t_ckxsr_next);
	dmc_write_reg(mcu->id, T_CMD_NEXT_ADDR, dmc->t_cmd_next);
	dmc_write_reg(mcu->id, T_PARITY_NEXT_ADDR, dmc->t_parity_next);
	dmc_write_reg(mcu->id, T_ZQCS_NEXT_ADDR, dmc->t_zqcs_next);
	dmc_write_reg(mcu->id, T_RDDATA_EN_NEXT_ADDR, dmc->t_rddata_en_next);
	dmc_write_reg(mcu->id, T_PHYRDLAT_NEXT_ADDR, dmc->t_phyrdlat_next);
	dmc_write_reg(mcu->id, T_PHYWRLAT_NEXT_ADDR, dmc->t_phywrlat_next);
	dmc_write_reg(mcu->id, RDLVL_CONTROL_NEXT_ADDR, dmc->rdlvl_control_next);
	dmc_write_reg(mcu->id, RDLVL_MRS_NEXT_ADDR, dmc->rdlvl_mrs_next);
	dmc_write_reg(mcu->id, T_RDLVL_EN_NEXT_ADDR, dmc->t_rdlvl_en_next);
	dmc_write_reg(mcu->id, T_RDLVL_RR_NEXT_ADDR, dmc->t_rdlvl_rr_next);
	dmc_write_reg(mcu->id, WRLVL_CONTROL_NEXT_ADDR, dmc->wrlvl_control_next);
	dmc_write_reg(mcu->id, WRLVL_MRS_NEXT_ADDR, dmc->wrlvl_mrs_next);
	dmc_write_reg(mcu->id, T_WRLVL_EN_NEXT_ADDR, dmc->t_wrlvl_en_next);
	dmc_write_reg(mcu->id, T_WRLVL_WW_NEXT_ADDR, dmc->t_wrlvl_ww_next);
	dmc_write_reg(mcu->id, PHY_POWER_CONTROL_NEXT_ADDR, dmc->phy_power_control_next);
	dmc_write_reg(mcu->id, T_LPRESP_NEXT_ADDR, dmc->t_lpresp_next);
	dmc_write_reg(mcu->id, PHY_UPDATE_CONTROL_NEXT_ADDR, dmc->phy_update_control_next);
	dmc_write_reg(mcu->id, ODT_TIMING_NEXT_ADDR, dmc->odt_timing_next);
	dmc_write_reg(mcu->id, ODT_WR_CONTROL_31_00_NEXT_ADDR, dmc->odt_wr_control_31_00_next);
	dmc_write_reg(mcu->id, ODT_WR_CONTROL_63_32_NEXT_ADDR, dmc->odt_wr_control_63_32_next);
	dmc_write_reg(mcu->id, ODT_RD_CONTROL_31_00_NEXT_ADDR, dmc->odt_rd_control_31_00_next);
	dmc_write_reg(mcu->id, ODT_RD_CONTROL_63_32_NEXT_ADDR, dmc->odt_rd_control_63_32_next);
	dmc_write_reg(mcu->id, DQ_MAP_CONTROL_15_00_NEXT_ADDR, dmc->dq_map_control_15_00_next);
	dmc_write_reg(mcu->id, DQ_MAP_CONTROL_31_16_NEXT_ADDR, dmc->dq_map_control_31_16_next);
	dmc_write_reg(mcu->id, DQ_MAP_CONTROL_47_32_NEXT_ADDR, dmc->dq_map_control_47_32_next);
	dmc_write_reg(mcu->id, DQ_MAP_CONTROL_63_48_NEXT_ADDR, dmc->dq_map_control_63_48_next);
	dmc_write_reg(mcu->id, DQ_MAP_CONTROL_71_64_NEXT_ADDR, dmc->dq_map_control_71_64_next);
	dmc_write_reg(mcu->id, PHY_RDWRDATA_CS_MASK_31_00_ADDR, dmc->phy_rdwrdata_cs_mask_31_00);
	dmc_write_reg(mcu->id, PHY_RDWRDATA_CS_MASK_63_32_ADDR, dmc->phy_rdwrdata_cs_mask_63_32);
	dmc_write_reg(mcu->id, PHY_REQUEST_CS_REMAP_ADDR, dmc->phy_request_cs_remap);

	/* User Config2 Update based of MCU Software Spec */
	config_dmc_user_config(mcu);

	/*Update DMC register */
	dmc_update(mcu->id);
}

/* Configure DMC Parameters */
void config_dmc_parameters(struct apm_mcu *mcu)
{
	/* Initialize to Reset value */
	memcpy(&dmc, &dmc_reset_values, sizeof(struct dmc_param));
	ddr_info("MCU[%d] - DMC Registers setup \n", mcu->id);

	/* Config DMC Parameters */
	config_dmc_feature_parameters(&dmc, mcu);
	config_dmc_dfi_parameters(&dmc, mcu);
	config_dmc_odt_timing_paramaters(&dmc, mcu);
	/* Always after ODT and DFI parameters are calculated */
	config_dmc_timing_parameters(&dmc, mcu);
	config_dmc_perf_parameters(&dmc);
	config_dmc_low_power_parameters(&dmc);
	config_dmc_addressing_parameters(&dmc, mcu);
	config_dmc_rank_remapping(&dmc, mcu);
	config_dmc_odt_wr_control(&dmc, mcu);
	config_dmc_odt_rd_control(&dmc, mcu);
	config_dmc_phyrdwr_data_cs(&dmc, mcu);
	config_dmc_memory_address(&dmc, mcu);
	config_dmc_dq_mapping(&dmc, mcu);
	config_read_training(&dmc);
	config_write_training(&dmc);

	/* Issue writes to the DMC */
	/* Update _now registers via UPDATE command */
	update_dmc_config(&dmc, mcu);
}


