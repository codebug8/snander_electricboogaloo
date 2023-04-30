//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#include <dgputil.h>

#include "ui.h"
#include "spi_nand_cmds.h"
#include "spi_nand_ids.h"
#include "spi_nand_log.h"

static int spi_nand_bp_bit2_to_bit6(uint8_t status_reg, struct flash_status *flash_status)
{
	/* Direction of protection */
	bool tb = status_reg & SPI_NAND_SR1_TB;
	int bp = (status_reg & SPI_NAND_SR1_BP_MASK) >> SPI_NAND_SR1_BP_SHIFT;
	int divider = 1024 >> bp;
	uint32_t prot_start = 0, prot_end = 0;

	spi_nand_info("filling status with reg: 0x%02x, tb: %d, div %d\n",
			status_reg, tb, divider);

	if (bp == 0) {
		for (int i = 0; i < flash_status->num_regions; i++) {
			struct flash_region *region = &flash_status->regions[i];
			flash_region_update_status_to_unlocked(region);
		}
		return 0;
	}

	if (divider == 0) {
		for (int i = 0; i < flash_status->num_regions; i++) {
			struct flash_region *region = &flash_status->regions[i];
			flash_region_update_status_to_locked(region);
		}
		return 0;
	}

	//for (int i = 0; i < flash_status->num_regions; i++) {
	//struct flash_region *region = &flash_status->regions[i];
//
//		if (region->addr_start >= prot_start && region->addr_end <= prot_end) {
//			region->locked = true;
//		}
//	}

	return 0;
}

static int spi_nand_set_bp_from_status_bit2_to_bit6(uint8_t *status_reg, struct flash_status *flash_status)
{
	*status_reg &= ~SPI_NAND_SR1_BP_MASK;

	return 0;
}

static bool spi_nand_ecc_ok_winbond(uint8_t status) {
	uint8_t ecc_status = status >> 4 & 0x3;

	/* ecc status value bigger than 2 shows uncorrectable errors */
	return (ecc_status < 0x2);
}

static const struct spi_nand_flash_ooblayout ooblayout_esmt = {
	.oobsize = 36,
	.oobfree = {{0,1}, {8,8}, {16,1}, {24,8}, {32,1}, {40,8}, {48,1}, {56,8} }
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_esmt_41lb = {
	.oobsize = 20,
	.oobfree = {{0,4}, {4,4}, {20,4}, {36,4}, {52,4}}
};

static const struct spi_nand_flash_ooblayout ooblayout_mxic = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_winbond = {
	.oobsize = 32,
	.oobfree = {{0,8}, {16,8}, {32,8}, {48,8} }
};

static const struct spi_nand_flash_ooblayout ooblayout_gigadevice_a = {
	.oobsize = 48,
	.oobfree = {{0,12}, {16,12}, {32,12}, {48,12} }
};

static const struct spi_nand_flash_ooblayout ooblayout_gigadevice_128 = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_gigadevice_256 = {
	.oobsize = 128,
	.oobfree = {{0,128}}
};

static const struct spi_nand_flash_ooblayout ooblayout_zentel = {
	.oobsize = 36,
	.oobfree = {{0,1}, {8,8}, {16,1}, {24,8}, {32,1}, {40,8}, {48,1}, {56,8} }
};

static const struct spi_nand_flash_ooblayout ooblayout_etron_73C044SNB = {
	.oobsize = 64,
	.oobfree = {{0,16}, {30,16}, {60,16}, {90,16}}
};

static const struct spi_nand_flash_ooblayout ooblayout_etron_73D044SNA = {
	.oobsize = 72,
	.oobfree = {{0,18}, {32,18}, {64,18}, {96,18}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_etron_73D044SNC = {
	.oobsize = 64,
	.oobfree = {{0,16}, {30,16}, {60,16}, {90,16}}
};

static const struct spi_nand_flash_ooblayout ooblayout_gigadevice_GD5FXGQ4U = {
	.oobsize = 52,
	.oobfree = {{0,16}, {20,12}, {36,12}, {52,12}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type1 = {
	.oobsize = 32,
	.oobfree = {{0,8}, {16,8}, {32,8}, {48,8}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type2 = {
	.oobsize = 50,
	.oobfree = {{0,4}, {4,12}, {20,12}, {36,12}, {52,12}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type6 = {
	.oobsize = 33,
	.oobfree = {{0,1}, {8,8}, {24,8}, {40,8}, {56,8}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type10 = {
	.oobsize = 72,
	.oobfree = {{0,18}, {32,18}, {64,18}, {96,18}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type14 = {
	.oobsize = 20,
	.oobfree = {{0,4}, {4,4}, {36,4}, {68,4}, {100,4}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type15 = {
	.oobsize = 36,
	.oobfree = {{0,4}, {4,8}, {20,8}, {36,8}, {52,8}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type18 = {
	.oobsize = 96,
	.oobfree = {{0,24}, {32,24},  {64,24}, {96,24}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_type19 = {
	.oobsize = 44,
	.oobfree = {{0,4}, {8,40}}
};

static const struct spi_nand_flash_ooblayout ooblayout_etron_73E044SNA = {
	.oobsize = 144,
	.oobfree = {{0,18}, {32,18}, {64,18}, {96,18}, {128,18}, {160,18}, {192,18}, {224,18}}
};

static const struct spi_nand_flash_ooblayout ooblayout_toshiba_128 = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_toshiba_256 = {
	.oobsize = 128,
	.oobfree = {{0,128}}
};

static const struct spi_nand_flash_ooblayout ooblayout_micron = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_heyang = {
	.oobsize = 32,
	.oobfree = {{0,8}, {32,8}, {64,8}, {96,8}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_pn = {
	.oobsize = 44,
	.oobfree = {{0,4}, {4,2}, {19,2}, {34,2}, {49,2}, {96,32}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_ato = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_ato_25D2GA = {
	.oobsize = 12,
	.oobfree = {{0,3}, {16,3}, {32,3}, {48,3}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_ato_25D2GB = {
	.oobsize = 48,
	.oobfree = {{0,12}, {16,12}, {32,12}, {48,12}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_fm = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_fm_32 = {
	.oobsize = 32,
	.oobfree = {{0,32}}
};

static const struct spi_nand_flash_ooblayout ooblayout_spi_controller_ecc_64 = {
	.oobsize = 32,
	.oobfree = {{0,8}, {16,8}, {32,8}, {48,8}}
};

static const struct spi_nand_flash_ooblayout ooblayout_spi_controller_ecc_128 = {
	.oobsize = 96,
	.oobfree = {{0,8}, {16,8}, {32,8}, {48,8}, {64,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_spi_controller_ecc_256 = {
	.oobsize = 224,
	.oobfree = {{0,8}, {16,8}, {32,8}, {48,8}, {64,192}}
};

static const struct spi_nand_flash_ooblayout ooblayout_ds = {
	.oobsize = 20,
	.oobfree = {{0,8}, {20,4}, {36,4}, {52,4}}
};

static const struct spi_nand_flash_ooblayout ooblayout_fison = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_flash_ooblayout ooblayout_tym = {
	.oobsize = 12,
	.oobfree = {{0,3}, {16,3}, {32,3}, {48,3}}
};

static const struct spi_nand_flash_ooblayout ooblayout_xincun = {
	.oobsize = 48,
	.oobfree = {{16,48}, {116,12}}
};

/* only use user meta data with ECC protected */
static const struct spi_nand_flash_ooblayout ooblayout_foresee = {
	.oobsize = 64,
	.oobfree = {{0,64}}
};

static const struct spi_nand_id spi_nand_flash_tables[] = {
	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F1GQ4UAYIG,
		name:				"GIGADEVICE GD5F1GQ4UA",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_a,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F1GQ4UBYIG,
		name:				"GIGADEVICE GD5F1GQ4UB",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F1GQ4UCYIG,
		name:				"GIGADEVICE GD5F1GQ4UC",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F1GQ4UEYIS,
		name:				"GIGADEVICE GD5F1GQ4UE",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_GD5FXGQ4U,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F2GQ4UBYIG,
		name:				"GIGADEVICE GD5F2GQ4UB",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"GIGADEVICE GD5F2GQ4UE",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F2GQ4UE9IS,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type2,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F2GQ4UCYIG,
		name:				"GIGADEVICE GD5F2GQ4UC",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F4GQ4UBYIG,
		name:				"GIGADEVICE GD5F4GQ4UB",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_4KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_256BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_256KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_gigadevice_256,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_GIGADEVICE,
		dev_id:					_SPI_NAND_DEVICE_ID_GD5F4GQ4UCYIG,
		name:				"GIGADEVICE GD5F4GQ4UC",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_4KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_256BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_256KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout: 			&ooblayout_gigadevice_256,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:					_SPI_NAND_DEVICE_ID_F50L512M41A,
		name:				"ESMT F50L512",
		device_size:				SPI_NAND_DEVICE_SIZE_512MBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_esmt,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:					_SPI_NAND_DEVICE_ID_F50L1G41A0,
		name:				"ESMT F50L1G",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_esmt,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:					_SPI_NAND_DEVICE_ID_F50L1G41LB,
		name:				"ESMT F50L1G41LB",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_esmt_41lb,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:					_SPI_NAND_DEVICE_ID_F50L2G41LB,
		name:				"ESMT F50L2G41LB",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_esmt_41lb,
		feature:				SPI_NAND_FLASH_DIE_SELECT_1_HAVE,
	},

	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= _SPI_NAND_DEVICE_ID_W25N01GV,
		.name		= "WINBOND W25N01G",
		.device_size	= SPI_NAND_DEVICE_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.erase_size	= SPI_NAND_BLOCK_SIZE_128KBYTE,
		.dummy_mode	= SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		.read_mode	= SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		.write_mode	= SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		.oob_free_layout= &ooblayout_winbond,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,

		.ecc_ok		= spi_nand_ecc_ok_winbond,
		.fill_bp_status = spi_nand_bp_bit2_to_bit6,
		.set_bp_from_status = spi_nand_set_bp_from_status_bit2_to_bit6,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_WINBOND,
		dev_id:					_SPI_NAND_DEVICE_ID_W25M02GV,
		name:				"WINBOND W25M02G",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_winbond,
		feature:				SPI_NAND_FLASH_DIE_SELECT_1_HAVE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MXIC,
		dev_id:					_SPI_NAND_DEVICE_ID_MXIC35LF1GE4AB,
		name:				"MXIC MX35LF1G",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_mxic,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MXIC,
		dev_id:					_SPI_NAND_DEVICE_ID_MXIC35LF2GE4AB,
		name:				"MXIC MX35LF2G",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_mxic,
		feature:				SPI_NAND_FLASH_PLANE_SELECT_HAVE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ZENTEL,
		dev_id:					_SPI_NAND_DEVICE_ID_A5U12A21ASC,
		name:				"ZENTEL A5U12A21ASC",
		device_size:				SPI_NAND_DEVICE_SIZE_512MBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_zentel,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ZENTEL,
		dev_id:					_SPI_NAND_DEVICE_ID_A5U1GA21BWS,
		name:				"ZENTEL A5U1GA21BWS",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_zentel,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* Etron */
	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73C044SNB,
		name:				"ETRON EM73C044SNB",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_etron_73C044SNB,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73C044SND,
		name:				"ETRON EM73C044SND",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73C044SNF",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73C044SNF,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type10,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73C044VCA",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73C044VCA,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73C044VCD",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73C044VCD,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73D044VCA",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044VCA,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type18,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73D044VCB",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044VCB,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73D044VCD",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044VCD,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type10,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73D044VCG",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044VCG,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"ETRON EM73D044VCH",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044VCH,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044SNA,
		name:				"ETRON EM73D044SNA",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_etron_73D044SNA,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044SNC,
		name:				"ETRON EM73D044SNC",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_etron_73D044SNC,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044SND,
		name:				"ETRON EM73D044SND",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73D044SNF,
		name:				"ETRON EM73D044SNF",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type10,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ETRON,
		dev_id:					_SPI_NAND_DEVICE_ID_EM73E044SNA,
		name:				"ETRON EM73E044SNA",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_4KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_256BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_256KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_etron_73E044SNA,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:					_SPI_NAND_DEVICE_ID_TC58CVG0S3H,
		name:				"TOSHIBA TC58CVG0S3H",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout: 			&ooblayout_toshiba_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:					_SPI_NAND_DEVICE_ID_TC58CVG1S3H,
		name:				"TOSHIBA TC58CVG1S3H",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout: 			&ooblayout_toshiba_128,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:					_SPI_NAND_DEVICE_ID_TC58CVG2S0H,
		name:				"TOSHIBA TC58CVG2S0H",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_4KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_256KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout: 			&ooblayout_toshiba_256,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:					_SPI_NAND_DEVICE_ID_TC58CVG2S0HRAIJ,
		name:				"KIOXIA TC58CVG2S0HRAIJ",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_4KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_256KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout: 			&ooblayout_toshiba_256,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:					_SPI_NAND_DEVICE_ID_MT29F1G01,
		name:				"MICRON MT29F1G01",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_micron,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:					_SPI_NAND_DEVICE_ID_MT29F2G01,
		name:				"MICRON MT29F2G01",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_micron,
		feature:				SPI_NAND_FLASH_PLANE_SELECT_HAVE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:					_SPI_NAND_DEVICE_ID_MT29F4G01,
		name:				"MICRON MT29F4G01",
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_micron,
		feature:				SPI_NAND_FLASH_PLANE_SELECT_HAVE | SPI_NAND_FLASH_DIE_SELECT_2_HAVE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_HEYANG,
		dev_id:					_SPI_NAND_DEVICE_ID_HYF1GQ4UAACAE,
		name:				"HEYANG HYF1GQ4UAACAE",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_heyang,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_HEYANG,
		dev_id:					_SPI_NAND_DEVICE_ID_HYF2GQ4UAACAE,
		name:				"HEYANG HYF2GQ4UAACAE",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_heyang,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_HEYANG,
		dev_id:					_SPI_NAND_DEVICE_ID_HYF2GQ4UHCCAE,
		name:				"HEYANG HYF2GQ4UHCCAE",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type14,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_HEYANG,
		dev_id:					_SPI_NAND_DEVICE_ID_HYF1GQ4UDACAE,
		name:				"HEYANG HYF1GQ4UDACAE",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_HEYANG,
		dev_id:					_SPI_NAND_DEVICE_ID_HYF2GQ4UDACAE,
		name:				"HEYANG HYF2GQ4UDACAE",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_PN,
		dev_id:					_SPI_NAND_DEVICE_ID_PN26G01AWSIUG,
		name:				"PN PN26G01A-X",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_pn,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_PN,
		dev_id:					_SPI_NAND_DEVICE_ID_PN26G02AWSIUG,
		name:				"PN PN26G02A-X",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_pn,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ATO,
		dev_id:					_SPI_NAND_DEVICE_ID_ATO25D1GA,
		name:				"ATO ATO25D1GA",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_ato,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ATO,
		dev_id:					_SPI_NAND_DEVICE_ID_ATO25D2GA,
		name:				"ATO ATO25D2GA",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_ato_25D2GA,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_ATO_2,
		dev_id:					_SPI_NAND_DEVICE_ID_ATO25D2GB,
		name:				"ATO ATO25D2GB",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_ato_25D2GB,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* FM */
	{
		name:				"FM FM25S01",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25S01,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fm,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"FM FM25S01A",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25S01A,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fm_32,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25G01B,
		name:				"FM FM25G01B",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fm,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25G02B,
		name:				"FM FM25G02B",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fm,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25G02C,
		name:				"FM FM25G02C",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fm_32,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FM,
		dev_id:					_SPI_NAND_DEVICE_ID_FM25G02,
		name:				"FM FM25G02",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* XTX */
	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_XTX,
		dev_id:					_SPI_NAND_DEVICE_ID_XT26G02B,
		name:				"XTX XT26G02B",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"XTX XT26G01A",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_XTX,
		dev_id:					_SPI_NAND_DEVICE_ID_XT26G01A,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type19,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"XTX XT26G02A",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_XTX,
		dev_id:					_SPI_NAND_DEVICE_ID_XT26G02A,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type19,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* Mira */
	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_MIRA,
		dev_id:					_SPI_NAND_DEVICE_ID_PSU1GS20BN,
		name:				"MIRA PSU1GS20BN",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type6,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* BIWIN */
	{
		name:				"BIWIN BWJX08U",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_BIWIN,
		dev_id:					_SPI_NAND_DEVICE_ID_BWJX08U,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type15,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"BIWIN BWET08U",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_BIWIN,
		dev_id:					_SPI_NAND_DEVICE_ID_BWET08U,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type10,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	/* FORESEE */
	{
		name:				"FORESEE FS35ND01GD1F1",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND01GD1F1,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"FORESEE FS35ND01GS1F1",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND01GS1F1,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"FORESEE FS35ND02GS2F1",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND02GS2F1,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"FORESEE FS35ND02GD1F1",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND02GD1F1,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_type1,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		name:				"FORESEE FS35ND02G-S3Y2",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND02GS3Y2,
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_foresee,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		name:				"FORESEE FS35ND04G-S2Y2",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND04GS2Y2,
		device_size:				SPI_NAND_DEVICE_SIZE_4GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_foresee,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		name:				"FS35ND01G-S1Y2",
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_FORESEE,
		dev_id:					_SPI_NAND_DEVICE_ID_FS35ND01GS1Y2,
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_foresee,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_DS,
		dev_id: 				_SPI_NAND_DEVICE_ID_DS35Q2GA,
		name:				"DS DS35Q2GA",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_ds,
		feature:				SPI_NAND_FLASH_PLANE_SELECT_HAVE,
	},
	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_DS,
		dev_id: 				_SPI_NAND_DEVICE_ID_DS35Q1GA,
		name:				"DS DS35Q1GA",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_ds,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_FISON,
		dev_id: 				_SPI_NAND_DEVICE_ID_CS11G0T0A0AA,
		name:				"FISON CS11G0T0A0AA",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fison,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_FISON,
		dev_id: 				_SPI_NAND_DEVICE_ID_CS11G1T0A0AA,
		name:				"FISON CS11G1T0A0AA",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fison,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_FISON,
		dev_id: 				_SPI_NAND_DEVICE_ID_CS11G0G0A0AA,
		name:				"FISON CS11G0G0A0AA",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_fison,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},

	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_TYM,
		dev_id: 				_SPI_NAND_DEVICE_ID_TYM25D2GA01,
		name:				"TYM TYM25D2GA01",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_tym,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_TYM,
		dev_id: 				_SPI_NAND_DEVICE_ID_TYM25D2GA02,
		name:				"TYM TYM25D2GA02",
		device_size:				SPI_NAND_DEVICE_SIZE_2GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_tym,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	{
		mfr_id: 				_SPI_NAND_MANUFACTURER_ID_TYM,
		dev_id: 				_SPI_NAND_DEVICE_ID_TYM25D1GA03,
		name:				"TYM TYM25D1GA03",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_64BYTE,
		erase_size: 				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode: 				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode: 				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_tym,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
	// Xincun
	{
		mfr_id:					_SPI_NAND_MANUFACTURER_ID_XINCUN,
		dev_id:					_SPI_NAND_DEVICE_ID_XCSP1AAWHNT,
		name:				"XINCUN XCSP1AAWH-NT",
		device_size:				SPI_NAND_DEVICE_SIZE_1GBIT,
		page_size:				_SPI_NAND_PAGE_SIZE_2KBYTE,
		oob_size:				_SPI_NAND_OOB_SIZE_128BYTE,
		erase_size:				SPI_NAND_BLOCK_SIZE_128KBYTE,
		dummy_mode:				SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
		read_mode:				SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
		write_mode:				SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE,
		oob_free_layout:			&ooblayout_xincun,
		feature:				SPI_NAND_FLASH_FEATURE_NONE,
	},
};

int spi_nand_flash_foreach(int (*cb)(const struct spi_nand_id *flash_info, void *data), void *data)
{
	for (int i = 0; i < array_size(spi_nand_flash_tables); i++) {
		int ret = cb(&spi_nand_flash_tables[i], data);
		if (ret)
			return ret;
	}

	return 0;
}
