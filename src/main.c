//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Daniel Palmer <daniel@thingy.jp>
 * Copyright (C) 2018-2021 McMCC <mcmcc@mail.ru>
 */

#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <dgputil.h>
#include <spi_controller.h>

#include "main.h"
#include "file.h"
#include "flash.h"
#include "spi_flash.h"
#include "ui.h"

#define TAG "main"
#define main_dbg(fmt, ...) ui_printf(LOGLEVEL_DBG, TAG, fmt, ##__VA_ARGS__)
#define main_info(fmt, ...) ui_printf(LOGLEVEL_INFO, TAG, fmt, ##__VA_ARGS__)
#define main_err(fmt, ...) ui_printf(LOGLEVEL_ERR, TAG, fmt, ##__VA_ARGS__)

#ifdef CONFIG_NEED_I2C
#include <i2c_controller.h>
#include "i2c_controller_local.h"
#endif

#ifdef CONFIG_EEPROM
#include "i2c_eeprom_api.h"
#endif

static bool killed = false;

static int do_identify(const struct flash_cntx *flash)
{
	return flash_identify(flash);
}

static void fill_addr_len(
		const struct flash_cntx *flash,
		const struct ui_parsed_cmdline *cmdline,
		uint32_t *addr, uint32_t *len)
{
	*addr = 0;

	if (cmdline->have_address)
		*addr = cmdline->address;

	if (cmdline->have_len)
		*len = cmdline->len;
	else {
		*len = flash->org.device_size - *addr;
	}
}

static int do_unlock(const struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline)
{
	uint32_t addr, len;
	int ret;

	fill_addr_len(flash, cmdline, &addr, &len);

	printf("Unlocking 0x%08x -> 0x%08x\n", addr, len);
	flash_mark_unlock(flash, addr, len);
	ui_print_flash_status(flash->status);

	ret = flash_unlock(flash, addr, len);

	printf("New lock status\n");
	ui_print_flash_status(flash->status);

	return 0;
}

static int do_lock(const struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline)
{
	uint32_t addr, len;
	int ret;

	fill_addr_len(flash, cmdline, &addr, &len);

	printf("Locking 0x%08x -> 0x%08x\n", addr, len);
	flash_mark_lock(flash, addr, len);
	ui_print_flash_status(flash->status);

	ret = flash_lock(flash, addr, len);

	printf("New lock status\n");
	ui_print_flash_status(flash->status);

	return 0;
}

static int do_erase(const struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline)
{
	const struct flash_org *org = &flash->org;
	uint32_t addr = 0, len;
	int ret;

	fill_addr_len(flash, cmdline, &addr, &len);

	if (len % org->block_size) {
		printf("Please set len = 0x%016llX multiple of the block size 0x%08X\n", len, org->block_size);
		return -EINVAL;
	}

	ui_op_start_erase(addr, len);
	ret = flash_erase(flash, addr, len);
	ui_statusbar_erasedone(len, len);
	ui_op_status(ret);

	return ret;
}

static int do_read(const struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline)
{
	uint32_t addr = 0, len;
	struct mmapped_file file;
	int ret;

	if (cmdline->have_len) {
		len = cmdline->len;
	}
	else
		len = flash->org.device_size;

	ui_op_start_read(addr, len);

	ret = file_open_and_mmap(cmdline->file, len, &file, true);
	if (ret)
		return ret;

	ret = flash_read(flash, file.map, addr, len);
	ui_op_status(ret);
	file_unmap_and_close(&file);

	return ret;
}

static int do_write(const struct flash_cntx *flash, const struct ui_parsed_cmdline *cmdline)
{
	struct mmapped_file file;
	uint32_t addr = 0, len;
	int ret;

	if (cmdline->have_address)
		addr = cmdline->address;

	if (cmdline->have_len)
		len = cmdline->len;
	else {
		ret = file_len(cmdline->file, &len);
		if (ret)
			return ret;
	}

	ret = file_open_and_mmap(cmdline->file, len, &file, false);
	if (ret)
		return ret;

	ui_op_start_write(addr, len);

	ret = flash_write(flash, file.map, addr, len);
	ui_op_status(ret);

	if(!ret && cmdline->verify) {
		ret = flash_verify(flash, file.map, addr, len);
		ui_op_status(ret);
	}

	file_unmap_and_close(&file);

	return ret;
}

#ifdef CONFIG_EEPROM
static const struct i2c_controller *i2c_controller = NULL;
#endif
static const struct spi_controller *spi_controller = NULL;

static void cleanup(void)
{
#ifdef CONFIG_EEPROM
	if (i2c_controller)
		i2c_controller_shutdown(i2c_controller);
#endif

	if (spi_controller)
		spi_controller_shutdown(spi_controller);
}

bool should_abort(void)
{
	return killed;
}

static void sigterm(int sig)
{
	printf("We got killed, aborting..\n");

	killed = true;
}

static int init_eeprom(struct flash_cntx *flash, struct ui_parsed_cmdline *cmdline)
{
#ifdef CONFIG_EEPROM
	int ret;

	if (cmdline->eepromid) {
		i2c_controller = i2c_controller_by_name(cmdline->programmer);
		if (i2c_controller == NULL) {
			main_err("Failed to find i2c controller for EEPROM operation\n");
			return -ENODEV;
		}

		ret = i2c_controller_init(i2c_controller, ui_printf, NULL);
		if (ret) {
			main_err("Failed to init i2c controller for EEPROM operation: %d\n", ret);
			return ret;
		}

		ret = i2c_eeprom_init(i2c_controller, flash, cmdline);
		if (ret) {
			main_err("Failed to init i2c EEPROM for EEPROM operation: %d\n", ret);
			return ret;
		}

		main_dbg("Ready for EEPROM operation\n");
		return 0;
	}

	return -ENODEV;
#else
	return -ENODEV;
#endif
}
int main(int argc, char** argv)
{
	int ret;
	struct ui_parsed_cmdline cmdline;

	signal(SIGINT, sigterm);

	struct flash_cntx flash = { 0 };


	ui_title();

	ret = ui_handle_cmdline(argc, argv, &cmdline);
	if (ret || cmdline.action == UI_ACTION_NONE)
		return ret;

	/* Check for EEPROM */
	ret = init_eeprom(&flash, &cmdline);
	if (!ret)
		goto do_action;

	/* SPI mode */
	spi_controller = spi_controller_by_name(cmdline.programmer);
	if (spi_controller == NULL)
		return -ENODEV;

	if (spi_controller_init(spi_controller, ui_printf, cmdline.connstring) < 0) {
		printf("Programmer device not found!\n\n");
		return -1;
	}

	ret = spi_flash_init(spi_controller, &flash, &cmdline);
	if (ret) {
		printf("Failed to init spi flash: %d\n", ret);
		return ret;
	}

do_action:
	switch (cmdline.action) {
	case UI_ACTION_NONE:
		break;
	case UI_ACTION_IDENTIFY:
		ret = do_identify(&flash);
		break;
	case UI_ACTION_UNLOCK:
		ret = do_unlock(&flash, &cmdline);
		break;
	case UI_ACTION_LOCK:
		ret = do_lock(&flash, &cmdline);
		break;
	case UI_ACTION_ERASE:
		ret = do_erase(&flash, &cmdline);
		break;
	case UI_ACTION_READ:
		ret = do_read(&flash, &cmdline);
		break;
	case UI_ACTION_WRITE:
		ret = do_write(&flash, &cmdline);
		break;
	}

	cleanup();

	return 0;
}
