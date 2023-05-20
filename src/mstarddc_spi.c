//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file was part of the flashrom project.
 *
 * Copyright (C) 2014 Alexandre Boeglin <alex@boeglin.org>
 * Copyright (C) 2021 Daniel Palmer <daniel@thingy.jp>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <dgputil.h>
#include <i2c_controller.h>
#include <spi_controller.h>

#include "ui.h"
#include "i2c_controller_local.h"

#define TAG "mstarddc"

#ifdef CONFIG_DEBUG_MSTAR_DDC
#define mstarddc_dbg(fmt, ...) ui_printf(LEVEL_DBG, TAG, fmt, ##__VA_ARGS__)
#else
#define mstarddc_dbg(fmt, ...)
#endif

struct mstarddc_spi_data {
	log_cb log_cb;
	uint8_t addr;
	const struct i2c_controller *i2c_controller;
	void *i2c_controller_priv;
	int doreset;
};

// MSTAR DDC Commands
#define MSTARDDC_SPI_WRITE	0x10
#define MSTARDDC_SPI_READ	0x11
#define MSTARDDC_SPI_END	0x12
#define MSTARDDC_SPI_STATUS 0x20
#define MSTARDDC_SPI_RESET	0x24

/* Returns 0 upon success, a negative number upon errors. */
static int mstarddc_spi_shutdown(const struct spi_controller *spi_controller, void *priv)
{
	struct mstarddc_spi_data *mstarddc_data = priv;

	assert(spi_controller);
	assert(mstarddc_data);

	i2c_controller_shutdown(mstarddc_data->i2c_controller,
				mstarddc_data->i2c_controller_priv);

	free(mstarddc_data);

	return 0;
}

// mmm! doesn't work, based on notes from the wiki page
static int mstarddc_spi_status(struct mstarddc_spi_data *mstarddc_data)
{;
	int ret;
	uint8_t cmd[] = { MSTARDDC_SPI_STATUS };
	uint8_t result[1];

	assert(mstarddc_data);

	do  {
		ret = i2c_controller_write_then_read(mstarddc_data->i2c_controller,
				mstarddc_data->addr, cmd, sizeof(cmd), result, sizeof(result),
				mstarddc_data->i2c_controller_priv);

		printf("status: %x\n", (unsigned) result[0]);
	} while (result[0] != 0x80 && result[0] != 0xc0);

	return ret;
}

/* Returns 0 upon success, a negative number upon errors. */
static int mstarddc_spi_send_command(const struct spi_controller *spi_controller,
		unsigned int writecnt,
		unsigned int readcnt,
		const unsigned char *writearr,
		unsigned char *readarr,
		void *priv)
{
	int ret = 0;
	struct mstarddc_spi_data *mstarddc_data = priv;

	assert(spi_controller);
	assert(mstarddc_data);

	mstarddc_dbg("Sending command, write: %d, read %d\n", writecnt, readcnt);

	if (!ret && writecnt) {
		size_t cmdsz = (writecnt + 1) * sizeof(uint8_t);
		uint8_t *cmd = malloc(cmdsz);

		if (!cmd) {
			printf("Error allocating memory: errno %d.\n", errno);
			ret = -ENOMEM;
		}
		else {
			cmd[0] = MSTARDDC_SPI_WRITE;
			memcpy(cmd + 1, writearr, writecnt);

			ret = i2c_controller_cmd(mstarddc_data->i2c_controller, mstarddc_data->addr,
				cmd, cmdsz, mstarddc_data->i2c_controller_priv);

			free(cmd);
		}
	}

	if (!ret && readcnt) {
		uint8_t cmd = MSTARDDC_SPI_READ;

		ret = i2c_controller_write_then_read(mstarddc_data->i2c_controller, mstarddc_data->addr,
				&cmd,1, readarr, readcnt, mstarddc_data->i2c_controller_priv);
	}

#ifdef MSTARDDC_DEBUG
	for (i = 0; i < writecnt; i++){
		if(i % 16 == 0)
			printf("w: ");
		printf("0x%02x ", writearr[i]);
		if((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
	for (i = 0; i < readcnt; i++){
		if(i % 16 == 0)
			printf("r: ");
		printf("0x%02x ", readarr[i]);
		if((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
#endif

//	if (!ret && (writecnt || readcnt)) {
//	}

	/* Do not reset if something went wrong, as it might prevent from
	 * retrying flashing. */
	if (ret != 0)
		mstarddc_data->doreset = 0;

	return ret;
}

static int mstarddc_spi_end_command(const struct spi_controller *spi_controller, void *priv)
{
	uint8_t cmd[] = {  MSTARDDC_SPI_END };
	struct mstarddc_spi_data *mstarddc_data = priv;

	assert(spi_controller);
	assert(mstarddc_data);

	mstarddc_dbg("Sending SPI end command\n");

	return i2c_controller_cmd(mstarddc_data->i2c_controller, mstarddc_data->addr,
		cmd, sizeof(cmd), mstarddc_data->i2c_controller_priv);
}

static int mstarddc_spi_enableisp(const struct spi_controller *spi_controller, void *spi_controller_priv)
{
	uint8_t cmd[5] = { 'M', 'S', 'T', 'A', 'R' };
	struct mstarddc_spi_data *mstarddc_data = spi_controller_priv;
	int ret;

	ret = i2c_controller_cmd(mstarddc_data->i2c_controller, mstarddc_data->addr,
		cmd, sizeof(cmd), mstarddc_data->i2c_controller_priv);

	if (ret)
		ret = mstarddc_spi_end_command(spi_controller, spi_controller_priv);

	return ret;
}

static int mstarddc_parse_connstr(const char *connection, int *i2c_addr, char **i2c_progname, char **i2c_connstr)
{
	char *addr, *name, *connstr = NULL, *tmp;

	addr = strdup(connection);
	tmp = strchr(addr, '@');
	if (!tmp) {
		printf("Couldn't find i2c addr");
		return -EINVAL;
	}
	*tmp = '\0';
	tmp++;
	name = strdup(tmp);

	/* Find the connection string for i2c part */
	tmp = strchr(name, ':');
	if (tmp) {
		*tmp = '\0';
		tmp++;
		connstr = strdup(tmp);
	}

	*i2c_addr = (int) strtol(addr, (char **)NULL, 16);
	*i2c_progname = name;
	*i2c_connstr = connstr;

	printf("addr: %x, int: %s, connstr: %s\n", *i2c_addr, name, connstr);

	return 0;
}

/* Returns 0 upon success, a negative number upon errors. */
static int mstarddc_spi_open(const struct spi_controller *spi_controller, log_cb log_cb, const char *connection, void **priv)
{
	int ret = 0;
	int mstarddc_addr;
	int mstarddc_doreset = 1;
	struct i2c_controller *i2c_controller;
	void *i2c_controller_priv;
	char *i2c_progname, *i2c_connstr;

	ret = mstarddc_parse_connstr(connection, &mstarddc_addr, &i2c_progname, &i2c_connstr);
	if (ret)
		return ret;

	i2c_controller = i2c_controller_by_name(i2c_progname);
	if (!i2c_controller)
		return -ENODEV;

	ret = i2c_controller_open(i2c_controller, log_cb, i2c_connstr, &i2c_controller_priv);
	if (ret)
		return ret;

	mstarddc_dbg("Will try to use device %s and address 0x%02x.\n", i2c_connstr, mstarddc_addr);

	// Get noreset=1 option from command-line
	//char *noreset = extract_programmer_param("noreset");
	//if (noreset != NULL && noreset[0] == '1')
		mstarddc_doreset = 0;
	//free(noreset);
	mstarddc_dbg("Info: Will %sreset the device at the end.\n", mstarddc_doreset ? "" : "NOT ");
	// Open device

#if 0
	if (write(mstarddc_fd, cmd, 5) < 0) {
		int enable_err = errno;
		uint8_t end_cmd = MSTARDDC_SPI_END;

		// Assume device is already in ISP mode, try to send END command
		if (write(mstarddc_fd, &end_cmd, 1) < 0) {
			printf("Error enabling ISP mode: errno %d & %d.\n"
				 "Please check that device (%s) and address (0x%02x) are correct.\n",
				 enable_err, errno, i2c_device, mstarddc_addr);
			ret = -1;
			goto out;
		}
	}
#endif

	struct mstarddc_spi_data *mstarddc_data = calloc(1, sizeof(*mstarddc_data));
	if (!mstarddc_data) {
		printf("Unable to allocate space for SPI master data\n");
		ret = -1;
		goto out;
	}

	mstarddc_data->i2c_controller = i2c_controller;
	mstarddc_data->i2c_controller_priv = i2c_controller_priv;
	mstarddc_data->doreset = mstarddc_doreset;
	mstarddc_data->addr = mstarddc_addr;

	*priv = mstarddc_data;

	// Enable ISP mode
	ret = mstarddc_spi_enableisp(spi_controller, *priv);

out:
	free(i2c_connstr);

	return ret;
}

static int mstarddc_spi_maxtransfer(const struct spi_controller *spi_controller, void *priv)
{
	struct mstarddc_spi_data *mstarddc_data = priv;

	assert(spi_controller);
	assert(mstarddc_data);

	if (mstarddc_data->i2c_controller) {
		if(i2c_controller_can_mangle(mstarddc_data->i2c_controller)) {
			return 2048;
		}
	}

	return 64;
}

static bool mstarddc_need_connstring(const struct spi_controller *spi_controller)
{
	return true;
}

static const char *mstarddc_connstring_format(const struct spi_controller *spi_controller)
{
	return "<i2c address>@<i2c programmer name>:<programmer connstring> i.e 49@i2cdev:/dev/i2c-10 or 49@ch341a";
}

const struct spi_controller mstarddc_spictrl = {
	.name = "mstarddc",
	.open = mstarddc_spi_open,
	.shutdown = mstarddc_spi_shutdown,
	.send_command = mstarddc_spi_send_command,
	.cs_release = mstarddc_spi_end_command,
	.max_transfer = mstarddc_spi_maxtransfer,
	.need_connstring = mstarddc_need_connstring,
	.connstring_format = mstarddc_connstring_format,
};
