//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __UI_H__
#define __UI_H__

#include <stdint.h>
#include <stdbool.h>

#include "flash.h"
#include "i2c_eeprom_ids.h"

enum ui_action {
	UI_ACTION_NONE,
	UI_ACTION_IDENTIFY,
	UI_ACTION_UNLOCK,
	UI_ACTION_LOCK,
	UI_ACTION_ERASE,
	UI_ACTION_READ,
	UI_ACTION_WRITE
};

struct ui_parsed_cmdline {
	enum ui_action action;
	bool verify;

	const char *programmer;
	const char *connstring;

	bool have_address;
	uint32_t address;
	bool have_len;
	uint32_t len;

	bool ecc_disable;
	bool ecc_ignore;

	const char *file;
#ifdef CONFIG_EEPROM
	const struct i2c_eeprom_type *eepromid;
#endif
};

int ui_handle_cmdline( int argc, char **argv, struct ui_parsed_cmdline *result);
void ui_title(void);
void ui_usage(void);
void ui_op_start_erase(uint32_t addr, uint32_t len);
void ui_op_start_write(uint32_t addr, uint32_t len);
void ui_statusbar_write(uint32_t len, uint32_t remainder);
void ui_statusbar_writedone(uint32_t len, uint32_t remainder);
void ui_op_start_read(uint32_t addr, uint32_t len);
void ui_statusbar_read(uint32_t len, uint32_t remainder);
void ui_statusbar_readdone(uint32_t len, uint32_t remainder);
void ui_statusbar_erase(uint32_t pos, uint32_t total);
void ui_statusbar_erasedone(uint32_t erase_len, uint32_t len);
void ui_statusbar_fullchiperase(void);
void ui_statusbar_verify(uint32_t len, uint32_t remainder);
void ui_statusbar_verifydone(uint32_t len, uint32_t remainder);
void ui_op_status(int rc);
void ui_print_flash_status(struct flash_status *status);

int ui_printf(int level, const char *tag, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

#endif // __UI_H__
