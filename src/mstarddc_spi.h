//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __MSTARDDC_SPI_H__
#define __MSTARDDC_SPI_H__

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

int mstarddc_spi_init(void);
int mstarddc_spi_shutdown(void);
int mstarddc_spi_send_command(unsigned int writecnt,
							  unsigned int readcnt,
							  const unsigned char *writearr,
							  unsigned char *readarr);
int mstarddc_enable_pins(bool enable);
int mstarddc_config_stream(unsigned int speed);

extern const struct spi_controller mstarddc_spictrl;

#endif /* __MSTARDDC_SPI_H__ */
