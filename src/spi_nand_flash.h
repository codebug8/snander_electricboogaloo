//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#ifndef __SPI_NAND_FLASH_H__
#define __SPI_NAND_FLASH_H__

#include <stddef.h>

#include "ui.h"
#include "flash.h"
#include "types.h"
#include "spi_controller.h"

#define SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX	32

typedef enum{
	SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND,
	SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
	SPI_NAND_FLASH_READ_DUMMY_BYTE_DEF_NO
} SPI_NAND_FLASH_READ_DUMMY_BYTE_T;

typedef enum{
	SPI_NAND_FLASH_RTN_NO_ERROR = 0,
	SPI_NAND_FLASH_RTN_PROBE_ERROR,
	SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL,
	SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK,
	SPI_NAND_FLASH_RTN_ERASE_FAIL,
	SPI_NAND_FLASH_RTN_PROGRAM_FAIL,
	SPI_NAND_FLASH_RTN_DEF_NO
} SPI_NAND_FLASH_RTN_T;

typedef enum{
	SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE = 0,
	SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
	SPI_NAND_FLASH_READ_SPEED_MODE_QUAD,
	SPI_NAND_FLASH_READ_SPEED_MODE_DEF_NO
} SPI_NAND_FLASH_READ_SPEED_MODE_T;


typedef enum{
	SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE = 0,
	SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD,
	SPI_NAND_FLASH_WRITE_SPEED_MODE_DEF_NO
} SPI_NAND_FLASH_WRITE_SPEED_MODE_T;


typedef enum{
	SPI_NAND_FLASH_DEBUG_LEVEL_0 = 0,
	SPI_NAND_FLASH_DEBUG_LEVEL_1,
	SPI_NAND_FLASH_DEBUG_LEVEL_2,
	SPI_NAND_FLASH_DEBUG_LEVEL_DEF_NO
} SPI_NAND_FLASH_DEBUG_LEVEL_T;

/* Bitwise */
#define SPI_NAND_FLASH_FEATURE_NONE		( 0x00 )
#define SPI_NAND_FLASH_PLANE_SELECT_HAVE	( 0x01 << 0 )
#define SPI_NAND_FLASH_DIE_SELECT_1_HAVE	( 0x01 << 1 )
#define SPI_NAND_FLASH_DIE_SELECT_2_HAVE	( 0x01 << 2 )

struct spi_nand_flash_oobfree{
	unsigned long offset;
	unsigned long len;
};

struct spi_nand_flash_ooblayout
{	unsigned long oobsize;
	struct spi_nand_flash_oobfree oobfree[SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX];
};

struct spi_nand_priv {
	/* Basic chip info */
	uint8_t	mfr_id;
	uint8_t dev_id;
	const char *name;

	/* Flash total Size */
	uint32_t device_size;
	uint32_t page_size;
	uint32_t erase_size;
	/* Spare Area (OOB) Size */
	uint32_t oob_size;

	/*
	 * This is either the size of the total data area
	 * or the size of the total data and oob area depending
	 * on the command line flags.
	 */
	uint32_t working_size;

	SPI_NAND_FLASH_READ_DUMMY_BYTE_T dummy_mode;
	SPI_NAND_FLASH_READ_SPEED_MODE_T read_mode;
	SPI_NAND_FLASH_WRITE_SPEED_MODE_T write_mode;
	struct spi_nand_flash_ooblayout *oob_free_layout;
	uint32_t feature;

	/* lock handling */
	int (*unlock)(void);

	/* ecc handling */
	bool ecc_disable;
	bool ecc_ignore;
	bool (*ecc_ok)(uint8_t status);

	/* This is a scratch buffer to use when working with pages */
	size_t page_buffer_sz;
	uint8_t *page_buffer;
};

int spi_nand_init(const struct spi_controller *spi_controller,
		struct flash_cntx *flash,
		const struct ui_parsed_cmdline *cmdline);
void spi_nand_flash_foreach(void (*cb)(const struct spi_nand_priv *flash_info));

#endif /* ifndef __SPI_NAND_FLASH_H__ */
