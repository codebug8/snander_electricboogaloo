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

#include "i2c_controller_local.h"

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

void i2c_controller_for_each(void (*cb)(const struct i2c_controller *i2c_controller))
{
	assert(cb);

	for (int i = 0; i < array_size(controllers); i++)
		cb(controllers[i]);
}
