//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018-2021 McMCC <mcmcc@mail.ru>
 * snorcmd_api.h
 */
#ifndef __SNORCMD_API_H__
#define __SNORCMD_API_H__

#include "spi_flash.h"
#include "spi_nor_ids.h"

int spi_nor_init(const struct spi_controller *spi_controller, struct flash_cntx *flash);
void spi_nor_flash_foreach(void (*cb)(const struct chip_info *flash_info));



#endif /* __SNORCMD_API_H__ */
