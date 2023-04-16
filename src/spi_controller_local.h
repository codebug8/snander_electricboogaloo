//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __SPI_CONTROLLER_LOCAL_H__
#define __SPI_CONTROLLER_LOCAL_H__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include <spi_controller.h>

const struct spi_controller* spi_controller_by_name(const char* name);
void spi_controller_for_each(void (*cb)(const struct spi_controller *spi_controller));

#endif
