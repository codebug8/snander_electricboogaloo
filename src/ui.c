//SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <argtable2.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <dgputil.h>

#include "ui.h"
#include "timer.h"

#include "spi_controller.h"
#include "spi_flash.h"
#include "spi_nand_flash.h"
#include "spi_nor_flash.h"

#ifdef CONFIG_NEED_I2C
#include <i2c_controller.h>
#include "i2c_controller_local.h"
#endif

#ifdef CONFIG_EEPROM
#include "i2c_eeprom_api.h"
#endif

void ui_title(void)
{
	printf("snander_electricboogaloo - ~dgp 2023\n");
	printf("based on SNANDer by McMCC <mcmcc@mail.ru>\n");
	printf("\n");
}

#ifdef EEPROM_SUPPORT
#include "ch341a_i2c.h"
#include "bitbang_microwire.h"
extern char eepromname[12];
extern int eepromsize;
extern int mw_eepromsize;
extern int org;
#define EHELP   "                select Microwire EEPROM {93c06|93c16|93c46|93c56|93c66|93c76|93c86|93c96} (need SPI-to-MW adapter)\n" \
		" -8             set organization 8-bit for Microwire EEPROM(default 16-bit) and set jumper on SPI-to-MW adapter\n" \
		" -f <addr len>  set manual address size in bits for Microwire EEPROM(default auto)\n"
#else
#define EHELP	""
#endif

static const char *ui_yes_no(bool yes)
{
	return yes ? "yes" : "no";
}

static void ui_print_spi_controller(const struct spi_controller *spi_controller)
{
	bool needconnstr = spi_controller_need_connstring(spi_controller);

	printf("%s:\n"
		   "\tneeds connection string?: %s\n", spi_controller->name, ui_yes_no(needconnstr));
	if (needconnstr)
		printf("\t%s\n", spi_controller->connstring_format(spi_controller));
}

static void ui_print_spi_nand(const struct spi_nand_priv *flash_info)
{
	printf("%s\n", flash_info->name);
}

static void ui_print_spi_nor(const struct chip_info *flash_info)
{
	printf("%s\n", flash_info->name);
}

static void ui_print_spi_flash_help(void)
{
	printf("Supported SPI NAND parts\n");
	spi_nand_flash_foreach(ui_print_spi_nand);
	printf("\n");
	printf("Supported SPI NOR parts\n");
	spi_nor_flash_foreach(ui_print_spi_nor);
	printf("\n");
}

#ifdef CONFIG_NEED_I2C
static void ui_print_i2c_controller(const struct i2c_controller *i2c_controller)
{
	bool canmangle = i2c_controller_can_mangle(i2c_controller);

	printf("%s\n", i2c_controller->name);
	printf("\tcan mangle transfers to match device parameters (erase size etc)?: %s\n", ui_yes_no(canmangle));
}
#endif

#ifdef CONFIG_EEPROM
static void ui_print_i2c_eeprom(const struct i2c_eeprom_type *eeprom)
{
	printf("%s\t(size: %d,\tpage size: %d)\n", eeprom->name, eeprom->size, eeprom->page_size);
}

static void ui_print_i2c_eeprom_help(void)
{
	printf("Supported EEPROM types:\n");
	i2c_eeprom_for_each(ui_print_i2c_eeprom);
}
#endif

static void ui_programmer_help(void)
{
	printf("Possible programmers for SPI:\n");
	spi_controller_for_each(ui_print_spi_controller);
#ifdef CONFIG_NEED_I2C
	printf("\n");
	printf("Possible programmers for I2C:\n");
	i2c_controller_for_each(ui_print_i2c_controller);
#endif
}

int ui_handle_cmdline(int argc, char **argv, struct ui_parsed_cmdline *result)
{
	int ret = 0;

	struct arg_lit *help, *disableecc, *ignoreecc, *list, *verify;
	struct arg_str *programmer, *connection;
	struct arg_int *i2c_speed;
	struct arg_lit *id, *unlock, *lock, *erase, *read, *write;
	struct arg_int *len, *address;
	struct arg_file *file;
	struct arg_end *end;
#ifdef CONFIG_EEPROM
	struct arg_str *eepromid;
#endif
	bool isreadorwrite;
	bool operationcanhaveaddressandlen;

	assert(result);

	void *argtable[] = {
			/* help */
			help = arg_lit0("h", "help", "Display this help text"),
			list = arg_lit0("L", "listchips", "Print list of supported chips"),
			/* programmer */
			programmer = arg_str0("p", "programmer", "<name>", "Programmer to use"),
			connection = arg_str0("c", "connection", "<custom>", "Extra connection string to pass to the programmer"),
			i2c_speed = arg_int0("z", "i2c_frequency", "<kHz>", "Specify i2c frequency"),
			/* eeprom options */
#ifdef CONFIG_EEPROM
			eepromid = arg_str0("E", "eepromtype", "<eepromtype>", "select I2C EEPROM type, enables I2C EEPROM mode"),
#endif
			/* action */
			id = arg_lit0("i", "identify", "Read the device ID"),
			unlock = arg_lit0("u", "unlock", "Unlock protected areas of the device, defaults to whole device, set the address and length to unlock specific areas"),
			lock = arg_lit0("b", "lock", "Lock areas of the device, defaults to whole device, set the address and length to lock specific areas"),
			erase = arg_lit0("e", "erase", "Erase the device, defaults to full erase, set the address and length to do a partial erase"),
			read = arg_lit0("r", "read", "Read the device, defaults to full read, set the address and length to do a partial read"),
			write = arg_lit0("w", "write", "Write to the device, defaults to the size of the input file, set the address and length to do a partial write"),
			/* action modifiers */
			verify = arg_lit0("v", "verify", "Verify after write"),
			len = arg_int0("l", "length", "<bytes>", "Set the length of the operation, for a write this will default "
												  "to the size of the input, for erase or read this defaults to the "
												  "size of the device"),
			address = arg_int0("a", "address", "<bytes>", "Set the device starting address for the operations, defaults to 0"),
			disableecc = arg_lit0("d", "disableecc", "disable internal ECC, read and write will be in page + OOB units"),
			ignoreecc = arg_lit0("I", "ignoreecc", "ECC ignore errors"),
			/* the file */
			file = arg_file0("f", "file", "<file path>", "The file to read into or write out from"),
			end = arg_end(1),
	};

	ret = arg_parse(argc, argv, argtable);
	if (ret) {
		arg_print_errors(stdout, end, "xxx");
		ret = -EINVAL;
		goto out;
	}

	result->action = UI_ACTION_NONE;
	result->have_address = false;
	result->address = 0;
	result->have_len = false;
	result->len = 0;
	result->verify = false;
	result->ecc_disable = false;
	result->ecc_ignore = false;

	if (help->count > 0) {
		arg_print_syntax(stdout, argtable, "\n");
		arg_print_glossary(stdout, argtable, "  %-30s %s\n");
		printf("\n");
		ui_programmer_help();
		printf("\n");
#ifdef CONFIG_EEPROM
		printf("\n");
		ui_print_i2c_eeprom_help();
#endif
		goto out;
	}

	if (list->count > 0) {
		ui_print_spi_flash_help();
#ifdef CONFIG_EEPROM
		printf("\n");
		ui_print_i2c_eeprom_help();
#endif
		goto out;
	}

#ifdef CONFIG_EEPROM
	result->eepromid = NULL;
	/* If we have an EEPROM type check it.. */
	if (eepromid->count) {
		const char *eepromname = eepromid->sval[0];
		result->eepromid = i2c_eeprom_by_name(eepromname);
		if (result->eepromid) {
			printf("Selected EEPROM: ");
			ui_print_i2c_eeprom(result->eepromid);
		}
		else {
			printf("Unknown I2C EEPROM type: %s\n", eepromname);
			ret = -EINVAL;
			goto out;
		}
	}
#endif

	/* Check we have a programmer argument */
	if (programmer->count) {
		static struct spi_controller *spi_controller;
		const char *progname = programmer->sval[0];

#ifdef CONFIG_EEPROM
		if (result->eepromid) {
			if (!i2c_controller_by_name(progname)) {
				printf("Couldn't find I2C programmer called \"%s\"\n", progname);
				ret = -ENODEV;
				goto out;
			}
		}
		else {
#endif
			spi_controller = spi_controller_by_name(progname);

			if(!spi_controller) {
				printf("Couldn't find SPI programmer called \"%s\"\n", progname);
				ret = -ENODEV;
				goto out;
			}

			if (spi_controller_need_connstring(spi_controller)) {
				if(!connection->count) {
					printf("\"%s\" needs a connection string\n");
					ret = -ENODEV;
					goto out;
				}

				result->connstring = strdup(connection->sval[0]);
			}
#ifdef CONFIG_EEPROM
		}
#endif

		result->programmer = strdup(programmer->sval[0]);
	}
	else {
		printf("Please specify which programmer to use.\n");
		ui_programmer_help();
		ret = -EINVAL;
		goto out;
	}

	/* Check we only have one action */
	if ((id->count + unlock->count + lock->count + erase->count + read->count + write->count) != 1) {
		printf("Please at least one and only one of [id|erase|read|write].\n");
		ret = -EINVAL;
		goto out;
	}

	/* So we have an action, which? */
	if (id->count) {
#ifdef CONFIG_EEPROM
		if (result->eepromid) {
			printf("Identify is not valid for I2C EEPROM\n");
			ret = -EINVAL;
			goto out;
		}
#endif
		result->action = UI_ACTION_IDENTIFY;
	}
	else if (unlock->count)
		result->action = UI_ACTION_UNLOCK;
	else if (lock->count)
		result->action = UI_ACTION_LOCK;
	else if (erase->count)
		result->action = UI_ACTION_ERASE;
	else if (read->count)
		result->action = UI_ACTION_READ;
	else if (write->count)
		result->action = UI_ACTION_WRITE;

	/* Check options that are only valid for read*/
	if (ignoreecc->count) {
		if (result->action == UI_ACTION_READ)
			result->ecc_ignore = true;
		else {
			printf("ignore ECC is only valid for read\n");
			ret = -EINVAL;
			goto out;
		}
	}

	/* Check options that are only valid for write */
	if (verify->count) {
		if (result->action == UI_ACTION_WRITE)
			result->verify = true;
		else {
			printf("Verify is only valid for write\n");
			ret = -EINVAL;
			goto out;
		}
	}

	/* Check options that are valid for read or write only*/
	isreadorwrite = (result->action == UI_ACTION_READ) ||
			(result->action == UI_ACTION_WRITE);
	if (disableecc->count) {
		if (isreadorwrite)
			result->ecc_disable = true;
		else {
			printf("Disable ECC is only valid for read or write\n");
			ret = -EINVAL;
			goto out;
		}
	}

	if (file->count && !isreadorwrite) {
		printf("No file is needed for the operation being performed\n");
		ret = -EINVAL;
		goto out;
	}

	if (!file->count && isreadorwrite) {
		printf("A file is needed for read/write operations\n");
		ret = -EINVAL;
		goto out;
	}

	if (isreadorwrite) {
		result->file = strdup(file->filename[0]);
	}

	/* Check options that are valid for read, write or erase*/
	operationcanhaveaddressandlen =
			(result->action == UI_ACTION_UNLOCK) ||
			(result->action == UI_ACTION_LOCK) ||
			(result->action == UI_ACTION_READ) ||
			(result->action == UI_ACTION_WRITE) ||
			(result->action == UI_ACTION_ERASE);

	if (address->count) {
		if (operationcanhaveaddressandlen) {
			result->have_address = true;
			result->address = (uint32_t) address->ival[0];
		}
		else {
			printf("Address is not valid for this operation\n");
			ret = -EINVAL;
			goto out;
		}
	}

	if (len->count) {
		if (operationcanhaveaddressandlen) {
			result->have_len = true;
			result->len = (uint32_t) len->ival[0];
		}
		else {
			printf("Length is not valid for this operation\n");
			ret = -EINVAL;
			goto out;
		}
	}

out:
	arg_freetable(argtable, array_size(argtable));

	if (ret)
		printf("Failed to parse command line: %d\n", ret);

	return ret;
}

static int ui_percentage(uint32_t len, uint32_t transferred)
{
	int percent;

	assert(len);

	if (transferred) {
		int tmp0 = 100 * ((transferred) / 1024);
		int tmp1 = (len / 1024);
		if (tmp0 && tmp1)
			percent =  tmp0 / tmp1 ;
		else percent = 0;
	}
	else
		percent = 0;

	return percent;
}

void ui_op_start_erase(uint32_t addr, uint32_t len)
{
	printf("ERASE:\n");
	printf("Erase addr = 0x%08"PRIx32", len = 0x%08"PRIx32"\n", addr, len);
}

void ui_op_start_write(uint32_t addr, uint32_t len)
{
	printf("WRITE:\n");
	printf("Write addr = 0x%08"PRIx32", len = 0x%08"PRIx32"\n", addr, len);
}

void ui_statusbar_write(uint32_t len, uint32_t remainder)
{
	int txfred = len - remainder;
	int percentage = ui_percentage(len, txfred);

	printf("\rWritten %d%% [%u] of [%u] bytes (%d bps)",
			percentage,
			txfred,
			len,
			timer_txfr_speed(txfred));
	fflush(stdout);
}

void ui_statusbar_writedone(uint32_t len, uint32_t remainder)
{
	ui_statusbar_write(len, remainder);
	printf("\n");
}

void ui_op_start_read(uint32_t addr, uint32_t len)
{
	printf("READ:\n");
	printf("Read addr = 0x%08"PRIx32", len = 0x%08"PRIx32"\n", addr, len);
}

void ui_statusbar_read(uint32_t len, uint32_t remainder)
{
	int txfred = len - remainder;
	int percent = ui_percentage(len, txfred);

	printf("\rRead %d%% [%u] of [%u] bytes (%d bps)",
			percent,
			txfred,
			len,
			timer_txfr_speed(txfred));
	fflush(stdout);
}

void ui_statusbar_readdone(uint32_t len, uint32_t remainder)
{
	ui_statusbar_read(len, remainder);
	printf("\n");
}

void ui_statusbar_erase(uint32_t pos, uint32_t total)
{
	printf("\rErase %d%% [%u] of [%u] bytes (%d bps)",
			ui_percentage(total, pos),
			pos,
			total,
			timer_txfr_speed(pos));
	fflush(stdout);
}

void ui_statusbar_erasedone(uint32_t erase_len, uint32_t len)
{
	ui_statusbar_erase(erase_len, len);
	printf("\n");
}

void ui_statusbar_verify(uint32_t len, uint32_t remainder)
{
	int txfred = len - remainder;
	int percent = ui_percentage(len, txfred);

	printf("\rVerify %d%% [%u] of [%u] bytes (%d bps)",
			percent,
			txfred,
			len,
			timer_txfr_speed(txfred));
	fflush(stdout);
}

void ui_statusbar_verifydone(uint32_t len, uint32_t remainder)
{
	ui_statusbar_verify(len, remainder);
	printf("\n");
}

void ui_statusbar_fullchiperase(void)
{
	printf("Doing full chip erase, please wait..\n");
}

void ui_op_status(int rc)
{
	if (rc < 0)
		printf("Status: BAD(%d)\n", rc);
	else
		printf("Status: OK\n");
}

void ui_print_flash_status(struct flash_status *status)
{
	const int perrow = 8;

	printf("Flash status:\n");
	printf("u - block is unlocked, l - block is locked, +/- - lock is going to be added or removed\n");


	for (int i = 0; i < status->num_regions; i += perrow) {
		struct flash_region *start_region = &status->regions[i];
		printf("0x%08x:", start_region->addr_start);

		for (int j = 0; j < perrow; j++) {
			struct flash_region *region = &status->regions[i + j];
			char addremovech = ' ';

			if (region->locked && region->want_to_unlock)
				addremovech = '-';
			else if (!region->locked && region->want_to_lock)
				addremovech = '+';

			printf(" %c%c|", (region->locked ? 'l' : 'u'), addremovech);
			//printf("0x%08x - 0x%08x: %d\n", region->addr_start, region->addr_end, region->protected);
		}
		printf("\n");
	}
}
int ui_printf(int level, const char *tag, const char *fmt, ...)
{
	va_list(args);

	printf("%-14s:", tag);
	va_start(args, fmt);

	return vprintf(fmt, args);
}
