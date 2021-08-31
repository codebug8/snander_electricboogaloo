//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#ifndef SRC_SPI_NAND_CMDS_H_
#define SRC_SPI_NAND_CMDS_H_

#include <dgputil.h>

/* SPI NAND Command Set */
#define _SPI_NAND_OP_GET_FEATURE			0x0F	/* Get Feature */
#define _SPI_NAND_OP_SET_FEATURE			0x1F	/* Set Feature */
#define _SPI_NAND_OP_PAGE_READ				0x13	/* Load page data into cache of SPI NAND chip */
#define _SPI_NAND_OP_READ_FROM_CACHE_SINGLE		0x03	/* Read data from cache of SPI NAND chip, single speed*/
#define _SPI_NAND_OP_READ_FROM_CACHE_DUAL		0x3B	/* Read data from cache of SPI NAND chip, dual speed*/
#define _SPI_NAND_OP_READ_FROM_CACHE_QUAD		0x6B	/* Read data from cache of SPI NAND chip, quad speed*/
#define _SPI_NAND_OP_WRITE_ENABLE			0x06	/* Enable write data to  SPI NAND chip */
#define _SPI_NAND_OP_WRITE_DISABLE			0x04	/* Reseting the Write Enable Latch (WEL) */
#define _SPI_NAND_OP_PROGRAM_LOAD_SINGLE		0x02	/* Write data into cache of SPI NAND chip with cache reset, single speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_QUAD			0x32	/* Write data into cache of SPI NAND chip with cache reset, quad speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_RAMDOM_SINGLE		0x84	/* Write data into cache of SPI NAND chip, single speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_RAMDON_QUAD		0x34	/* Write data into cache of SPI NAND chip, quad speed */

#define _SPI_NAND_OP_PROGRAM_EXECUTE			0x10	/* Write data from cache into SPI NAND chip */
#define _SPI_NAND_OP_READ_ID				0x9F	/* Read Manufacture ID and Device ID */
#define _SPI_NAND_OP_BLOCK_ERASE			0xD8	/* Erase Block */
#define _SPI_NAND_OP_RESET				0xFF	/* Reset */
#define _SPI_NAND_OP_DIE_SELECT				0xC2	/* Die Select */

/* SPI NAND register address of command set */
#define _SPI_NAND_ADDR_ECC				0x90	/* Address of ECC Config */
#define _SPI_NAND_ADDR_PROTECTION			0xA0	/* Address of protection */
#define _SPI_NAND_ADDR_FEATURE				0xB0	/* Address of feature */
#define _SPI_NAND_ADDR_STATUS				0xC0	/* Address of status */
#define _SPI_NAND_ADDR_FEATURE_4			0xD0	/* Address of status 4 */
#define _SPI_NAND_ADDR_STATUS_5				0xE0	/* Address of status 5 */
#define _SPI_NAND_ADDR_MANUFACTURE_ID			0x00	/* Address of Manufacture ID */
#define _SPI_NAND_ADDR_DEVICE_ID			0x01	/* Address of Device ID */

/* SPI NAND value of register address of command set */
#define _SPI_NAND_VAL_DISABLE_PROTECTION		0x0	/* Value for disable write protection */
#define _SPI_NAND_VAL_ENABLE_PROTECTION			0x38	/* Value for enable write protection */
#define _SPI_NAND_VAL_OIP				0x1	/* OIP = Operaton In Progress */
#define _SPI_NAND_VAL_ERASE_FAIL			0x4	/* E_FAIL = Erase Fail */
#define _SPI_NAND_VAL_PROGRAM_FAIL			0x8	/* P_FAIL = Program Fail */

/* SPI NAND Size Define */
#define _SPI_NAND_PAGE_SIZE_512				0x0200
#define _SPI_NAND_PAGE_SIZE_2KBYTE			0x0800
#define _SPI_NAND_PAGE_SIZE_4KBYTE			0x1000
#define _SPI_NAND_OOB_SIZE_64BYTE			0x40
#define _SPI_NAND_OOB_SIZE_120BYTE			0x78
#define _SPI_NAND_OOB_SIZE_128BYTE			0x80
#define _SPI_NAND_OOB_SIZE_256BYTE			0x100
#define _SPI_NAND_BLOCK_SIZE_128KBYTE			0x20000
#define _SPI_NAND_BLOCK_SIZE_256KBYTE			0x40000
#define _SPI_NAND_CHIP_SIZE_512MBIT			0x04000000
#define _SPI_NAND_CHIP_SIZE_1GBIT			0x08000000
#define _SPI_NAND_CHIP_SIZE_2GBIT			0x10000000
#define _SPI_NAND_CHIP_SIZE_4GBIT			0x20000000

/* Others Define */
#define _SPI_NAND_LEN_ONE_BYTE			(1)
#define _SPI_NAND_LEN_TWO_BYTE			(2)
#define _SPI_NAND_LEN_THREE_BYTE		(3)
#define _SPI_NAND_BLOCK_ROW_ADDRESS_OFFSET	(6)

#define _SPI_NAND_OOB_SIZE			256
#define _SPI_NAND_PAGE_SIZE			(4096 + _SPI_NAND_OOB_SIZE)
#define _SPI_NAND_CACHE_SIZE			(_SPI_NAND_PAGE_SIZE+_SPI_NAND_OOB_SIZE)

#define LINUX_USE_OOB_START_OFFSET		4
#define MAX_LINUX_USE_OOB_SIZE			26

#define EMPTY_DATA				(0)
#define NONE_EMPTY_DATA				(1)
#define EMPTY_OOB				(0)
#define NONE_EMPTY_OOB				(1)

#define SPI_NAND_STATUS2_OTPE			bit(6)
#define SPI_NAND_OTP_PAGE_UNIQUEID		0x0
#define SPI_NAND_UNIQUEID_LEN			32
#define SPI_NAND_OTP_PAGE_PARAMETERPAGE	0x1

static const uint8_t spi_nand_parameter_page_signature[] = { 0x4f, 0x4e, 0x46, 0x49 };

struct spi_nand_parameter_page {
	uint8_t	signature[4];
	uint8_t revision_number[2];
	uint8_t feature_supported[2];
	uint8_t optional_command_support[2];
	uint8_t reserved1[22];
	uint8_t device_manufacturer[12];
	uint8_t device_model[20];
	uint8_t jedec_manufacturer_id[1];
};

#endif /* SRC_SPI_NAND_CMDS_H_ */
