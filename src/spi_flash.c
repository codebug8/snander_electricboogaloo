//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018-2021 McMCC <mcmcc@mail.ru>
 */

#include "ui.h"
#include "spi_flash.h"
#include "spi_nand_flash.h"
#include "spi_nor_flash.h"

int spi_flash_init(const struct spi_controller *spi_controller,
		struct flash_cntx *flash_cntx,
		const struct ui_parsed_cmdline *cmdline)
{
	assert(spi_controller);
	assert(flash_cntx);
	assert(cmdline);

	flash_cntx->spi_controller = spi_controller;

	int ret = spi_nand_init(spi_controller, flash_cntx, cmdline);
	if (!ret)
		return 0;

	ret = spi_nor_init(spi_controller, flash_cntx);
	if (!ret)
		return 0;

	return ret;
}
