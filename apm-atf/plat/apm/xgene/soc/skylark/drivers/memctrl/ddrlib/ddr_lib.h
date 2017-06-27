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

#ifndef _DDR_LIB_H_
#define _DDR_LIB_H_

/* ARM Trusted Firmware */
#include "apm_ddr_sdram.h"
#define DSB_SY_CALL     __asm__ volatile("dsb sy")
#define DSB_SY_CALL_WDELAY     {DELAY(5); DSB_SY_CALL;}
#define __dc_zva(claddr) \
	__asm__ volatile ("\tdc zva, %0\n": : "r" (claddr): "memory")
#define DMB_SY_CALL     __asm__ volatile("dmb sy")
#define ISB     __asm__ volatile("isb")
#define htole32(x)	(x)
#define le32toh(x)	htole32(x)

#include "mcu_phy.h"
#include "ddr_mcu.h"
#include "ddr_mcu_bist.h"
#include "ddr_ud.h"

/* Arch timer delays */
#define DELAY(x)    udelay(x)			/* usec */
#define DELAYN(x)   udelay((x)/1000)		/* nsec */
#define DELAYP(x)   udelay((x)/1000/1000)	/* psec */
#define PRINT_PHY_TRDELAYS 1

#define DDR_SCU_WRITE32(_scu_offset, _data32) \
  (*(volatile unsigned int*)(SCU_BASE_ADDR + (_scu_offset)) = (htole32(_data32)))
#define DDR_SCU_READ32(_scu_offset) \
  ((unsigned int)(le32toh(*(volatile unsigned int*)(SCU_BASE_ADDR + (_scu_offset)))))

/******************************************************************************/
/*      PHY/MCU 							      */
/******************************************************************************/
#define IGNORE_INIT_PARITY_ERR

#define DIMM_ADDR_MIRROR_CTRL            0xAA

#define DIMM_MR1_DIC                     1

/* Single Rank UDIMM (DIMM-staticODT=40ohm) */
#define DIMM_MR1_RTTNOM_U1R              RttNom40ohm
/* Single Rank UDIMM (DIMM-staticODT=40ohm, DynamicODT=OFF) */
#define DIMM_MR1_RTTNOM_U2R              RttNom40ohm
/* Quad Rank RDIMM (DIMM-staticODT Even Rank =40ohm) */
#define DIMM_MR1_RTTNOM_R4R              RttNom40ohm
/* 2S - FarEndSlot (DIMM-staticODT Even Rank =40ohm) */
#define DIMM_MR1_RTTNOM_U2R2S0           RttNom40ohm
/* 2S - CloserSlot (DIMM-staticODT Even Rank =30ohm) */
#define DIMM_MR1_RTTNOM_U2R2S1           RttNom30ohm
/* Single Rank UDIMM   (DIMM-dynamicODT=OFF) */
#define DIMM_MR2_RTTWR_U1R               RttWrOff
/* Dual Rank One UDIMM (DIMM-dynamicODT=OFF) */
#define DIMM_MR2_RTTWR_U2R1S             RttWrOff
/* Dual Rank Two UDIMM (DIMM-dynamicODT=120ohm) */
#define DIMM_MR2_RTTWR_U2R2S             RttWr120ohm
/* Quad Rank RDIMM     (DIMM-dynamicODT=ON) */
#define DIMM_MR2_RTTWR_R4R               RttWr120ohm

#define DIMM_RANK_MASK                   0x1
/* Enable All ranks */
#define DIMM_DESKEW_RANK_MASK            0xFF
#define WR_DESKEW_EN                     1
/* 0: 1T mode, 1: 2T mode */
#define MCU_ENABLE_2T_MODE               0

/*  Function headers */
/* ddr_main.c */
int ddr_sys_setup(struct apm_memc *memc, unsigned int flag);
int ddr_sys_init(struct apm_memc *memc, unsigned int flag);

/* ddr_addrmap.c */
unsigned long long ddr_base_address(void);
unsigned int  compute_active_channels(void);
unsigned int  compute_no_of_ranks(void);
unsigned long long compute_rank_size(void);
unsigned long long ddr_mem_capacity(void);
unsigned long long get_sys_addr(unsigned long long phy_addr);
int cacheline_stride(unsigned int *val);
unsigned long long mcu_offset_address(unsigned int mcu_id, unsigned int rank_addr,
                                      unsigned int bank_addr, unsigned int row_addr,
                                      unsigned int column_addr);
int ddr_address_map(void *p, unsigned int flag);

/* ddr_mcu_config.c */
int mcu_phy_config_early_rddata_en(void *ptr);
int mcu_post_train_setup(void *ptr);
int setOdtMR1MR2_wrlvl(void *ptr, unsigned int rank);
int setOdtMR1MR2_funcmode(void *ptr);

/* ddr_mcu_utils.c */
unsigned long long mcu_addr(unsigned int mcu_id, unsigned int reg);
unsigned int mcu_read_reg(unsigned int mcu_id, unsigned int reg);
void mcu_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value);
void mcu_set_pll_params(struct mcu_pllctl *mcu_pllctl, apm_mcu_udparam_t ud_param, unsigned int value);
void update_bist_timing_registers(unsigned int mcu_id);
void update_pmu_registers(struct apm_mcu *mcu);
void update_bist_addressing(struct apm_mcu *mcu);
int mcu_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value,
		 unsigned int mask, unsigned int long long cycles);
void mcu_poison_disable_set(unsigned int mcu_id);
void mcu_poison_disable_clear(unsigned int mcu_id);
unsigned int ddr_speed_display(unsigned int t_ck_ps);

/* ddr_pcp_config.c */
int mcu_unreset(struct apm_memc *ddr);
int change_mcu_phy_dll_reset(struct apm_mcu *mcu);
int memc_ecc_init_en(struct apm_memc *memc, unsigned int flag);

/* ddr_pcp_utils.c */
unsigned int rdiv(unsigned long long a, unsigned long long b);
unsigned int cdiv(unsigned long long a, unsigned long long b);
unsigned int ilog2(unsigned long a);
unsigned long long rb_page_translate(unsigned int page_addr,
                                     unsigned int offset);
unsigned int rb_addr_read(unsigned long long sys_addr);
void rb_addr_write(unsigned long long sys_addr, unsigned int wr_data);
unsigned int rb_read(unsigned int page_addr, unsigned int offset);
void rb_write(unsigned int page_addr, unsigned int offset, unsigned int wr_data);
unsigned int mcb_read_reg(int mcb_id, unsigned int reg);
void mcb_write_reg(int mcb_id, unsigned int reg, unsigned int value);
int mcb_poll_reg(int mcb_id, unsigned int reg, unsigned int value, unsigned int mask);
unsigned int csw_read_reg(unsigned int reg);
void csw_write_reg(unsigned int reg, unsigned int value);
unsigned int iob_read_reg(unsigned int page_offset, unsigned int reg);
void iob_write_reg(unsigned int page_offset, unsigned int reg, unsigned int value);
int update_async_reset_mcu(struct apm_memc *memc, unsigned int val);
void mcu_board_specific_settings(struct apm_memc *memc);
int pcp_config_check(struct apm_memc *ddr);
int pcp_addressing_mode(struct apm_memc *ddr);
unsigned int get_mcu_pllcr0_page_offset(unsigned int index);
unsigned int get_mcu_pllcr1_page_offset(unsigned int index);
unsigned int get_mcu_pllcr2_page_offset(unsigned int index);
unsigned int get_mcu_pllcr3_page_offset(unsigned int index);
unsigned int get_mcu_pllcr4_page_offset(unsigned int index);
unsigned int get_mcu_pllsr_page_offset(unsigned int index);
unsigned int get_mcu_ccr_page_offset(unsigned int index);
unsigned int get_mcu_rcr_page_offset(unsigned int index);
int is_skylark_A0(void);

/* ddr_dmc_utils.c */
unsigned long long dmc_addr(unsigned int mcu_id, unsigned int reg);
unsigned int dmc_read_reg(unsigned int mcu_id, unsigned int reg);
void dmc_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value);
int dmc_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value,
                 unsigned int mask);
void dmc_cmd_multiple(unsigned int mcu_id, unsigned int direct_cmd, unsigned int rank_sel,
                      unsigned int bank, unsigned int expected_pslverr,
                      unsigned int abort, unsigned int override_replay);
void dmc_cmd_single(unsigned int mcu_id, unsigned int direct_cmd, unsigned int rank_bin,
                    unsigned int bank, unsigned int expected_pslverr,
                    unsigned int abort,unsigned int override_replay);
int dmc_ready_state(unsigned int mcu_id);
int dmc_config_state(unsigned int mcu_id);
int dmc_execute_drain(unsigned int mcu_id);
void dmc_update(unsigned int mcu_id);
int phy_update(unsigned int mcu_id);
void config_phy_low_power_parameters(unsigned int mcu_id);
unsigned int config_mr0_value(unsigned int t_wr, unsigned int cas_latency);
unsigned int config_mr1_value(unsigned int rtt_nom, unsigned int driver_impedance,
                              unsigned int write_lvl, unsigned int output_buffer_en);
unsigned int config_mr2_value(unsigned int cw_latency, unsigned int rtt_wr, unsigned int crc_enable);
unsigned int config_mr3_value(unsigned int t_ck_ps, unsigned int refresh_granularity,
                              unsigned int mpr_mode, unsigned int mpr_page, unsigned int pda_mode);
unsigned int config_mr4_value(unsigned int t_cal, unsigned int rd_preamble, unsigned wr_preamble);
unsigned int config_mr5_value(unsigned int rtt_park, unsigned int read_dbi, unsigned int write_dbi, unsigned int parity_latency);
unsigned int config_mr6_value(unsigned int vref_mode, unsigned int vref_training, unsigned int ccd_l);
void mr6_vreftrain(unsigned int mcu_id, unsigned int target_rank, unsigned int vref_mode,
		unsigned int vref_range, unsigned int vref_value, unsigned int ccd_l,
		unsigned int pda_component);
void mr3_pda_access(unsigned int mcu_id, unsigned int target_rank, unsigned int pda_mode,
		unsigned int component, unsigned int t_ck_ps, unsigned int refresh_granularity);
unsigned int ecc_enable(unsigned int mcu_id);
unsigned int crc_enable(unsigned int mcu_id);
unsigned int cal_t_wr(unsigned int mcu_id, unsigned int t_ck_ps, unsigned int write_preamble);
unsigned int read_t_cal(unsigned int mcu_id);
unsigned int read_dbi_enable(unsigned int mcu_id);
unsigned int write_dbi_enable(unsigned int mcu_id);
void dram_init(struct apm_mcu *mcu);
void dram_mrs_program(struct apm_mcu *mcu);
void zqcl_wait(struct apm_mcu *mcu, unsigned int zqcl_init);
void dram_zqcl(struct apm_mcu *mcu);
int dram_zqcs(unsigned int mcu_id);
int dmc_flush_seq(unsigned int mcu_id);
void dmc_ecc_errc_clear(unsigned int mcu_id);
void dmc_ecc_errd_clear(unsigned int mcu_id);
void dmc_ram_err_clear(unsigned int mcu_id);
void dmc_link_err_clear(unsigned int mcu_id);
void dmc_ecc_interrupt_ctrl(unsigned int mcu_id, unsigned int ram_errc, unsigned int ram_errd,
			    unsigned int dram_errc, unsigned int dram_errd);
unsigned int dmc_error_status(unsigned int mcu_id);
unsigned int get_rank_mux(unsigned int mcu_id, unsigned int rank,
                          unsigned int two_dpc_enable);
unsigned int mrs_inversion(unsigned int mrs_mr);
unsigned int address_mirroring(unsigned int address);
void config_rcd_buffer(struct apm_mcu *mcu);

/*ddr_dmc_config.c */
void config_dmc_timing_parameters(struct dmc_param *dmc, struct apm_mcu *mcu);
void config_dmc_perf_parameters(struct dmc_param *dmc);
void config_dmc_dfi_parameters(struct dmc_param *dmc, struct apm_mcu *mcu);
void config_dmc_low_power_parameters(struct dmc_param *dmc);
void config_dmc_odt_timing_paramaters(struct dmc_param *dmc, struct apm_mcu *mcu);
void config_dmc_addressing_parameters(struct dmc_param *dmc, struct apm_mcu *mcu);
void update_dmc_config(struct dmc_param *dmc, struct apm_mcu *mcu);
void config_dmc_parameters(struct apm_mcu *mcu);

/* ddr_phy_config.c */
int  phy_csr_config(struct apm_mcu *mcu);
void phy_post_train_setup(struct apm_mcu *mcu);
void set_phy_deskew_pll(unsigned int mcu_id);
void set_phy_config(struct apm_mcu *mcu);
void set_master_dly(unsigned int mcu_id);
void set_grp_slave_dly(struct apm_mcu *mcu);
void set_tsel_timing(struct apm_mcu *mcu);
void set_drv_term(struct apm_mcu *mcu);
void set_wrlvl_config(unsigned int mcu_id);
void set_gtlvl_config(unsigned int mcu_id);
void set_rdlvl_config(unsigned int mcu_id);

/* ddr_phy_training.c */
int phy_training_mode(struct apm_mcu *mcu);
int ddr_phy_training(struct apm_memc *memc, unsigned int flag);

/* ddr_phy_hw_leveling.c */
int dmc_wrlvl_routine(struct apm_mcu *mcu, unsigned int rank);
int dmc_rdgate_routine(struct apm_mcu *mcu, unsigned int rank);
int dmc_rdlvl_routine(struct apm_mcu *mcu, unsigned int rank);
void dmc_rdlvl_pattern(struct apm_mcu *mcu, unsigned int rank);

/* ddr_phy_utils.c */
unsigned long long phy_addr(unsigned int mcu_id, unsigned int reg);
unsigned int phy_read_reg(unsigned int mcu_id, unsigned int reg);
void phy_write_reg(unsigned int mcu_id, unsigned int reg, unsigned int value);
int phy_poll_reg(unsigned int mcu_id, unsigned int reg, unsigned int value,
                 unsigned int mask);
void mcu_phy_rdfifo_reset(unsigned int mcu_id);
void phy_vref_ctrl(unsigned int mcu_id, unsigned int vref_range, unsigned int vref_value);
void control_periodic_io_cal(unsigned int mcu_id, int set);
void phy_sw_rx_cal_setup(unsigned int mcu_id, unsigned int offset);
int phy_sw_rx_calibration(unsigned int mcu_id, unsigned int device_type, int cal_en);
void update_x4_fdbk_term_pvt(struct apm_mcu * mcu);

/* Check Observation registers */
int mcu_check_wrlvl_obs(unsigned int mcu_id,
                        unsigned int value);
int mcu_check_rdgate_obs(unsigned int mcu_id,
                         unsigned int value);
int mcu_check_rdlvl_obs(unsigned int mcu_id,
                        unsigned int value);
void config_per_cs_training_index(unsigned int mcu_id, unsigned int rank,
				  unsigned int per_cs_training_en);
void update_per_cs_training_index(unsigned int mcu_id, unsigned int rank);
unsigned int  get_per_cs_training_en(unsigned int mcu_id);
void mcu_phy_cal_start(unsigned int mcu_id);
void mcu_phy_peek_wrlvl_obs_sl(unsigned int mcu_id,
                              phy_slice_train_obs_t * datast,
                               unsigned int slice);
void mcu_phy_peek_wrlvl_obs_rnk(unsigned int mcu_id, unsigned int rank,
                                phy_rnk_train_obs_t * datast);
void mcu_phy_peek_wrlvl_res_sl(unsigned int mcu_id,
                               unsigned int device_type,
                               phy_slice_train_res_t * datast,
                               unsigned int slice);
void mcu_phy_peek_wrlvl_res_rnk(unsigned int mcu_id, unsigned int rank,
                                unsigned int device_type,
                                phy_rnk_train_res_t * datast);
void mcu_phy_peek_wrlvl_start_sl(unsigned int mcu_id,
                                 unsigned int device_type,
                                 phy_slice_train_res_t * datast,
                                 unsigned int slice);
void mcu_phy_peek_wrlvl_start_rnk(unsigned int mcu_id, unsigned int rank,
                                  unsigned int device_type,
                                  phy_rnk_train_res_t * datast);
void mcu_phy_peek_rdgate_obs_sl(unsigned int mcu_id,
                              phy_slice_train_obs_t * datast,
                                unsigned int slice);
void mcu_phy_peek_rdgate_obs_rnk(unsigned int mcu_id, unsigned int rank,
                                 phy_rnk_train_obs_t * datast);
void mcu_phy_peek_rdgate_res_sl(unsigned int mcu_id,
                                unsigned int device_type,
                                phy_slice_train_res_t * datast,
                                unsigned int slice);
void mcu_phy_peek_rdgate_res_rnk(unsigned int mcu_id, unsigned int rank,
                                 unsigned int device_type,
                                 phy_rnk_train_res_t * datast);
void mcu_phy_peek_rdeye_obs_sl(unsigned int mcu_id,
                               phy_slice_train_obs_t * datast,
                               unsigned int slice);
void mcu_phy_peek_rdeye_obs_rnk(unsigned int mcu_id, unsigned int rank,
                                phy_rnk_train_obs_t * datast);
void mcu_phy_peek_rdeye_res_sl(unsigned int mcu_id,
                               phy_slice_train_res_t * datast,
                               unsigned int slice);
void mcu_phy_peek_rdeye_res_rnk(unsigned int mcu_id, unsigned int rank,
                                phy_rnk_train_res_t * datast);
unsigned int calc_partial_RDDQS_start(unsigned int cperiod, unsigned int remndr);
void phy_wr_all_data_slices(unsigned int mcu_id, unsigned int addr,  unsigned int regd);
unsigned int wrdskw_get_reg_offset(unsigned int bit);
unsigned int wrdskw_set_delay_value(unsigned int bit, unsigned int regd, unsigned int delay);
unsigned int rddskw_rise_get_reg_offset(unsigned int bit);
unsigned int rddskw_rise_set_delay_value(unsigned int bit, unsigned int regd, unsigned int delay);
unsigned int rddskw_fall_get_reg_offset(unsigned int bit);
unsigned int rddskw_fall_set_delay_value(unsigned int bit, unsigned int regd, unsigned int delay);

/* ddr_post_training.c */
int ddr_post_training(struct apm_memc *memc, unsigned int flag);

/* ddr_pre_training.c */
int ddr_pre_training(struct apm_memc *memc, unsigned int flag);

/* ddr_ud_params.c */
void ddr_sys_default_params_mcu_setup(struct apm_mcu_udparam *udp);
void ddr_sys_default_params_mcb_setup(struct apm_mcb_udparam *mcbudp);
void ddr_sys_default_params_memc_setup(struct apm_memc *memc);
void ddr_sys_default_params_setup(struct apm_memc *memc);
int populate_mc_default_params(void *ptr);

/* Enums for polling */
#define POLL_GREATER	2
#define POLL_LESS	1
#define POLL_EQUAL      0

#endif /* _DDR_LIB_H_ */
