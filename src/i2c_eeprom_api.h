//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 McMCC <mcmcc@mail.ru>
 * i2c_eeprom_api.h
 */
#ifndef __I2C_EEPROM_API_H__
#define __I2C_EEPROM_API_H__

#include <ui.h>
#include <stdint.h>

#include "i2c_eeprom_ids.h"
#include "spi_flash.h"

int i2c_eeprom_init(const struct i2c_controller *i2c_controller, struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline);
int i2c_eeprom_read(const struct flash_cntx *cntx, uint8_t *buf, uint32_t from, uint32_t len);
int i2c_eeprom_erase(const struct flash_cntx *cntx, uint32_t offs, uint32_t len);
int i2c_eeprom_write(const struct flash_cntx *cntx, uint8_t *buf, uint32_t to, uint32_t len);
void i2c_eeprom_for_each(void (*cb)(const struct i2c_eeprom_type *eeprom));
const struct i2c_eeprom_type *i2c_eeprom_by_name(const char *name);

#endif /* __I2C_EEPROM_API_H__ */
