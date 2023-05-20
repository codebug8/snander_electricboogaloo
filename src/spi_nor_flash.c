//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018-2021 McMCC <mcmcc@mail.ru>
 * spi_nor_flash.c
 */

#include "spi_nor_flash.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <dgputil.h>

#include "cmdbuff.h"
#include "spi_controller.h"
#include "types.h"
#include "timer.h"
#include "ui.h"
#include "main.h"

#include "spi_nor_ids.h"
#include "spi_nor_cmds.h"

#define TAG "spi_nor"

#ifdef CONFIG_DEBUG_SPI_NOR
#define spi_nor_dbg(fmt, ...) ui_printf(LOGLEVEL_DBG, TAG, fmt, ##__VA_ARGS__)
#else
#define spi_nor_dbg(fmt, ...)
#endif
#define spi_nor_info(fmt, ...) ui_printf(LOGLEVEL_INFO, TAG, fmt, ##__VA_ARGS__)
#define spi_nor_err(fmt, ...) ui_printf(LOGLEVEL_ERR, TAG, fmt, ##__VA_ARGS__)

static int spi_nor_wait_ready(const struct flash_cntx *cntx, int sleep_ms);
static int spi_nor_read_status_reg(const struct flash_cntx *cntx, u8 *val);
static int spi_nor_write_status_reg(const struct flash_cntx *cntx, u8 val);

#define PREAMBLE(_cntx) \
		const struct spi_controller *spi_controller __attribute__ ((unused)); \
		spi_controller = cntx->spi_controller; \
		assert(spi_controller); \
		void *spi_controller_priv __attribute__ ((unused)); \
		spi_controller_priv = cntx->spi_controller_priv; \
		const struct chip_info *spi_chip_info = flash_get_priv(_cntx); \
		assert(spi_chip_info)

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline void spi_nor_write_enable(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_WREN, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);
}

static inline void spi_nor_write_disable(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_WRDI, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int spi_nor_wait_ready(const struct flash_cntx *cntx, int sleep_ms)
{
	PREAMBLE(cntx);
	int count;
	int sr = 0;

	/*
	 * one chip guarantees max 5 msec wait here after page writes,
	 * but potentially three seconds (!) after page erase.
	 */
	for (count = 0; count < ((sleep_ms + 1) * 1000); count++) {
		if ((spi_nor_read_status_reg(cntx, (u8 *)&sr)) < 0)
			break;
		//| SR_WEL
		else if (!(sr & (SR_WIP | SR_EPE))) {
			return 0;
		}
		//fixme usleep(500);

		if (should_abort())
			return -EBUSY;
	}

	printf("%s: read_sr fail: %x\n", __func__, sr);
	return -1;
}

/*
 * read status register
 */
static int snor_read_rg(const struct flash_cntx *cntx, uint8_t code, uint8_t *val)
{
	PREAMBLE(cntx);
	int retval;

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, code, spi_controller_priv);
	retval = spi_controller_read(spi_controller, val, 1, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);
	if (retval) {
		printf("%s: ret: %x\n", __func__, retval);
		return -1;
	}

	return 0;
}

/*
 * write status register
 */
static int snor_write_rg(const struct flash_cntx *cntx, uint8_t code, uint8_t val)
{
	PREAMBLE(cntx);
	int retval;

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, code, spi_controller_priv);
	retval = spi_controller_write(spi_controller, &val, 1, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);
	if (retval) {
		printf("%s: ret: %x\n", __func__, retval);
		return -1;
	}

	return 0;
}

static int snor_4byte_mode(const struct flash_cntx *cntx, int enable)
{
	PREAMBLE(cntx);
	int retval;

	if (spi_nor_wait_ready(cntx, 1))
		return -1;

	if (spi_chip_info->id == 0x1) /* Spansion */
	{
		u8 br_cfn;
		u8 br = enable ? 0x81 : 0;

		snor_write_rg(cntx, OPCODE_BRWR, br);
		snor_read_rg(cntx, OPCODE_BRRD, &br_cfn);
		if (br_cfn != br) {
			printf("4B mode switch failed %s, 0x%02x, 0x%02x\n", enable ? "enable" : "disable" , br_cfn, br);
			return -1;
		}
	} else {
		u8 code = enable ? 0xb7 : 0xe9; /* B7: enter 4B, E9: exit 4B */

		spi_controller_cs_assert(spi_controller, spi_controller_priv);
		retval = spi_controller_write1(spi_controller, code, spi_controller_priv);
		spi_controller_cs_release(spi_controller, spi_controller_priv);
		if (retval) {
			printf("%s: ret: %x\n", __func__, retval);
			return -1;
		}
		if ((!enable) && (spi_chip_info->id == 0xef)) /* Winbond */
		{
			code = 0;
			spi_nor_write_enable(cntx);
			snor_write_rg(cntx, 0xc5, code);
		}
	}
	return 0;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int snor_erase_sector(const struct flash_cntx *cntx, unsigned long offset)
{
	PREAMBLE(cntx);

	spi_nor_dbg("%s: offset:%x\n", __func__, offset);

	/* Wait until finished previous write command. */
	if (spi_nor_wait_ready(cntx, 950))
		return -1;

	/* If we need 4 byte addresses, enable it */
	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 1);

	/* Send write enable, then erase commands. */
	spi_nor_write_enable(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);

	spi_controller_write1(spi_controller, OPCODE_SE, spi_controller_priv);
	if (spi_chip_info->addr4b)
		spi_controller_write1(spi_controller, (offset >> 24) & 0xff, spi_controller_priv);
	spi_controller_write1(spi_controller, (offset >> 16) & 0xff, spi_controller_priv);
	spi_controller_write1(spi_controller, (offset >> 8) & 0xff, spi_controller_priv);
	spi_controller_write1(spi_controller, offset & 0xff, spi_controller_priv);

	spi_controller_cs_release(spi_controller, spi_controller_priv);

	spi_nor_wait_ready(cntx, 950);

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 0);

	return 0;
}

static int spi_nor_full_chip_erase(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);

	/* Wait until finished previous write command. */
	if (spi_nor_wait_ready(cntx, 3))
		return -EBUSY;

	/* Send write enable, then erase commands. */
	spi_nor_write_enable(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_BE1, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	spi_nor_wait_ready(cntx, 950);
	spi_nor_write_disable(cntx);

	return 0;
}

/* read SPI flash device ID */
static int spi_nor_read_devid(const struct flash_cntx *cntx, uint8_t *rxbuf, int len)
{
	const struct spi_controller *spi_controller = cntx->spi_controller; \
	void *spi_controller_priv = cntx->spi_controller_priv; \
	int ret;

	assert(spi_controller);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);

	ret = spi_controller_write1(spi_controller, OPCODE_RDID, spi_controller_priv);
	if (ret)
		goto out;

	ret = spi_controller_read(spi_controller, rxbuf, len, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);

out:
	spi_controller_cs_release(spi_controller, spi_controller_priv);
	if (ret)
		spi_nor_dbg("%s: ret: %x\n", __func__, ret);

	return ret;
}

/* read status register */
static int spi_nor_read_status_reg(const struct flash_cntx *cntx, uint8_t *val)
{
	PREAMBLE(cntx);
	int ret = 0;

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_RDSR, spi_controller_priv);
	ret = spi_controller_read(spi_controller, val, 1, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	if (ret) {
		spi_nor_dbg("%s: ret: %x\n", __func__, ret);
		return ret;
	}

	return 0;
}

/* write status register */
static int spi_nor_write_status_reg(const struct flash_cntx *cntx, u8 val)
{
	PREAMBLE(cntx);
	int ret = 0;

	/* Write must be enabled before changing the nv bits in the status register.. */
	spi_nor_write_enable(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_WRSR, spi_controller_priv);
	ret = spi_controller_write(spi_controller, &val, 1, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	spi_nor_wait_ready(cntx, 1);

	if (ret) {
		spi_nor_dbg("%s: ret: %x\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct chip_info *spi_nor_probe(const struct flash_cntx *cntx)
{
	const struct chip_info *info = NULL, *match = NULL;
	uint8_t buf[5];
	uint32_t jedec, jedec_strip, weight;
	int ret;

	ret = spi_nor_read_devid(cntx, buf, 5);
	if (ret)
		return err_ptr(ret);

	jedec = (u32)((u32)(buf[1] << 24) | ((u32)buf[2] << 16) | ((u32)buf[3] <<8) | (u32)buf[4]);
	jedec_strip = jedec & 0xffff0000;

	spi_nor_info("spi device id: %x %x %x %x %x (%x)\n",
			buf[0], buf[1], buf[2], buf[3], buf[4], jedec);

	// FIXME, assign default as AT25D
	weight = 0xffffffff;
	match = &chips_data[0];
	for (int i = 0; i < spi_nor_ids_num(); i++) {
		info = &chips_data[i];
		if (info->id == buf[0]) {
			if ((info->jedec_id == jedec) || ((info->jedec_id & 0xffff0000) == jedec_strip)) {
				spi_nor_info("Detected SPI NOR Flash: %s, Flash Size: %ld MB\n",
						info->name, (info->sector_size * info->n_sectors) >> 20);
				return info;
			}

			if (weight > (info->jedec_id ^ jedec)) {
				weight = info->jedec_id ^ jedec;
				match = info;
			}
		}
	}
	printf("SPI NOR Flash Not Detected!\n");
	match = NULL; /* Not support JEDEC calculate info */

	return match;
}

static int spi_nor_erase(const struct flash_cntx *cntx, uint32_t offs, uint32_t len)
{
	PREAMBLE(cntx);
	int ret;

	spi_nor_dbg("erase: offs:%x len:%x\n", offs, len);

	/* sanity checks */
	assert(len != 0);

	timer_start();

	if(!offs && len == cntx->org.device_size)
	{
		ui_statusbar_fullchiperase();
		ret = spi_nor_full_chip_erase(cntx);
		goto out;
	}

	/* now erase those sectors */
	for (int i = 0; i < len; i+= spi_chip_info->sector_size) {
		ui_statusbar_erase(i, len);

		ret = snor_erase_sector(cntx, i);
		if (ret)
			break;

		if (should_abort())
			break;
	}

out:
	timer_end();

	return ret;
}

static int spi_nor_set_addr(const struct flash_cntx *cntx, uint32_t addr)
{
	PREAMBLE(cntx);
	CMDBUFF(cmd);

	assert(spi_chip_info);

	if (spi_chip_info->addr4b)
		cmdbuff_push(&cmd, (addr >> 24) & 0xff);
	cmdbuff_push(&cmd, (addr >> 16) & 0xff);
	cmdbuff_push(&cmd, (addr >> 8) & 0xff);
	cmdbuff_push(&cmd, addr & 0xff);

	spi_controller_write(spi_controller, cmd.buff, cmd.pos, 0, spi_controller_priv);

	return 0;
}

static int spi_nor_for_each_chunk(const struct flash_cntx *cntx,
		uint8_t *buf, uint32_t start, uint32_t len, uint32_t chunklen,
		int (*cb)(const struct flash_cntx *cntx, uint8_t *buf, uint32_t off, uint32_t len, uint32_t total, uint32_t remainder))
{
	int ret = 0;

	for (int i = 0; i < len; i += chunklen) {
		ret = cb(cntx, buf + i, start + i, chunklen, len, len - i);
		if (ret)
			break;
	}

	return ret;
}

static int spi_nor_read_chunk(const struct flash_cntx *cntx, uint8_t *buf,
		uint32_t off, uint32_t len, uint32_t total, uint32_t remainder)
{
	PREAMBLE(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_READ,  spi_controller_priv);
	spi_nor_set_addr(cntx, off);
	spi_controller_read(spi_controller, buf, len, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	ui_statusbar_read(total, remainder);

	if(should_abort())
		return -EBUSY;

	return 0;
}
static int spi_nor_read(const struct flash_cntx *cntx, uint8_t *buf, uint32_t from, uint32_t len)
{
	PREAMBLE(cntx);
	unsigned transfer_sz = 4096;

	spi_nor_dbg("read: from:%x len:%x \n", from, len);

	/* sanity checks */
	assert(len);

	timer_start();
	/* Wait till previous write/erase is done. */
	if (spi_nor_wait_ready(cntx, 1)) {
		/* REVISIT status return?? */
		return -1;
	}

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 1);

	spi_nor_for_each_chunk(cntx, buf, from, len, transfer_sz, spi_nor_read_chunk);

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 0);

	ui_statusbar_readdone(len, 0);
	timer_end();

	return len;
}

static int spi_nor_write_page(const struct flash_cntx *cntx, uint8_t *buf,
		uint32_t off, uint32_t len, uint32_t total, uint32_t remainder)
{
	PREAMBLE(cntx);

	int ret = 0;

	spi_nor_write_enable(cntx);

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_PP, spi_controller_priv);
	spi_nor_set_addr(cntx, off);
	ret = spi_controller_write(spi_controller, buf, len, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	if (!ret)
		ret = spi_nor_wait_ready(cntx, 1);

	ui_statusbar_write(total, remainder);

	if (should_abort())
		return -EBUSY;

	return ret;
}

static int spi_nor_write(const struct flash_cntx *cntx, uint8_t *buf, uint32_t to, uint32_t len)
{
	PREAMBLE(cntx);
	int rc = 0, retlen = 0;

	spi_nor_dbg("write: to:%x len:%x \n", to, len);

	/* sanity checks */
	assert(len);

	if (to + len > spi_chip_info->sector_size * spi_chip_info->n_sectors)
		return -1;

	timer_start();

	/* Wait until finished previous write command. */
	if (spi_nor_wait_ready(cntx, 2))
		return -EBUSY;

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 1);

	spi_nor_wait_ready(cntx, 3);

	spi_nor_for_each_chunk(cntx, buf, to, len, FLASH_PAGESIZE, spi_nor_write_page);

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 0);

	spi_nor_write_disable(cntx);

	ui_statusbar_writedone(len, 0);
	timer_end();

	return retlen;
}

static int spi_nor_verify_chunk(const struct flash_cntx *cntx, uint8_t *buf,
		uint32_t off, uint32_t len, uint32_t total, uint32_t remainder)
{
	PREAMBLE(cntx);
	uint8_t chkbuf[4096];

	spi_controller_cs_assert(spi_controller, spi_controller_priv);
	spi_controller_write1(spi_controller, OPCODE_READ, spi_controller_priv);
	spi_nor_set_addr(cntx, off);
	spi_controller_read(spi_controller, chkbuf, len, SPI_CONTROLLER_SPEED_SINGLE, spi_controller_priv);
	spi_controller_cs_release(spi_controller, spi_controller_priv);

	if(memcmp(buf, chkbuf, len) != 0)
		spi_nor_err("bad!!\n");

	ui_statusbar_verify(total, remainder);

	if(should_abort())
		return -EBUSY;

	return 0;
}

static int spi_nor_verify(const struct flash_cntx *cntx, const uint8_t *buf, uint32_t off, uint32_t len)
{
	PREAMBLE(cntx);
	unsigned transfer_sz = 4096;

	spi_nor_dbg("verify: from:%x len:%x \n", from, len);

	/* sanity checks */
	assert(len);

	timer_start();
	if (spi_nor_wait_ready(cntx, 1)) {
		return -1;
	}

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 1);

	spi_nor_for_each_chunk(cntx, buf, off, len, transfer_sz, spi_nor_verify_chunk);

	if (spi_chip_info->addr4b)
		snor_4byte_mode(cntx, 0);

	ui_statusbar_verifydone(len, 0);
	timer_end();

	return len;
}

static int spi_nor_identify(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);

	spi_nor_info("Device type: %s\n", spi_chip_info->name);
	spi_nor_info("page size: %d\n", (int) spi_chip_info->sector_size);

	ui_print_flash_status(cntx->status);

	return 0;
}

static int spi_nor_lockunlock(const struct flash_cntx *cntx,
	uint32_t offset, uint32_t len)
{
	PREAMBLE(cntx);

	if (!spi_chip_info->set_bp_from_status) {
		spi_nor_info("Sorry, can't lock/unlock, chip doesn't have call back to update lock bits\n");
		return -EPERM;
	}

	uint8_t status, new_status;

	spi_nor_read_status_reg(cntx, &status);
	spi_chip_info->set_bp_from_status(&status, cntx->status);
	spi_nor_write_status_reg(cntx, status);

	spi_nor_read_status_reg(cntx, &new_status);
	spi_nor_info("wanted status 0x%02x, have 0x%02x\n", status, new_status);
	spi_chip_info->fill_bp_status(new_status, cntx->status);

	return 0;
}

static int spi_nor_unlock(const struct flash_cntx *cntx, uint32_t offset, uint32_t len)
{
	return spi_nor_lockunlock(cntx, offset, len);
}

static int spi_nor_lock(const struct flash_cntx *cntx, uint32_t offset, uint32_t len)
{
	return spi_nor_lockunlock(cntx, offset, len);
}

static const struct flash_ops spi_nor_flash_ops = {
	.identify = spi_nor_identify,
	.unlock = spi_nor_unlock,
	.lock = spi_nor_lock,
	.erase = spi_nor_erase,
	.read  = spi_nor_read,
	.write = spi_nor_write,
	.verify = spi_nor_verify,
};

int spi_nor_init(struct flash_cntx *flash)
{
	const struct chip_info *spi_chip_info;

	spi_chip_info = spi_nor_probe(flash);

	if (!spi_chip_info)
		return -ENODEV;

	flash_set_priv(flash, spi_chip_info);
	flash->ops = &spi_nor_flash_ops;
	flash->org.device_size = spi_chip_info->sector_size * spi_chip_info->n_sectors;
	flash->org.block_size = spi_chip_info->sector_size;

	size_t statussz = flex_array_sz(flash->status, regions, spi_chip_info->n_sectors);
	flash->status = malloc(statussz);
	if (!flash->status)
		return -ENOMEM;

	memset(flash->status, 0, statussz);
	flash->status->num_regions = spi_chip_info->n_sectors;

	for (int i = 0; i < flash->status->num_regions; i++) {
		struct flash_region *region = &flash->status->regions[i];
		uint32_t addr_start = spi_chip_info->sector_size * i;
		uint32_t addr_end = (addr_start + spi_chip_info->sector_size) - 1;

		flash_region_init(region, addr_start, addr_end);
	}

	if (spi_chip_info->fill_bp_status) {
		uint8_t status;

		spi_nor_read_status_reg(flash, &status);
		spi_chip_info->fill_bp_status(status, flash->status);
	}

	return 0;
}

void spi_nor_flash_foreach(void (*cb)(const struct chip_info *flash_info))
{
	for (int i = 0; i < spi_nor_ids_num(); i++)
		cb(&chips_data[i]);
}
