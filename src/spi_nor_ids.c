//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * spi_nor_ids.c
 */

#include <dgputil.h>

#include "spi_nor_ids.h"

static int spi_nor_bp_bit2_to_bit5_top_down(uint8_t status_reg, struct flash_status *flash_status)
{
	int protected_regions;
	int bp = (status_reg >> 2) & 0xf;

	switch(bp) {
	case 0:
		protected_regions = 0;
		break;
	case 0b1000:
		protected_regions = (flash_status->num_regions / 2);
		break;
	case 0b111:
		protected_regions = (flash_status->num_regions / 4);
		break;
	case 0b110:
		protected_regions = (flash_status->num_regions / 8);
		break;
	case 0b101:
		protected_regions = (flash_status->num_regions / 16);
		break;
	case 0b100:
		protected_regions = (flash_status->num_regions / 32);
		break;
	case 0b11:
		protected_regions = (flash_status->num_regions / 64);
		break;
	case 0b10:
		protected_regions = (flash_status->num_regions / 128);
		break;
	case 0b1:
		protected_regions = (flash_status->num_regions / 256);
		break;
	/* all protected */
	default:
		protected_regions = flash_status->num_regions;
		break;
	}

	/* Reset all regions */
	for (int i = 0; i < flash_status->num_regions; i++) {
		struct flash_region *region = &flash_status->regions[i];

		region->locked = false;
		region->lockable = true;
	}

	/* Mark the protected ones */
	for (int i = (flash_status->num_regions - 1); i >= 0 && protected_regions > 0; i--, protected_regions--) {
		struct flash_region *region = &flash_status->regions[i];

		region->locked = true;
	}

	return 0;
}

static int spi_nor_set_bit2_to_bit5_top_down(uint8_t *status_reg, struct flash_status *flash_status)
{
	int lock = 0;
	int unlock = 0;

	bool seen_locked = false;

	for (int i = 0; i < flash_status->num_regions; i++) {
		struct flash_region *region = &flash_status->regions[i];

		if ((region->locked && !region->want_to_unlock) || region->want_to_lock) {
			seen_locked = true;
			lock++;
		}
		else if ((!region->locked && !region->want_to_lock) || region->want_to_unlock) {
			/* We can only lock a contiguous region */
			if (seen_locked)
				return -EINVAL;
			unlock++;
		}
	}

	/* lock the whole array */
	if (lock == flash_status->num_regions)
		*status_reg |= 0x3c;
	else if (unlock == flash_status->num_regions)
		*status_reg &= ~0x3c;

	return 0;
}

const struct chip_info chips_data [] = {
	/* REVISIT: fill in JEDEC ids, for parts that have them */
	{ "AT25DF321",		0x1f, 0x47000000, 64 * 1024, 64,  0 },
	{ "AT26DF161",		0x1f, 0x46000000, 64 * 1024, 32,  0 },

	{ "F25L016",		0x8c, 0x21150000, 64 * 1024, 32,  0 }, //ESMT
	{ "F25L16QA",		0x8c, 0x41158c41, 64 * 1024, 32,  0 },
	{ "F25L032",		0x8c, 0x21160000, 64 * 1024, 64,  0 },
	{ "F25L32QA",		0x8c, 0x41168c41, 64 * 1024, 64,  0 },
	{ "F25L064",		0x8c, 0x21170000, 64 * 1024, 128, 0 },
	{ "F25L64QA",		0x8c, 0x41170000, 64 * 1024, 128, 0 },

	{ "GD25Q16",		0xc8, 0x40150000, 64 * 1024, 32,  0 },
	{ "GD25Q32",		0xc8, 0x40160000, 64 * 1024, 64,  0 },
	{ "GD25Q64CSIG",	0xc8, 0x4017c840, 64 * 1024, 128, 0 },
	{ "GD25Q128CSIG",	0xc8, 0x4018c840, 64 * 1024, 256, 0 },
	{ "GD25Q256CSIG",	0xc8, 0x4019c840, 64 * 1024, 512, 1 },

	{ "MX25L1605D",		0xc2, 0x2015c220, 64 * 1024, 32,  0 },
	{ "MX25L3205D",		0xc2, 0x2016c220, 64 * 1024, 64,  0 },
	{ "MX25L6405D",		0xc2, 0x2017c220, 64 * 1024, 128, 0 },
	{
		.name = "MX25L12805D",
		.id = 0xc2,
		.jedec_id = 0x2018c220,
		.sector_size = 64 * 1024,
		.n_sectors = 256,

		.fill_bp_status = spi_nor_bp_bit2_to_bit5_top_down,
		.set_bp_from_status = spi_nor_set_bit2_to_bit5_top_down,
	},
	{ "MX25L25635E",	0xc2, 0x2019c220, 64 * 1024, 512, 1 },
	{ "MX25L51245G",	0xc2, 0x201ac220, 64 * 1024, 1024, 1 },

	{ "FL016AIF",		0x01, 0x02140000, 64 * 1024, 32,  0 },
	{ "FL064AIF",		0x01, 0x02160000, 64 * 1024, 128, 0 },
	{ "S25FL032P",		0x01, 0x02154D00, 64 * 1024, 64,  0 },
	{ "S25FL064P",		0x01, 0x02164D00, 64 * 1024, 128, 0 },
	{ "S25FL128P",		0x01, 0x20180301, 64 * 1024, 256, 0 },
	{ "S25FL129P",		0x01, 0x20184D01, 64 * 1024, 256, 0 },
	{ "S25FL256S",		0x01, 0x02194D01, 64 * 1024, 512, 1 },
	{ "S25FL116K",		0x01, 0x40150140, 64 * 1024, 32,  0 },
	{ "S25FL132K",		0x01, 0x40160140, 64 * 1024, 64,  0 },
	{ "S25FL164K",		0x01, 0x40170140, 64 * 1024, 128, 0 },

	{ "EN25F16",		0x1c, 0x31151c31, 64 * 1024, 32,  0 },
	{ "EN25Q16",		0x1c, 0x30151c30, 64 * 1024, 32,  0 },
	{ "EN25QH16",		0x1c, 0x70151c70, 64 * 1024, 32,  0 },
	{ "EN25Q32B",		0x1c, 0x30161c30, 64 * 1024, 64,  0 },
	{ "EN25F32",		0x1c, 0x31161c31, 64 * 1024, 64,  0 },
	{ "EN25F64",		0x1c, 0x20171c20, 64 * 1024, 128, 0 },
	{ "EN25Q64",		0x1c, 0x30171c30, 64 * 1024, 128, 0 },
	{ "EN25QA64A",		0x1c, 0x60170000, 64 * 1024, 128, 0 },
	{ "EN25QH64A",		0x1c, 0x70171c70, 64 * 1024, 128, 0 },
	{ "EN25Q128",		0x1c, 0x30181c30, 64 * 1024, 256, 0 },
	{ "EN25QA128A",		0x1c, 0x60180000, 64 * 1024, 256, 0 },
	{ "EN25QH128A",		0x1c, 0x70181c70, 64 * 1024, 256, 0 },

	{ "W25X05",		0xef, 0x30100000, 64 * 1024, 1,   0 },
	{ "W25X10",		0xef, 0x30110000, 64 * 1024, 2,   0 },
	{ "W25X20",		0xef, 0x30120000, 64 * 1024, 4,   0 },
	{ "W25X40",		0xef, 0x30130000, 64 * 1024, 8,   0 },
	{ "W25X80",		0xef, 0x30140000, 64 * 1024, 16,  0 },
	{ "W25X16",		0xef, 0x30150000, 64 * 1024, 32,  0 },
	{ "W25X32VS",		0xef, 0x30160000, 64 * 1024, 64,  0 },
	{ "W25X64",		0xef, 0x30170000, 64 * 1024, 128, 0 },
	{ "W25Q20CL",		0xef, 0x40120000, 64 * 1024, 4,   0 },
	{ "W25Q20BW",		0xef, 0x50120000, 64 * 1024, 4,   0 },
	{ "W25Q20EW",		0xef, 0x60120000, 64 * 1024, 4,   0 },
	{ "W25Q80",		0xef, 0x50140000, 64 * 1024, 16,  0 },
	{ "W25Q80BL",		0xef, 0x40140000, 64 * 1024, 16,  0 },
	{ "W25Q16JQ",		0xef, 0x40150000, 64 * 1024, 32,  0 },
	{ "W25Q16JM",		0xef, 0x70150000, 64 * 1024, 32,  0 },
	{ "W25Q32BV",		0xef, 0x40160000, 64 * 1024, 64,  0 },
	{ "W25Q32DW",		0xef, 0x60160000, 64 * 1024, 64,  0 },
	{ "W25Q64BV",		0xef, 0x40170000, 64 * 1024, 128, 0 },
	{ "W25Q64DW",		0xef, 0x60170000, 64 * 1024, 128, 0 },
	{ "W25Q128BV",		0xef, 0x40180000, 64 * 1024, 256, 0 },
	{ "W25Q128FW",		0xef, 0x60180000, 64 * 1024, 256, 0 },
	{ "W25Q256FV",		0xef, 0x40190000, 64 * 1024, 512, 1 },
	{ "W25Q512JV",		0xef, 0x71190000, 64 * 1024, 1024, 1 },

	{ "M25P016",		0x20, 0x20150000, 64 * 1024, 32,  0 },
	{ "N25Q032A",		0x20, 0xba161000, 64 * 1024, 64,  0 },
	{ "N25Q064A",		0x20, 0xba171000, 64 * 1024, 128, 0 },
	{ "M25P128",		0x20, 0x20180000, 64 * 1024, 256, 0 },
	{ "N25Q128A",		0x20, 0xba181000, 64 * 1024, 256, 0 },
	{ "XM25QH32B",		0x20, 0x40160000, 64 * 1024, 64,  0 },
	{ "XM25QH32A",		0x20, 0x70160000, 64 * 1024, 64,  0 },
	{ "XM25QH64A",		0x20, 0x70170000, 64 * 1024, 128, 0 },
	{ "XM25QH128A",		0x20, 0x70182070, 64 * 1024, 256, 0 },
	{ "N25Q256A",		0x20, 0xba191000, 64 * 1024, 512, 1 },
	{ "MT25QL512AB",	0x20, 0xba201044, 64 * 1024, 1024, 1 },

	{ "ZB25VQ16",		0x5e, 0x40150000, 64 * 1024, 32,  0 },
	{ "ZB25VQ32",		0x5e, 0x40160000, 64 * 1024, 64,  0 },
	{ "ZB25VQ64",		0x5e, 0x40170000, 64 * 1024, 128, 0 },
	{ "ZB25VQ128",		0x5e, 0x40180000, 64 * 1024, 256, 0 },

	{ "BY25Q16BS",		0x68, 0x40150000, 64 * 1024, 32,  0 },
	{ "BY25Q32BS",		0x68, 0x40160000, 64 * 1024, 64,  0 },
	{ "BY25Q64AS",		0x68, 0x40170000, 64 * 1024, 128, 0 },
	{ "BY25Q128AS",		0x68, 0x40180000, 64 * 1024, 256, 0 },

	{ "XT25F32B",		0x0b, 0x40150000, 64 * 1024, 32,  0 },
	{ "XT25F32B",		0x0b, 0x40160000, 64 * 1024, 64,  0 },
	{ "XT25F64B",		0x0b, 0x40170000, 64 * 1024, 128, 0 },
	{ "XT25F128B",		0x0b, 0x40180000, 64 * 1024, 256, 0 },

	{ "PM25LQ016",		0x7f, 0x9d450000, 64 * 1024, 32,  0 },
	{ "PM25LQ032",		0x7f, 0x9d460000, 64 * 1024, 64,  0 },
	{ "PM25LQ064",		0x7f, 0x9d470000, 64 * 1024, 128, 0 },
	{ "PM25LQ128",		0x7f, 0x9d480000, 64 * 1024, 256, 0 },

	{ "IC25LP016",		0x9d, 0x60150000, 64 * 1024, 32,  0 },
	{ "IC25LP032",		0x9d, 0x60160000, 64 * 1024, 64,  0 },
	{ "IC25LP064",		0x9d, 0x60170000, 64 * 1024, 128, 0 },
	{ "IC25LP128",		0x9d, 0x60180000, 64 * 1024, 256, 0 },

	{ "FS25Q016",		0xa1, 0x40150000, 64 * 1024, 32,  0 },
	{ "FS25Q032",		0xa1, 0x40160000, 64 * 1024, 64,  0 },
	{ "FS25Q064",		0xa1, 0x40170000, 64 * 1024, 128, 0 },
	{ "FS25Q128",		0xa1, 0x40180000, 64 * 1024, 256, 0 },
	{ "FM25W16",		0xa1, 0x28150000, 64 * 1024, 32,  0 },
	{ "FM25W32",		0xa1, 0x28160000, 64 * 1024, 64,  0 },
	{ "FM25W64",		0xa1, 0x28170000, 64 * 1024, 128, 0 },
	{ "FM25W128",		0xa1, 0x28180000, 64 * 1024, 256, 0 },

	{ "FM25Q16A",		0xf8, 0x32150000, 64 * 1024, 32,  0 },
	{ "FM25Q32A",		0xf8, 0x32160000, 64 * 1024, 64,  0 },
	{ "FM25Q64A",		0xf8, 0x32170000, 64 * 1024, 128, 0 },
	{ "FM25Q128A",		0xf8, 0x32180000, 64 * 1024, 256, 0 },

	{ "PN25F16",		0xe0, 0x40150000, 64 * 1024, 32,  0 },
	{ "PN25F32",		0xe0, 0x40160000, 64 * 1024, 64,  0 },
	{ "PN25F64",		0xe0, 0x40170000, 64 * 1024, 128, 0 },
	{ "PN25F128",		0xe0, 0x40180000, 64 * 1024, 256, 0 },

	{ "P25Q16H",		0x85, 0x60150000, 64 * 1024, 32,  0 },
	{ "P25Q32H",		0x85, 0x60160000, 64 * 1024, 64,  0 },
	{ "P25Q64H",		0x85, 0x60170000, 64 * 1024, 128, 0 },
	{ "P25Q128H",		0x85, 0x60180000, 64 * 1024, 256, 0 },
};

int spi_nor_ids_num(void)
{
	return array_size(chips_data);
}
