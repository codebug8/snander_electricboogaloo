//SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __SPI_NOR_IDS_H_
#define __SPI_NOR_IDS_H_

#include <stdint.h>

#include "flash.h"

struct chip_info {
	const char		*name;
	uint8_t			id;
	uint32_t		jedec_id;
	unsigned long	sector_size;
	unsigned int	n_sectors;
	char			addr4b;

	int (*fill_bp_status)(uint8_t status_reg, struct flash_status *flash_status);
	int (*set_bp_from_status)(uint8_t *status_reg, struct flash_status *flash_status);
};

extern const struct chip_info chips_data [];
int spi_nor_ids_num(void);

#endif
