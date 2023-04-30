//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#include <stddef.h>
#include <string.h>

#include <dgputil.h>
#include <spi_controller.h>

#ifdef CONFIG_CH341A
#include <libch341a.h>
#endif

#ifdef CONFIG_MSTAR_DDC
#include "mstarddc_spi.h"
#endif

static const struct spi_controller *spi_controllers[] = {
#ifdef CONFIG_CH341A
	&ch341a_spi,
#endif
#ifdef CONFIG_MSTAR_DDC
	&mstarddc_spictrl,
#endif
};

const struct spi_controller* spi_controller_by_name(const char* name)
{
	for (int i = 0; i < array_size(spi_controllers); i++) {
		if(strcmp(spi_controllers[i]->name, name) == 0) {
			return spi_controllers[i];
		}
	}

	return NULL;
}

void spi_controller_for_each(void (*cb)(const struct spi_controller *spi_controller))
{
	assert(cb);

	for (int i = 0; i < array_size(spi_controllers); i++)
		cb(spi_controllers[i]);
}
