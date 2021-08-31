//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 McMCC <mcmcc@mail.ru>
 * i2c_eeprom.c
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <dgputil.h>

#include "i2c_controller.h"
#include "i2c_eeprom_ids.h"
#include "spi_flash.h"
#include "timer.h"
#include "ui.h"

#define I2C_EEPROM_ADDR 0x50

int i2c_eeprom_read(const struct flash_cntx *cntx, uint8_t *buf, uint32_t from, uint32_t len)
{
	const struct i2c_controller *i2c_controller = cntx->i2c_controller;
	int tfrsz;

	assert(i2c_controller);

	if (len == 0)
		return -EINVAL;

	/*
	 * If the controller can mangle it can probably handle whatever size we want
	 * so go for the page size..
	 */
	if (i2c_controller_can_mangle(i2c_controller))
		tfrsz = cntx->org.block_size;
	/* otherwise use what the controller says it can handle */
	else
		tfrsz = i2c_controller_max_transfer(i2c_controller);

	timer_start();

	i2c_controller_set_addr(i2c_controller, I2C_EEPROM_ADDR);
	for (int off = from; off < from + len; off += tfrsz) {
		int remainder = (from + len) - off;
		uint8_t addr[] = { (off >> 8) & 0xff, off & 0xff };

		i2c_controller_write_then_read(i2c_controller, addr, sizeof(addr), buf + off, min(tfrsz, remainder));

		ui_statusbar_read(len, remainder);
	}
	//if (ch341readEEPROM(ch341a, pbuf, eepromsize, &eeprom_info) < 0) {
	//	printf("Couldnt read [%d] bytes from [%s] EEPROM address 0x%08lu\n", (int)len, eepromname, from);
	//	return -1;
	//}

	printf("Read [%d] bytes from [%s] EEPROM address 0x%08lu\n", (int)len, "xxxxx", from);
	timer_end();

	return (int)len;
}

static void i2c_eeprom_check_can_write_page(const struct flash_cntx *cntx)
{
	const struct i2c_controller *i2c_controller = cntx->i2c_controller;

	if (cntx->org.block_size > i2c_controller_max_transfer(i2c_controller)) {
		printf("WARNING: your i2c maximum transfer size is smaller than the block/erase/page size\n");
	}
}

int i2c_eeprom_erase(const struct flash_cntx *cntx, uint32_t from, uint32_t len)
{
	const struct i2c_controller *i2c_controller = cntx->i2c_controller;
	uint8_t buf[] = { 0xff };

	assert(i2c_controller);

	i2c_eeprom_check_can_write_page(cntx);

	if (len == 0)
		return -1;

	timer_start();

	i2c_controller_set_addr(i2c_controller, I2C_EEPROM_ADDR);
	for (int off = from; off < from + len; off += 1) {
		int remainder = (from + len) - off;
		uint8_t addr[] = { (off >> 8) & 0xff, off & 0xff };
		i2c_controller_write_then_write(i2c_controller, addr, sizeof(addr), buf, 1);
		ui_statusbar_erase(off - from, len);
	}

	printf("Erased [%d] bytes of [%s] EEPROM address 0x%08lu\n", (int)len, "xxx", from);
	timer_end();

	return 0;
}

int i2c_eeprom_write(const struct flash_cntx *cntx, uint8_t *buf, uint32_t to, uint32_t len)
{
	const struct i2c_controller *i2c_controller = cntx->i2c_controller;
	int txfr_size = cntx->org.block_size;

	assert(i2c_controller);

	i2c_eeprom_check_can_write_page(cntx);

	if (len == 0)
		return -1;

	timer_start();

	i2c_controller_set_addr(i2c_controller, I2C_EEPROM_ADDR);
	for (int off = to; off < to + len; off += txfr_size) {
		int remainder = (to + len) - off;
		uint8_t addr[] = { (off >> 8) & 0xff, off & 0xff };

		i2c_controller_write_then_write(i2c_controller, addr, sizeof(addr), buf + off, txfr_size);
		usleep(10000);

		ui_statusbar_write(len, remainder);
	}

	//if (to || len < eepromsize) {
		//if (ch341readEEPROM(ch341a, pbuf, eepromsize, &eeprom_info) < 0) {
		//	printf("Couldnt read [%d] bytes from [%s] EEPROM\n", (int)len, eepromname);
		//	return -1;
		//}
	//}
	//memcpy(pbuf + to, buf, len);

	//if(ch341writeEEPROM(ch341a, pbuf, eepromsize, &eeprom_info) < 0) {
	//	printf("Failed to write [%d] bytes of [%s] EEPROM address 0x%08lu\n", (int)len, eepromname, to);
	//	return -1;
	//}

	printf("Wrote [%d] bytes to [%s] EEPROM address 0x%08lu\n", (int)len, "xxxx", to);
	timer_end();

	return (int)len;
}

#if 0
int flash_i2c_init()
{
	int ret = -1;

	if ((eepromsize <= 0) && (mw_eepromsize <= 0)) {
	} else if ((eepromsize > 0) || (mw_eepromsize > 0)) {
		if ((eepromsize > 0) && (flen = i2c_init()) > 0) {

		} else if ((mw_eepromsize > 0) && (flen = mw_init()) > 0) {
			cmd->flash_erase = mw_eeprom_erase;
			cmd->flash_write = mw_eeprom_write;
			cmd->flash_read  = mw_eeprom_read;
		}
	}
	else
		printf("\nFlash" __EEPROM___ " not found!!!!\n\n");

	return ret;
}
#endif

static const struct flash_ops i2c_eeprom_flash_ops = {
	.erase = i2c_eeprom_erase,
	.write = i2c_eeprom_write,
	.read  = i2c_eeprom_read,
};

int i2c_eeprom_init(const struct i2c_controller *i2c_controller, struct flash_cntx *flash,
					const struct ui_parsed_cmdline *cmdline)
{
	flash->ops = &i2c_eeprom_flash_ops;
	flash->org.device_size = cmdline->eepromid->size;
	flash->org.block_size = cmdline->eepromid->page_size;
	flash->i2c_controller = i2c_controller;

	return 0;
}

void i2c_eeprom_for_each(void (*cb)(const struct i2c_eeprom_type *eeprom))
{
	for (int i = 0; i < array_size(eepromlist); i++)
		cb(&eepromlist[i]);
}

const struct i2c_eeprom_type *i2c_eeprom_by_name(const char *name)
{
	for (int i = 0; i < array_size(eepromlist); i++) {
		const struct i2c_eeprom_type *type = &eepromlist[i];

		if (strcmp(type->name, name) == 0)
			return type;
	}

#if 0
	else if ((mw_eepromsize = deviceSize_3wire(optarg)) > 0) {
		memset(eepromname, 0, sizeof(eepromname));
		strncpy(eepromname, optarg, 10);
		org = 1;
		if (len > mw_eepromsize) {
			printf("Error set size %lld, max size %d for EEPROM %s!!!\n", len, mw_eepromsize, eepromname);
			exit(0);
		}
	} else {
		printf("Unknown EEPROM chip %s!!!\n", optarg);
		exit(0);
	}
#endif

	return NULL;
}
