//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __I2C_EEPROM_IDS_H__
#define __I2C_EEPROM_IDS_H__

struct	i2c_eeprom_type {
	char *name;
	uint32_t size;
	uint16_t page_size;
	/* Length of addres in bytes */
	uint8_t addr_size;
	uint8_t i2c_addr_mask;
};

const static struct i2c_eeprom_type eepromlist[] = {
	{ "24c01",   128,     8,  1, 0x00 }, // 16 pages of 8 bytes each = 128 bytes
	{ "24c02",   256,     8,  1, 0x00 }, // 32 pages of 8 bytes each = 256 bytes
	{ "24c04",   512,    16,  1, 0x01 }, // 32 pages of 16 bytes each = 512 bytes
	{ "24c08",   1024,   16,  1, 0x03 }, // 64 pages of 16 bytes each = 1024 bytes
	{ "24c16",   2048,   16,  1, 0x07 }, // 128 pages of 16 bytes each = 2048 bytes
	{ "24c32",   4096,   32,  2, 0x00 }, // 32kbit = 4kbyte
	{ "24c64",   8192,   32,  2, 0x00 },
	{ "24c128",  16384,  64,  2, 0x00 },
	{ "24c256",  32768,  64,  2, 0x00 },
	{ "24c512",  65536,  128, 2, 0x00 },
	{ "24c1024", 131072, 128, 2, 0x01 },
};

#endif /* __I2C_EEPROM_IDS_H__ */
