//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018-2021 McMCC <mcmcc@mail.ru>
 */
#ifndef __FLASHCMD_API_H__
#define __FLASHCMD_API_H__

#include "ui.h"
#include "flash.h"
#include "spi_controller.h"

int spi_flash_init(const struct spi_controller *spi_controller,
		   void *spi_controller_priv,
		   struct flash_cntx *flash_cntx,
		   const struct ui_parsed_cmdline *cmdline);

#endif /* __FLASHCMD_API_H__ */
