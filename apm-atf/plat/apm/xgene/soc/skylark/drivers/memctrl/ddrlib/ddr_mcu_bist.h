#ifndef __DDR_MCU_BIST_H__
#define __DDR_MCU_BIST_H__

#include "apm_ddr_sdram.h"

/* Organize to compare all nibbles in slice */
typedef struct apm_mcu_bist_72B_data {
	unsigned char darr[8][9];
} apm_mcu_bist_2x64B_data_t;

typedef struct apm_mcu_bist_address_setup {
	unsigned int rank0;
	unsigned int rank1;
	unsigned int bank;
	unsigned int bankgroup0;
	unsigned int bankgroup1;
	unsigned int cid0;
	unsigned int cid1;
	unsigned int row;
	unsigned int col;
} apm_mcu_bist_address_setup_t;

typedef struct apm_mcu_bist_config {
	unsigned int itercnt;
	unsigned int bistcrcen;
	unsigned int bist2t;
	unsigned int bistcmprdsbl;
	unsigned int stoponmiscompare;
	unsigned int stoponalert;
	unsigned int readdbien;
	unsigned int writedbien;
	unsigned int mapcidtocs;
	unsigned int rdloopcnt;
	unsigned int wrloopcnt;
} apm_mcu_bist_config_t;

#define BIST_PATTERN0			0x55
#define BIST_PATTERN1			0xAA
#define BIST_PATTERN2			0x5A
#define BIST_PATTERN3			0xA5
#define BIST_PATTERN4			0x0F
#define BIST_PATTERN5			0xF0

#define BIST_MAX_PATTERNS               6 /* Useful patterns */

/* WR CAL Pattern Selector ID */
#define WR_CAL_PATTERN_SELECTOR		(BIST_MAX_PATTERNS + 1)


#define MCU_BIST_BANK 			0
#define MCU_BIST_BANKGROUP0 		0
#define MCU_BIST_BANKGROUP1 		0
#define MCU_BIST_CID0 			0
#define MCU_BIST_CID1 			0
#define MCU_BIST_ROW 			0
#define MCU_BIST_COL 			0

#define MCU_BIST_ITER_CNT		10 // 0xFFFF
#define MCU_BIST_CRC_EN			0
#define MCU_BIST_CMPR_DSBL		0
#define MCU_BIST_STOP_ON_MISCOMPARE	1
#define MCU_BIST_STOP_ON_ALERT		1
#define MCU_BIST_READ_DBI_EN		0
#define MCU_BIST_WRITE_DBI_EN		0
#define MCU_BIST_MAP_CID_TO_CS		0
#define MCU_BIST_RDLOOP_CNT		1 // 15
#define MCU_BIST_WRLOOP_CNT		1 // 15

/* Maximum number of cycles to delay the incoming dfi_wrdata_en/dfi_wrdata signal */
#define MAX_WRITE_PATH_LAT_ADD_CYCLES	7

/*  Function headers */
/* ddr_phy_mcu_bist.c */
int mcu_bist_phy_wrcal(struct apm_mcu *mcu, unsigned int rank);
int mcu_bist_phy_wrdeskew(struct apm_mcu *mcu, unsigned int rank);
int mcu_bist_phy_rddeskew(struct apm_mcu *mcu, unsigned int rank);
int mcu_dram_vref_training(struct apm_mcu *mcu, unsigned int rank);
int mcu_phy_vref_training(struct apm_mcu *mcu, unsigned int rankmask);

/* ddr_mcu_bist_utils.c */
void mcu_bist_datacmp_reg_dump(unsigned int mcu_id, unsigned int line);
int run_wrdqdly_le_bist (struct apm_mcu *mcu,
				  unsigned int slice,
				  unsigned int bit,
				  unsigned int physliceon,
				  unsigned int *delay_le,
				  unsigned int start_delay,
				  unsigned int end_delay,
				  unsigned int delay_step);
int run_wrdqdly_te_bist (struct apm_mcu *mcu,
				  unsigned int slice,
				  unsigned int bit,
				  unsigned int physliceon,
				  unsigned int *delay_te,
				  unsigned int start_delay,
				  unsigned int end_delay,
				  unsigned int delay_step);
int run_rddqdly_le_bist (struct apm_mcu *mcu,
				  unsigned int rise,
				  unsigned int slice,
				  unsigned int bit,
				  unsigned int physliceon,
				  unsigned int *delay_le,
				  unsigned int start_delay,
				  unsigned int end_delay,
				  unsigned int delay_step);
int run_rddqdly_te_bist (struct apm_mcu *mcu,
				  unsigned int rise,
				  unsigned int slice,
				  unsigned int bit,
				  unsigned int physliceon,
				  unsigned int *delay_te,
				  unsigned int start_delay,
				  unsigned int end_delay,
				  unsigned int delay_step);
void mcu_bist_start(unsigned int mcu_id);
void mcu_bist_stop(unsigned int mcu_id);
int mcu_bist_completion_poll(unsigned int mcu_id);
unsigned int mcu_bist_err_status(unsigned int mcu_id);
void mcu_bist_address_setup(unsigned int mcu_id,
			    struct apm_mcu_bist_address_setup *b_addr_setup);
void mcu_bist_config(unsigned int mcu_id, unsigned int bist2t,
		     struct apm_mcu_bist_config *b_config);
int mcu_bist_status(unsigned int mcu_id);
int mcu_bist_byte_status_line0(unsigned int mcu_id, unsigned int device_type,
			       unsigned int byte);
void update_bist_datapattern(unsigned int mcu_id, unsigned int test);
void mcu_bist_peek_wrdataX(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata,
			   unsigned int dataX);
void mcu_bist_peek_rddataX(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata,
			   unsigned int dataX);
int phy_loopback_test(apm_mcu_t *mcu, phylpbkopt_e lpbk_opt);
int  phy_lpbck_sequence(apm_mcu_t *mcu, phylpbkcnt_e ctrl_lpbk_cnt,
			phylpbkdtype_e ctrl_data_type, phylpbkmux_e lpbk_mux_ext );
void mcu_bist_data_setup(unsigned int mcu_id, apm_mcu_bist_2x64B_data_t *pdata0,
			 apm_mcu_bist_2x64B_data_t *pdata1);
int mcu_bist_datacmp(unsigned int mcu_id, unsigned long long bitmask, unsigned int ecc_bitmask);
void mcu_bist_setup(unsigned int mcu_id, unsigned int rank, unsigned int bist2t);

#endif /* __DDR_MCU_BIST_H__ */
