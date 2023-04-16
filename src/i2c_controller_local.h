//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __I2C_CONTROLLER_LOCAL_H__
#define __I2C_CONTROLLER_LOCAL_H__

#include <i2c_controller.h>

struct i2c_controller *i2c_controller_by_name(const char* name);
void i2c_controller_for_each(void (*cb)(const struct i2c_controller *i2c_controller));

#endif
