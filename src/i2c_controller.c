//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <dgputil.h>
#include <i2c_controller.h>

#ifdef CONFIG_CH341A
#include <libch341a.h>
#endif

#ifdef CONFIG_DEBUG_I2C
#define i2c_dbg(fmt, ...) ui_printf(TAG, fmt, ##__VA_ARGS__)
#else
#define i2c_dbg(fmt, ...)
#endif

static struct i2c_controller *controllers[] = {
	&i2cdev_i2c,
#ifdef CONFIG_CH341A
	&ch341a_i2c,
#endif
};

struct i2c_controller *i2c_controller_by_name(const char* name)
{
	for (int i = 0; i < array_size(controllers); i++) {
		if (strcmp(controllers[i]->name, name) == 0)
			return controllers[i];
	}

	return NULL;
}

static inline int i2c_controller_mangle_needed_messages(const struct i2c_controller *i2c_controller,
		size_t sz)
{
	int max_transfer = i2c_controller_max_transfer(i2c_controller);

	assert(sz);

	return (sz / max_transfer) + ((sz % max_transfer) ? 1 : 0);
}

static inline int i2c_controller_write_then_read_mangled(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf, size_t writesz, uint8_t* readbuf, size_t readsz)
{
	int msgs_for_first, msgs_for_second, msgs_total, max_transfer;
	struct i2c_rdwr_ioctl_data i2c_data;
	struct i2c_msg *msg;
	size_t remainder;
	int ret;

	assert(i2c_controller);

	msgs_for_first = i2c_controller_mangle_needed_messages(i2c_controller, writesz);
	msgs_for_second = i2c_controller_mangle_needed_messages(i2c_controller, readsz);
	msgs_total = msgs_for_first + msgs_for_second;
	max_transfer = i2c_controller_max_transfer(i2c_controller);

	msg = malloc(sizeof(*msg) * msgs_total);
	if (!msg)
		return -ENOMEM;

	memset(&i2c_data, 0, sizeof(i2c_data));
	memset(msg, 0, sizeof(msg) * msgs_total);

	i2c_data.nmsgs = msgs_total;
	i2c_data.msgs = msg;

	i2c_dbg("using %d + %d txfrs for mangled read, write %d, read %d\n",
			msgs_for_first, msgs_for_second, writesz, readsz);

	remainder = writesz;
	for(int i = 0; i < msgs_for_first; i++) {
		struct i2c_msg *msg = &i2c_data.msgs[i];

		msg->addr = i2c_controller_get_addr(i2c_controller);
		msg->buf = writebuf + (max_transfer * i);
		msg->len = min(remainder, max_transfer);
		if (i != 0)
			msg->flags = I2C_M_NOSTART;
		msg->flags |= I2C_M_IGNORE_NAK;
		remainder -= max_transfer;
	}

	remainder = readsz;
	for(int i = 0; i < msgs_for_second; i++) {
		struct i2c_msg *msg = &i2c_data.msgs[msgs_for_first + i];

		msg->addr = i2c_controller_get_addr(i2c_controller);
		msg->buf = readbuf + (max_transfer * i);
		msg->len = min(remainder, max_transfer);
		msg->flags = I2C_M_RD;
		if (i != 0)
			msg->flags |= I2C_M_NOSTART;
		remainder -= max_transfer;
	}

	ret = i2c_controller_do_transaction(i2c_controller, &i2c_data, 10);

	free(msg);

	return ret;
}

int i2c_controller_write_then_read(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf, size_t writesz, uint8_t* readbuf, size_t readsz)
{
	int max_transfer = i2c_controller_max_transfer(i2c_controller);

	if (writesz <= max_transfer && readsz <= max_transfer)
		return i2c_controller_write_then_read_simple(i2c_controller, writebuf, writesz, readbuf, readsz);
	else if (i2c_controller_can_mangle(i2c_controller))
		return i2c_controller_write_then_read_mangled(i2c_controller, writebuf, writesz, readbuf, readsz);
	else
		return -EINVAL;
}

/* Write two buffers using mangling to chop them up into multiple i2c messages */
static inline int i2c_controller_write_then_write_mangled(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf0, size_t writesz0, uint8_t* writebuf1, size_t writesz1)
{
	int msgs_for_first, msgs_for_second, msgs_total, max_transfer;
	struct i2c_rdwr_ioctl_data i2c_data;
	struct i2c_msg *msg;
	size_t remainder;
	int ret;

	assert(i2c_controller);

	msgs_for_first = i2c_controller_mangle_needed_messages(i2c_controller, writesz0);
	msgs_for_second = i2c_controller_mangle_needed_messages(i2c_controller, writesz1);
	msgs_total = msgs_for_first + msgs_for_second;
	max_transfer = i2c_controller_max_transfer(i2c_controller);

	msg = malloc(sizeof(*msg) * msgs_total);
	if (!msg)
		return -ENOMEM;

	memset(&i2c_data, 0, sizeof(i2c_data));
	memset(msg, 0, sizeof(msg) * msgs_total);

	i2c_data.nmsgs = msgs_total;
	i2c_data.msgs = msg;

	printf("using %d + %d txfrs\n", msgs_for_first, msgs_for_second);

	remainder = writesz0;
	for(int i = 0; i < msgs_for_first; i++) {
		struct i2c_msg *msg = &i2c_data.msgs[i];

		msg->addr = i2c_controller_get_addr(i2c_controller);
		msg->buf = writebuf0 + (max_transfer * i);
		msg->len = min(remainder, max_transfer);
		if (i != 0)
			msg->flags = I2C_M_NOSTART;
		remainder -= max_transfer;
	}

	remainder = writesz1;
	for(int i = 0; i < msgs_for_second; i++) {
		struct i2c_msg *msg = &i2c_data.msgs[msgs_for_first + i];

		msg->addr = i2c_controller_get_addr(i2c_controller);
		msg->buf = writebuf1 + (max_transfer * i);
		msg->len = min(remainder, max_transfer);
		msg->flags = I2C_M_NOSTART;
		remainder -= max_transfer;
	}

	ret = i2c_controller_do_transaction(i2c_controller, &i2c_data, 10);

	free(msg);

	return ret;
}

int i2c_controller_write_then_write(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf0, size_t writesz0, uint8_t* writebuf1, size_t writesz1)
{
	int max_transfer = i2c_controller_max_transfer(i2c_controller);

	assert(writebuf0);
	assert(writebuf1);

	if ((writesz0 + writesz1) <= max_transfer)
		return i2c_controller_write_then_write_simple(i2c_controller, writebuf0, writesz0, writebuf1, writesz1);
	else if (i2c_controller_can_mangle(i2c_controller))
		return i2c_controller_write_then_write_mangled(i2c_controller, writebuf0, writesz0, writebuf1, writesz1);
	else
		return -EINVAL;
}

/* Simple write for two buffers, allocate some memory, concat and send */
int i2c_controller_write_then_write_simple(const struct i2c_controller *i2c_controller,
		uint8_t *writebuf0, size_t writesz0, uint8_t* writebuf1, size_t writesz1)
{
	struct i2c_rdwr_ioctl_data i2c_data;
	struct i2c_msg msg[1];
	int totalsize = writesz0 + writesz1;
	uint8_t *combinedbuffer;
	int ret;

	combinedbuffer = malloc(totalsize);
	if (!combinedbuffer)
		return -ENOMEM;

	memcpy(combinedbuffer, writebuf0, writesz0);
	memcpy(combinedbuffer + writesz0, writebuf1, writesz1);

	memset(&i2c_data, 0, sizeof(i2c_data));
	memset(&msg, 0, sizeof(msg));

	i2c_data.nmsgs = array_size(msg);
	i2c_data.msgs = msg;
	i2c_data.msgs[0].addr = i2c_controller_get_addr(i2c_controller);
	i2c_data.msgs[0].buf = combinedbuffer;
	i2c_data.msgs[0].len = totalsize;

	ret = i2c_controller_do_transaction(i2c_controller, &i2c_data, 10);

	free(combinedbuffer);

	return ret;
}

void i2c_controller_for_each(void (*cb)(const struct i2c_controller *i2c_controller))
{
	assert(cb);

	for (int i = 0; i < array_size(controllers); i++)
		cb(controllers[i]);
}
