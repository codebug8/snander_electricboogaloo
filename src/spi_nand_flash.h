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
#include "spi_nand_ids.h"

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
	SPI_NAND_FLASH_DEBUG_LEVEL_0 = 0,
	SPI_NAND_FLASH_DEBUG_LEVEL_1,
	SPI_NAND_FLASH_DEBUG_LEVEL_2,
	SPI_NAND_FLASH_DEBUG_LEVEL_DEF_NO
} SPI_NAND_FLASH_DEBUG_LEVEL_T;

int spi_nand_init(const struct spi_controller *spi_controller,
		struct flash_cntx *flash,
		const struct ui_parsed_cmdline *cmdline);

#endif /* ifndef __SPI_NAND_FLASH_H__ */
