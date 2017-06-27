/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef _DDR_SPD_H_
#define _DDR_SPD_H_

#ifdef DDR3_MODE

/*
 * Format from "JEDEC Standard No. 21-C,
 * Appendix D: Rev 1.0: SPD's for DDR SDRAM
 */
typedef struct ddr3_spd_eeprom_s {
	/* General Section: Bytes 0-59 */
	unsigned char info_size_crc;	/*  0 # bytes written into serial memory,
					   CRC coverage */
	unsigned char spd_rev;		/*  1 SPD revision */
	unsigned char mem_type;		/*  2 Key Byte / Fundamental mem type */
	unsigned char module_type;	/*  3 Key Byte / Module Type */
	unsigned char density_banks;	/*  4 SDRAM Density and Banks */
	unsigned char addressing;	/*  5 SDRAM Addressing */
        unsigned char module_vdd;	/*  6 Module nominal voltage, VDD */
	unsigned char organization;	/*  7 Module Organization */
	unsigned char bus_width;	/*  8 Module Memory Bus Width */
	unsigned char ftb_div;		/*  9 Fine Timebase (FTB)
					   Dividend / Divisor */
	unsigned char mtb_dividend;	/* 10 Medium Timebase (MTB) Dividend */
	unsigned char mtb_divisor;	/* 11 Medium Timebase (MTB) Divisor */
	unsigned char tCK_min;		/* 12 SDRAM Minimum Cycle Time */
	unsigned char res_13;		/* 13 Reserved */
	unsigned char caslat_lsb;	/* 14 CAS Latencies Supported,
					   Least Significant Byte */
	unsigned char caslat_msb;	/* 15 CAS Latencies Supported,
					   Most Significant Byte */
	unsigned char tAA_min;		/* 16 Min CAS Latency Time */
	unsigned char tWR_min;		/* 17 Min Write REcovery Time */
	unsigned char tRCD_min;		/* 18 Min RAS# to CAS# Delay Time */
	unsigned char tRRD_min;		/* 19 Min Row Active to
					   Row Active Delay Time */
	unsigned char tRP_min;		/* 20 Min Row Precharge Delay Time */
	unsigned char tRAS_tRC_ext;	/* 21 Upper Nibbles for tRAS and tRC */
	unsigned char tRAS_min_lsb;	/* 22 Min Active to Precharge
					   Delay Time */
	unsigned char tRC_min_lsb;	/* 23 Min Active to Active/Refresh
					   Delay Time, LSB */
	unsigned char tRFC_min_lsb;	/* 24 Min Refresh Recovery Delay Time */
	unsigned char tRFC_min_msb;	/* 25 Min Refresh Recovery Delay Time */
	unsigned char tWTR_min;		/* 26 Min Internal Write to
					   Read Command Delay Time */
	unsigned char tRTP_min;		/* 27 Min Internal Read to Precharge
					   Command Delay Time */
	unsigned char tFAW_msb;		/* 28 Upper Nibble for tFAW */
	unsigned char tFAW_min;		/* 29 Min Four Activate Window
					   Delay Time */
	unsigned char opt_features;	/* 30 SDRAM Optional Features */
	unsigned char therm_ref_opt;	/* 31 SDRAM Thermal and Refresh Opts */

	unsigned char therm_ref_sens;	/* 32 SDRAM Thermal sensor */
	unsigned char sdram_dev_type;	/* 33 SDRAM device type */
	unsigned char tckmin_correct;   /* 34 tCKmin correction value */
	unsigned char taamin_correct;   /* 35 taamin correction value */
	unsigned char trcdmin_correct;  /* 36 trcdmin correction value */
	unsigned char trpmin_correct;   /* 37 trpmin correction value */
	unsigned char trcmin_correct;   /* 38 trcmin correction value */
	unsigned char res_39_59[21];	/* 39-59 Reserved, General Section */

	/* Module-Specific Section: Bytes 60-116 */
	union {
		struct {
			/* 60 (Unbuffered) Module Nominal Height */
			unsigned char mod_height;
			/* 61 (Unbuffered) Module Maximum Thickness */
			unsigned char mod_thickness;
			/* 62 (Unbuffered) Reference Raw Card Used */
			unsigned char ref_raw_card;
			/* 63 (Unbuffered) Address Mapping from
			   Edge Connector to DRAM */
			unsigned char addr_mapping;
			/* 64-116 (Unbuffered) Reserved */
			unsigned char res_64_116[53];
		} unbuffered;
		struct {
			/* 60 (Registered) Module Nominal Height */
			unsigned char mod_height;
			/* 61 (Registered) Module Maximum Thickness */
			unsigned char mod_thickness;
			/* 62 (Registered) Reference Raw Card Used */
			unsigned char ref_raw_card;
			/* 63-64 */
			unsigned char res_63_64[2];
			/* 65-66 Register Mfg ID */
			unsigned char rmid_lsb;
			unsigned char rmid_msb;
			/* 67-116 (Unbuffered) Reserved */
			unsigned char res_67_116[50];
		} registered;
                unsigned char uc[57];   /* 60-116 Module-Specific Section */
	} mod_section;

	/* Unique Module ID: Bytes 117-125 */
	unsigned char mmid_lsb;	/* 117 Module MfgID Code LSB - JEP-106 */
	unsigned char mmid_msb;	/* 118 Module MfgID Code MSB - JEP-106 */
	unsigned char mloc;	/* 119 Mfg Location */
	unsigned char mdate[2];	/* 120-121 Mfg Date */
	unsigned char sernum[4];	/* 122-125 Module Serial Number */

	/* CRC: Bytes 126-127 */
	unsigned char crc[2];	/* 126-127 SPD CRC */

	/* Other Manufacturer Fields and User Space: Bytes 128-255 */
	unsigned char mpart[18];	/* 128-145 Mfg's Module Part Number */
	unsigned char mrev[2];	/* 146-147 Module Revision Code */

	unsigned char dmid_lsb;	/* 148 DRAM MfgID Code LSB - JEP-106 */
	unsigned char dmid_msb;	/* 149 DRAM MfgID Code MSB - JEP-106 */

	unsigned char msd[26];	/* 150-175 Mfg's Specific Data */
	unsigned char cust[80];	/* 176-255 Open for Customer Use */

} spd_eeprom_t;

#else

typedef struct ddr4_spd_eeprom_s {
	/* Block 0: Base Configuration and DRAM Parameters */
	unsigned char info_size_crc;	/*  0 # bytes written into serial memory,
					      CRC coverage */
	unsigned char spd_rev;		/*  1 SPD revision */
	unsigned char mem_type;		/*  2 Key Byte / Fundamental mem type */
	unsigned char module_type;	/*  3 Key Byte / Module Type
					      0x1 = RDIMM, 0x2 = UDIMM, 0x4 = LRDIMM */
	unsigned char density_banks;	/*  4 SDRAM Density and Banks */
	unsigned char addressing;	/*  5 SDRAM Addressing */
	unsigned char package_type;	/*  6 Package Type */
	unsigned char opt_features;	/*  7 SDRAM optional features */
	unsigned char therm_ref_opt;	/*  8 SDRAM Thermal and Refresh Opts */

	unsigned char oth_opt_features;	/*  9 Other SDRAM optional features */
	unsigned char res_10;		/* 10 Reserved */
	unsigned char module_vdd;	/* 11 Module nominal voltage, VDD */
	unsigned char organization;	/* 12 Module Organization */
	unsigned char bus_width;	/* 13 Module Memory Bus Width */
	unsigned char therm_ref_sens;	/* 14 SDRAM Thermal sensor */
	unsigned char extd_module_type;	/* 15 Extended module type */
	unsigned char res_16;		/* 16 Reserved */
	unsigned char timebases;	/* 17 Timebases */
	unsigned char tCKAVG_min;	/* 18 SDRAM Minimum Cycle Time */
	unsigned char tCKAVG_max;	/* 19 SDRAM Minimum Cycle Time */
	unsigned char caslat_0;		/* 20 CAS Latencies Supported,
					      Least Significant Byte */
	unsigned char caslat_1;		/* 21 CAS Latencies Supported,
					      second byte */
	unsigned char caslat_2;		/* 22 CAS Latencies Supported,
					      third byte */
	unsigned char caslat_3;		/* 23 CAS Latencies Supported,
					      Most Significant Byte */
	unsigned char tAA_min;		/* 24 Min CAS Latency Time */
	unsigned char tRCD_min;		/* 25 Min RAS# to CAS# Delay Time */
	unsigned char tRP_min;		/* 26 Min Row Precharge Delay Time */
	unsigned char tRAS_tRC_ext;	/* 27 Upper Nibbles for tRAS and tRC */
	unsigned char tRAS_min_lsb;	/* 28 Min Active to Precharge Delay Time, LSB */
	unsigned char tRC_min_lsb;	/* 29 Min Active to Active/Refresh
					      Delay Time, LSB */
	unsigned char tRFC1_min_lsb;	/* 30 Min Refresh Recovery Delay Time 1 */
	unsigned char tRFC1_min_msb;	/* 31 Min Refresh Recovery Delay Time 1 */
	unsigned char tRFC2_min_lsb;	/* 32 Min Refresh Recovery Delay Time 2 */
	unsigned char tRFC2_min_msb;	/* 33 Min Refresh Recovery Delay Time 2 */
	unsigned char tRFC4_min_lsb;	/* 34 Min Refresh Recovery Delay Time 4 */
	unsigned char tRFC4_min_msb;	/* 35 Min Refresh Recovery Delay Time 4 */
	unsigned char tFAW_msb;		/* 36 Upper Nibble for tFAW */
	unsigned char tFAW_min;		/* 37 Min Four Activate Window Delay Time */
	unsigned char tRRD_S_min;	/* 38 Min Row Active to Row Active Delay Time,
					      different bank group */
	unsigned char tRRD_L_min;	/* 39 Min Row Active to Row Active Delay Time
					      same bank group */
	unsigned char tCCD_L_min;	/* 40 Min CAS to CAS Delay Time,
					      same bank group */
	unsigned char res_41_59[19];	/* 41-59 Reserved, must be 0x00 */
	unsigned char cntr_bit_map[18];	/* 60-77 Connector to SDRAM bit mapping */
	unsigned char res_78_116[39];	/* 78-116 Reserved, must be coded as 0x00 */
	unsigned char tCCD_L_min_fos;   /* 117 Fine offset for tCCD_L_min */
	unsigned char tRRD_L_min_fos;   /* 118 Fine offset for tRRD_L_min */
	unsigned char tRRD_S_min_fos;   /* 119 Fine offset for tRRD_S_min */
	unsigned char tRC_min_fos;	/* 120 Fine offset for tRC_min */
	unsigned char tRP_min_fos;	/* 121 Fine offset for tRP_min */
	unsigned char tRCD_min_fos;	/* 122 Fine offset for tRCD_min */
	unsigned char tAA_min_fos;	/* 123 Fine offset for tAA_min */
	unsigned char tCKAVG_max_fos;   /* 124 Fine offset for tCKAVG_max */
	unsigned char tCKAVG_min_fos;   /* 125 Fine offset for tCKAVG_min */
	unsigned char crc[2];		/* 126-127 SPD CRC */

	/* Module-Specific Section: Bytes 128-255 */
	union {
		struct {
			/* 128 (Unbuffered) Module Nominal Height */
			unsigned char mod_height;
			/* 129 (Unbuffered) Module Maximum Thickness */
			unsigned char mod_thickness;
			/* 130 (Unbuffered) Reference Raw Card Used */
			unsigned char ref_raw_card;
			/* 131 (Unbuffered) Address Mapping from
			   Edge Connector to DRAM */
			unsigned char addr_mapping;
			/* 132-253 (Unbuffered) Reserved */
			unsigned char res_132_253[122];
			/* 254-255 (Unbuffered) CRC for module specific section */
			unsigned char mod_crc[2];
		} unbuffered;
		struct {
                        /* 128 (Registered) Module Nominal Height */
                        unsigned char mod_height;
                        /* 129 (Registered) Module Maximum Thickness */
                        unsigned char mod_thickness;
                        /* 130 (Registered) Reference Raw Card Used */
                        unsigned char ref_raw_card;
                        /* 131 (Registered) DIMM Module Attributes */
                        unsigned char dimm_attr;
                        /* 132 (Registered) RDIMM Thermal Heat Speader Solution */
                        unsigned char dimm_heat_spreader;
			/* 133 (Registered) MfgID Code LSB */
			unsigned char rmid_lsb;
			/* 134 (Registered) MfgID Code MSB */
			unsigned char rmid_msb;
			/* 135 (Registered) Revision Number */
			unsigned char rmrev;
                        /* 136 (Registered) Address Mapping from
                           Edge Connector to DRAM */
                        unsigned char addr_mapping;
                        /* 137-253 (Registered) Reserved */
                        unsigned char res_137_253[117];
                        /* 254-255 (Registered) CRC for module specific section */
                        unsigned char mod_crc[2];
		} registered;
                struct {
                        /* 128 (load_reduced) Module Nominal Height */
                        unsigned char mod_height;
                        /* 129 (load_reduced) Module Maximum Thickness */
                        unsigned char mod_thickness;
                        /* 130 (load_reduced) Reference Raw Card Used */
                        unsigned char ref_raw_card;
                        /* 131 (load_reduced) DIMM Module Attributes */
                        unsigned char dimm_attr;
                        /* 132 (load_reduced) LRDIMM Thermal Heat Speader Solution */
                        unsigned char dimm_heat_spreader;
                        /* 133 (load_reduced) MfgID Code LSB */
                        unsigned char lrmid_lsb;
                        /* 134 (load_reduced) MfgID Code MSB */
                        unsigned char lrmid_msb;
                        /* 135 (load_reduced) Revision Number */
                        unsigned char rmrev;
                        /* 136 (load_reduced) Address Mapping from
                           Edge Connector to DRAM */
                        unsigned char addr_mapping;
                        /* 137 (load_reduced) Driver Strength for Control, Command and Address */
                        unsigned char ca_drv;
                        /* 138 (load_reduced) Drive Strength for CK */
                        unsigned char ck_drv;
                        /* 139 (load_reduced) Data Buffer Revision Number */
                        unsigned char dbrev;
                        /* 140-143 (load_reduced) DRAM VrefDQ for Package Rank 03 */
                        unsigned char vrefdq_rank[4];
                        /* 144 (load_reduced) Data Buffer VrefDQ for DRAM Interface */
                        unsigned char dbvrefdq;
                        /* 145 (load_reduced) Data Buffer MDQ Drive Strength
                               and RTT for <= 1866 */
                        unsigned char dbdq_drv_rtt_1866;
                        /* 146 (load_reduced) Data Buffer MDQ Drive Strength
                               and RTT for > 1866 and <= 2400 */
                        unsigned char dbdq_drv_rtt_1866_2400;
                        /* 147 (load_reduced) Data Buffer MDQ Drive Strength
                               and RTT for > 2400 and <= 3200 */
                        unsigned char dbdq_drv_rtt_2400_3200;
                        /* 148 (load_reduced) DRAM Drive Strength */
                        unsigned char dram_drv;
                        /* 149 (load_reduced) DRAM ODT (RTT_WR and RTT_NOM) <= 1866 */
                        unsigned char dram_rtt_wr_nom_1866;
                        /* 150 (load_reduced) DRAM ODT (RTT_WR and RTT_NOM) > 1866 and <= 2400 */
                        unsigned char dram_rtt_wr_nom_1866_2400;
                        /* 151 (load_reduced) DRAM ODT (RTT_WR and RTT_NOM) > 2400 and <= 3200 */
                        unsigned char dram_rtt_wr_nom_2400_3200;
                        /* 152 (load_reduced) DRAM ODT (RTT_PARK) <= 1866 */
                        unsigned char dram_rtt_park_1866;
                        /* 153 (load_reduced) DRAM ODT (RTT_PARK) > 1866 and <= 2400 */
                        unsigned char dram_rtt_park_1866_2400;
                        /* 154 (load_reduced) DRAM ODT (RTT_PARK) > 2400 and <= 3200 */
                        unsigned char dram_rtt_park_2400_3200;
                        /* 155-253 (load_reduced) Reserved */
                        unsigned char res_155_253[99];
                        /* 254-255 (load_reduced) CRC for module specific section */
                        unsigned char mod_crc[2];
                } load_reduced;
	} mod_section;

	/* Block 2, lower half - Reserved */
	unsigned char res_256_319[64];	/* 256-319 Reserved, must be coded as 0x00 */

	/* Block 2, upper half - Mfg info */
	unsigned char mmid_lsb;		/* 320 Module MfgID Code LSB - JEP-106 */
	unsigned char mmid_msb;		/* 321 Module MfgID Code MSB - JEP-106 */
	unsigned char mloc;		/* 322 Mfg Location */
	unsigned char mdate[2];		/* 323-324 Mfg Date */
	unsigned char sernum[4];	/* 325-328 Module Serial Number */
	unsigned char mpart[20];	/* 329-348 Mfg's Module Part Number */
	unsigned char mrev;		/* 349 Module Revision Code */

	unsigned char dmid_lsb;		/* 350 DRAM MfgID Code LSB - JEP-106 */
	unsigned char dmid_msb;		/* 351 DRAM MfgID Code MSB - JEP-106 */
	unsigned char dm_step;		/* 352 DRAM stepping */
	unsigned char msd[29];		/* 353-381 Mfg's Specific Data */
	unsigned char res_382_383[2];	/* 382_383 Reserved, must be coded as 0x00 */

	/* Block 3 - End User Programmable */
	unsigned char cust[128];	/* 384-511 Reserved for use by end users */
} spd_eeprom_t;

#endif

typedef struct mcu_spd_eeprom {
        unsigned int mcu_id;
        unsigned int slot;
        spd_eeprom_t spd_info;
} mcu_spd_eeprom_t;


void spd_ddr_init_hang(void);
void print_spd_basic(spd_eeprom_t *spd);
int spd_check(spd_eeprom_t * spd);

unsigned int compute_mem_type(spd_eeprom_t *spd);
unsigned int compute_mem_device_type(spd_eeprom_t *spd);
unsigned int compute_bank_group(spd_eeprom_t *spd);
unsigned int compute_package_type(spd_eeprom_t *spd);
unsigned long long compute_rank_capacity(spd_eeprom_t *spd);
unsigned int compute_no_ranks(spd_eeprom_t *spd);
unsigned int compute_row_bits(spd_eeprom_t *spd);
unsigned int compute_column_bits(spd_eeprom_t *spd);
unsigned int compute_bank_bits(spd_eeprom_t *spd);
unsigned int check_ecc_capability(spd_eeprom_t *spd);
unsigned int compute_spd_timing(int, int);
#ifdef DDR3_MODE
unsigned int compute_trrd_timing(spd_eeprom_t * spd);
unsigned int compute_trfc_timing(spd_eeprom_t * spd);
unsigned int compute_tck_min_ps(spd_eeprom_t * spd);
#else
unsigned int compute_tccd_l_timing(spd_eeprom_t * spd);
unsigned int compute_trrd_l_timing(spd_eeprom_t * spd);
unsigned int compute_trrd_s_timing(spd_eeprom_t * spd);
unsigned int compute_trfc1_timing(spd_eeprom_t * spd);
unsigned int compute_trfc2_timing(spd_eeprom_t * spd);
unsigned int compute_trfc4_timing(spd_eeprom_t * spd);
unsigned int compute_tckavg_min_ps(spd_eeprom_t * spd);
unsigned int compute_tckavg_max_ps(spd_eeprom_t * spd);
#endif
unsigned int compute_trc_timing(spd_eeprom_t *spd);
unsigned int compute_tras_timing(spd_eeprom_t *spd);
unsigned int compute_trp_timing(spd_eeprom_t *spd);
unsigned int compute_trcd_timing(spd_eeprom_t *spd);
unsigned int compute_taa_timing(spd_eeprom_t *spd);
unsigned int compute_tfaw_timing(spd_eeprom_t *spd);
unsigned int get_memory_clk_period_ps(void *p);
unsigned int time_ps_to_clk(void *p, unsigned int time_ps);
unsigned int mclk_to_picos(void *p, unsigned int mclk);
unsigned int compute_cas_latency(void *);
unsigned int compute_cas_write_latency(void *);
int spd_init(void *);
unsigned int spd_compare(mcu_spd_eeprom_t *, mcu_spd_eeprom_t *);



/*
 * Byte 2 Fundamental Memory Types.
 */
#define SPD_MEMTYPE_FPM		        (0x01)
#define SPD_MEMTYPE_EDO		        (0x02)
#define SPD_MEMTYPE_PIPE_NIBBLE 	(0x03)
#define SPD_MEMTYPE_SDRAM	        (0x04)
#define SPD_MEMTYPE_ROM		        (0x05)
#define SPD_MEMTYPE_SGRAM	        (0x06)
#define SPD_MEMTYPE_DDR		        (0x07)
#define SPD_MEMTYPE_DDR2	        (0x08)
#define SPD_MEMTYPE_DDR2_FBDIMM	        (0x09)
#define SPD_MEMTYPE_DDR2_FBDIMM_PROBE	(0x0A)
#define SPD_MEMTYPE_DDR3	        (0x0B)
#define SPD_MEMTYPE_DDR4	        (0x0C)

/*
 * Byte 3 Key Byte / Module Type for DDR3 SPD
 */
#define SPD_DDR3_MODTYPE_RDIMM		(0x01)
#define SPD_DDR3_MODTYPE_UDIMM		(0x02)
#define SPD_DDR3_MODTYPE_SODIMM		(0x03)
#define SPD_DDR3_MODTYPE_MICRODIMM	(0x04)
#define SPD_DDR3_MODTYPE_MINIRDIMM	(0x05)
#define SPD_DDR3_MODTYPE_MINIUDIMM	(0x06)
#define SPD_DDR3_MODTYPE_LRDIMM         (0x0B)

#define SPD_DDR4_MODTYPE_RDIMM          (0x01)
#define SPD_DDR4_MODTYPE_UDIMM          (0x02)
#define SPD_DDR4_MODTYPE_SODIMM         (0x03)
#define SPD_DDR4_MODTYPE_LRDIMM         (0x04)

/*
 * Byte 6 SDRAM Package Type
 */
#define SPD_DDP                         (0x0)
#define SPD_QDP                         (0x1)
#define SPD_3DS                         (0x2)

#define SPD_2H                          (0x1)
#define SPD_4H                          (0x3)
#define SPD_8H                          (0x7)

#define SPD_MTB_PS                      (125)
#define SPD_FTB_PS                      (1)

#endif /* _DDR_SPD_H_ */
