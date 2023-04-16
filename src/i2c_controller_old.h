//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __I2C_CONTROLLER_H__
#define __I2C_CONTROLLER_H__

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include <i2c_controller.h>

#include "util.h"

struct i2c_client {
	uint8_t addr;
	void *priv;
};

struct i2c_controller *i2c_controller_by_name(const char* name);
void i2c_controller_for_each(void (*cb)(const struct i2c_controller *i2c_controller));

static inline void i2c_controller_set_addr(const struct i2c_controller *i2c_controller, uint8_t addr)
{
	i2c_controller->client->addr = addr;

	if (i2c_controller->set_addr)
		i2c_controller->set_addr(i2c_controller, addr);
}

static inline uint8_t i2c_controller_get_addr(const struct i2c_controller *i2c_controller)
{
	assert(i2c_controller);

	return i2c_controller->client->addr;
}

static inline void i2c_controller_set_priv(const struct i2c_controller *i2c_controller, void *priv)
{
	assert(!i2c_controller->client->priv);

	i2c_controller->client->priv = priv;
}

static inline int i2c_controller_max_transfer(const struct i2c_controller *i2c_controller)
{
	assert(i2c_controller);

	if (i2c_controller->max_transfer)
		return i2c_controller->max_transfer(i2c_controller);

	return -ENOTSUP;
}

static inline void *i2c_controller_get_priv(const struct i2c_controller *i2c_controller)
{
	assert(i2c_controller->client->priv);

	return i2c_controller->client->priv;
}

static inline int i2c_controller_do_transaction(const struct i2c_controller *i2c_controller,
		struct i2c_rdwr_ioctl_data *i2c_data, int tries)
{
	int ret;

	assert(i2c_controller);

	for(; tries; tries--) {
		ret = i2c_controller->do_transaction(i2c_controller, i2c_data);
		if (ret <= 0) {
			printf("Error sending write command: %d, tries left %d\n", ret, tries);
			ret = -1;
		}
		else
			return 0;
	}

	return ret;
}

static inline int i2c_controller_shutdown(const struct i2c_controller *i2c_controller)
{
	int ret = 0;

	if (i2c_controller->shutdown)
		ret = i2c_controller->shutdown(i2c_controller);

	return ret;
}

static inline bool i2c_controller_can_mangle(const struct i2c_controller *i2c_controller)
{
	int ret;

	assert(i2c_controller);

	if (!i2c_controller->get_func)
		return false;

	ret = i2c_controller->get_func(i2c_controller);
	if (ret > 0)
		return ret & I2C_FUNC_NOSTART;

	return false;
}

#define I2C_CONTROLLER_CMD_RETRIES 1

static inline int i2c_controller_cmd_onebyone(const struct i2c_controller *i2c_controller, uint8_t *cmd, size_t cmdlen)
{
	struct i2c_rdwr_ioctl_data i2c_data = { 0 };
	int nummsgs = cmdlen + 1;
	struct i2c_msg *msgs;
	int ret;

	assert(i2c_controller);

	msgs = malloc(sizeof(*msgs) * nummsgs);
	if (!msgs)
		return -ENOMEM;

	memset(&i2c_data, 0, sizeof(i2c_data));
	memset(msgs, 0, sizeof(msgs) * nummsgs);

	i2c_data.nmsgs = nummsgs;
	i2c_data.msgs = msgs;

	/* For the first message just send the address */
	{
		struct i2c_msg *msg = &i2c_data.msgs[0];

		msg->addr = i2c_controller_get_addr(i2c_controller);
	}

	for (int i = 0; i < cmdlen; i++) {
		struct i2c_msg *msg = &i2c_data.msgs[i + 1];

		msg->addr = i2c_controller_get_addr(i2c_controller);
		msg->len = 1;
		msg->buf = &cmd[i];
		msg->flags = I2C_M_NOSTART;
	}

	ret = i2c_controller_do_transaction(i2c_controller, &i2c_data, I2C_CONTROLLER_CMD_RETRIES);

	return ret;
}

static inline int i2c_controller_cmd_simple(const struct i2c_controller *i2c_controller, uint8_t *cmd, size_t cmdlen)
{
	struct i2c_rdwr_ioctl_data i2c_data = { 0 };
	struct i2c_msg msg[1] = { 0 };
	int ret;

	i2c_data.nmsgs = array_size(msg);
	i2c_data.msgs = msg;
	i2c_data.msgs[0].addr = i2c_controller_get_addr(i2c_controller);
	i2c_data.msgs[0].len = cmdlen;
	i2c_data.msgs[0].buf = cmd;

	ret = i2c_controller_do_transaction(i2c_controller, &i2c_data, I2C_CONTROLLER_CMD_RETRIES);

	return ret;
}

static inline bool i2c_controller_does_not_stop_on_nak(const struct i2c_controller *i2c_controller)
{
	assert(i2c_controller);

	if (i2c_controller->does_not_stop_on_nak)
		return i2c_controller->does_not_stop_on_nak(i2c_controller);

	return false;
}
static inline int i2c_controller_cmd(const struct i2c_controller *i2c_controller, uint8_t *cmd, size_t cmdlen)
{
	bool doesnotstoponnak;

	assert(i2c_controller);

	if (i2c_controller_does_not_stop_on_nak(i2c_controller)) {
		return i2c_controller_cmd_onebyone(i2c_controller, cmd, cmdlen);
	}
	else
		return i2c_controller_cmd_simple(i2c_controller, cmd, cmdlen);
}

static inline int i2c_controller_write_then_read_simple(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf, size_t writesz, uint8_t* readbuf, size_t readsz)
{
	struct i2c_rdwr_ioctl_data i2c_data = { 0 };
	struct i2c_msg msg[2] = { 0 };

	i2c_data.nmsgs = array_size(msg);
	i2c_data.msgs = msg;
	i2c_data.msgs[0].addr = i2c_controller_get_addr(i2c_controller);
	i2c_data.msgs[0].buf = writebuf;
	i2c_data.msgs[0].len = writesz;
	i2c_data.msgs[0].flags = I2C_M_IGNORE_NAK;

	i2c_data.msgs[1].addr = i2c_controller_get_addr(i2c_controller);
	i2c_data.msgs[1].buf = readbuf;
	i2c_data.msgs[1].len = readsz;
	i2c_data.msgs[1].flags = I2C_M_RD;

	return i2c_controller_do_transaction(i2c_controller, &i2c_data, 10);
}

int i2c_controller_write_then_read(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf, size_t writesz, uint8_t* readbuf, size_t readsz);
int i2c_controller_write_then_write(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf0, size_t writesz0, uint8_t* writebuf1, size_t writesz1);
int i2c_controller_write_then_write_simple(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf0, size_t writesz0, uint8_t* writebuf1, size_t writesz1);

#endif
