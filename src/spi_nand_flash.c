//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <dgputil.h>

#include "types.h"
#include "spi_nand_flash.h"
#include "spi_controller.h"
#include "spi_nand_cmds.h"
#include "timer.h"
#include "ui.h"
#include "cmdbuff.h"
#include "main.h"

#define TAG "spi_nand"

#ifdef CONFIG_DEBUG_SPI_NAND
#define spi_nand_dbg(fmt, ...) ui_printf(LOGLEVEL_DGB, TAG, fmt, ##__VA_ARGS__)
#else
#define spi_nand_dbg(fmt, ...)
#endif
#define spi_nand_info(fmt, ...) ui_printf(LOGLEVEL_INFO, TAG, fmt, ##__VA_ARGS__)
#define spi_nand_err(fmt, ...) ui_printf(LOGLEVEL_ERR, TAG, fmt, ##__VA_ARGS__)

//#define CONFIG_SPI_NAND_TRACE

#ifdef CONFIG_SPI_NAND_TRACE
#define spi_nand_trace(...)			\
({						\
	printf("%s:%d: ", __func__, __LINE__);	\
	printf(__VA_ARGS__);			\
})
#else
#define spi_nand_trace(...)
#endif

#define _SPI_NAND_BLOCK_ALIGNED_CHECK( __addr__,__block_size__) ((__addr__) & ( __block_size__ - 1))

/* Porting Replacement */
#define _SPI_NAND_PRINTF			printf	/* Always print information */
#if !defined(SPI_NAND_FLASH_DEBUG)
#define _SPI_NAND_DEBUG_PRINTF(args...)
#define _SPI_NAND_DEBUG_PRINTF_ARRAY(args...)
#else
#define _SPI_NAND_DEBUG_PRINTF(level, args...)		printf(args)	/* spi_nand_flash_debug_printf */
#define _SPI_NAND_DEBUG_PRINTF_ARRAY(level, args...)	printf(args)	/* spi_nand_flash_debug_printf_array */
#endif

static unsigned char _plane_select_bit = 0;
static unsigned char _die_id = 0;
int en_oob_write = 0;
int en_oob_erase = 0;
unsigned char _ondie_ecc_flag = 1;    /* Ondie ECC : [ToDo :  Init this flag base on diffrent chip ?] */

#define IMAGE_OOB_SIZE				64	/* fix 64 oob buffer size padding after page buffer, no hw ecc info */
#define PAGE_OOB_SIZE				64	/* 64 bytes for 2K page, 128 bytes for 4k page */

/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
static unsigned long bmt_oob_size = 64;
static uint32_t erase_oob_size = 0;
static uint32_t ecc_size = 0;

#include "spi_nand_ids.h"

#if 0
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_reset(const struct spi_controller *spi_controller)
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Send FFh opcode (Reset) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_RESET);

	/* 3. Chip Select High */
	spi_controller_cs_release(spi_controller);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_reset\n");

	return (rtn_status);
}
#endif

static int spi_nand_protocol_set_feature(const struct spi_controller *spi_controller, uint8_t addr, uint8_t data)
{
	CMDBUFF(cmd);

	/* Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* Send 0Fh opcode (Set Feature) */
	cmdbuff_push(&cmd, _SPI_NAND_OP_SET_FEATURE);

	/* Offset addr */
	cmdbuff_push(&cmd, addr);

	/* Write new setting */
	cmdbuff_push(&cmd, data);

	spi_controller_write(spi_controller, cmd.buff, cmd.pos, SPI_CONTROLLER_SPEED_SINGLE);

	/* Chip Select High */
	spi_controller_cs_release(spi_controller);

	spi_nand_dbg("set_feature %x: val = 0x%x\n", addr, data);

	return 0;
}

static int spi_nand_protocol_get_feature(const struct spi_controller *spi_controller, uint8_t addr, uint8_t *ptr_rtn_data)
{
	CMDBUFF(cmd);

	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Send 0Fh opcode (Get Feature) */
	cmdbuff_push(&cmd, _SPI_NAND_OP_GET_FEATURE);

	/* 3. Offset addr */
	cmdbuff_push(&cmd, addr);

	spi_controller_write(spi_controller, cmd.buff, cmd.pos, SPI_CONTROLLER_SPEED_SINGLE);

	/* 4. Read 1 byte data */
	spi_controller_read(spi_controller, ptr_rtn_data, _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);

	/* 5. Chip Select High */
	spi_controller_cs_release(spi_controller);

	spi_nand_dbg("get_feature %x: val = 0x%x\n", addr, *ptr_rtn_data);

	return 0;
}

static inline int spi_nand_protocol_set_status_reg_1(const struct spi_controller *spi_controller,
		uint8_t protection)
{
	/* Offset addr of protection (0xA0) */
	return spi_nand_protocol_set_feature(spi_controller, _SPI_NAND_ADDR_PROTECTION, protection);
}

static inline int spi_nand_protocol_get_status_reg_1(const struct spi_controller *spi_controller,
		uint8_t *ptr_rtn_protection )
{
	/* Offset addr of protection (0xA0) */
	return spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_PROTECTION, ptr_rtn_protection);
}

static inline int spi_nand_protocol_set_status_reg_2(const struct spi_controller *spi_controller,
		uint8_t feature)
{
	/* Offset addr of feature (0xB0) */
	return spi_nand_protocol_set_feature(spi_controller,_SPI_NAND_ADDR_FEATURE, feature);
}

static inline int spi_nand_protocol_get_status_reg_2(const struct spi_controller *spi_controller,
		uint8_t *value)
{
	/* Offset addr of protection (0xB0) */
	return spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_FEATURE, value);
}

static inline int spi_nand_protocol_get_status_reg_3(const struct spi_controller *spi_controller,
		uint8_t *ptr_rtn_status)
{
	/* Offset addr of status (0xC0) */
	return spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_STATUS, ptr_rtn_status);
}

static inline int spi_nand_protocol_set_status_reg_4(const struct spi_controller *spi_controller,
		uint8_t feature)
{
	/* Offset addr of feature 4 (0xD0) */
	return spi_nand_protocol_set_feature(spi_controller,_SPI_NAND_ADDR_FEATURE_4, feature);
}

static inline int spi_nand_protocol_get_status_reg_4(const struct spi_controller *spi_controller,
		uint8_t *ptr_rtn_status)
{
	/* Offset addr of feature 4 (0xD0) */
	return spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_FEATURE_4, ptr_rtn_status);
}

static inline int spi_nand_protocol_get_status_reg_5(const struct spi_controller *spi_controller,
		uint8_t *ptr_rtn_status)
{
	/* Offset addr of status 5 (0xE0)) */
	return spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_STATUS_5, ptr_rtn_status);
}

static int spi_nand_protocol_write_enable(const struct spi_controller *spi_controller)
{
	spi_nand_trace("enabling write\n");

	/* Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* Write op_cmd 0x06 (Write Enable (WREN)*/
	spi_controller_write1(spi_controller, _SPI_NAND_OP_WRITE_ENABLE);

	/* Chip Select High */
	spi_controller_cs_release(spi_controller);

	return 0;
}

static int spi_nand_protocol_write_disable(const struct spi_controller *spi_controller)
{
	/* Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* Write op_cmd 0x04 (Write Disable (WRDI)*/
	spi_controller_write1(spi_controller, _SPI_NAND_OP_WRITE_DISABLE);

	/* Chip Select High */
	spi_controller_cs_release(spi_controller);

	return 0;
}

static int spi_nand_protocol_block_erase(const struct spi_controller *spi_controller,
		uint32_t block_idx )
{
	/* Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* Write op_cmd 0xD8 (Block Erase) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_BLOCK_ERASE );

	/* Write block number */
	block_idx = block_idx << _SPI_NAND_BLOCK_ROW_ADDRESS_OFFSET;	/*Row Address format in SPI NAND chip */

	spi_nand_dbg("erase : block idx = 0x%x\n", block_idx);

	/* dummy byte */
	spi_controller_write1(spi_controller, (block_idx >> 16) & 0xff );
	spi_controller_write1(spi_controller, (block_idx >> 8)  & 0xff );
	spi_controller_write1(spi_controller, block_idx & 0xff );

	/* Chip Select High */
	spi_controller_cs_release(spi_controller);

	return 0;
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id (const struct spi_controller *spi_controller,
		struct spi_nand_priv *ptr_rtn_flash_id )
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	/* 1. Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Write op_cmd 0x9F (Read ID) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_READ_ID );

	/* 3. Write Address Byte (0x00) */
	spi_controller_write1(spi_controller, _SPI_NAND_ADDR_MANUFACTURE_ID );

	/* 4. Read data (Manufacture ID and Device ID) */
	uint8_t tmp[2];
	spi_controller_read(spi_controller, tmp, 2, SPI_CONTROLLER_SPEED_SINGLE);

	ptr_rtn_flash_id->mfr_id = tmp[0];
	ptr_rtn_flash_id->dev_id = tmp[1];

	/* 5. Chip Select High */
	spi_controller_cs_release(spi_controller);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_id : mfr_id = 0x%x, dev_id = 0x%x\n",
		ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id);

	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id_2 (const struct spi_controller *spi_controller,
		struct spi_nand_priv *ptr_rtn_flash_id )
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	/* 1. Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Write op_cmd 0x9F (Read ID) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_READ_ID);

	/* 3. Read data (Manufacture ID and Device ID) */
	uint8_t tmp[2];
	spi_controller_read(spi_controller, tmp, 2, SPI_CONTROLLER_SPEED_SINGLE);

	ptr_rtn_flash_id->mfr_id = tmp[0];
	ptr_rtn_flash_id->dev_id = tmp[1];

	/* 4. Chip Select High */
	spi_controller_cs_release(spi_controller);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_id_2 : mfr_id = 0x%x, dev_id = 0x%x\n",
		ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id);

	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id_3 (const struct spi_controller *spi_controller,
		struct spi_nand_priv *ptr_rtn_flash_id )
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	uint8_t dummy = 0;

	/* 1. Chip Select Low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Write op_cmd 0x9F (Read ID) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_READ_ID);

	/* 3. Read data (Manufacture ID and Device ID) */
	spi_controller_read(spi_controller, &dummy, _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);
	spi_controller_read(spi_controller, &(ptr_rtn_flash_id->mfr_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);
	spi_controller_read(spi_controller, &(ptr_rtn_flash_id->dev_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);

	/* 4. Chip Select High */
	spi_controller_cs_release(spi_controller);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_id_3 : dummy = 0x%x, mfr_id = 0x%x, dev_id = 0x%x\n", dummy, ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id);

	return (rtn_status);
}

#define PREAMBLE(_cntx) \
	const struct spi_controller *spi_controller __attribute__ ((unused)); \
	const struct spi_nand_priv *flash_info __attribute__ ((unused)); \
	assert(_cntx);\
	spi_controller = _cntx->spi_controller;\
	assert(spi_controller);\
	flash_info = flash_get_priv(_cntx);\
	assert(flash_info)

static int spi_nand_protocol_page_read (const struct flash_cntx *cntx, uint32_t page_number)
{
	PREAMBLE(cntx);
	CMDBUFF(cmd);

	/* Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* Send 13h opcode */
	cmdbuff_push(&cmd, _SPI_NAND_OP_PAGE_READ);

	/* Send page number */
	cmdbuff_push(&cmd, (page_number >> 16) & 0xff);
	cmdbuff_push(&cmd, (page_number >> 8) & 0xff);
	cmdbuff_push(&cmd, page_number & 0xff);
	spi_controller_write(spi_controller, cmd.buff, cmd.pos, SPI_CONTROLLER_SPEED_SINGLE);

	/* Chip Select High */
	spi_controller_cs_release(spi_controller);

	spi_nand_dbg("loaded page 0x%x into cache\n", page_number);

	return 0;
}

static int _spi_nand_protocol_read_from_cache(const struct flash_cntx *flash,
		uint32_t data_offset,
		uint32_t len,
		uint8_t *buf,
		uint32_t read_mode,
		SPI_NAND_FLASH_READ_DUMMY_BYTE_T dummy_mode )
{
	PREAMBLE(flash);
	CMDBUFF(cmd);

	spi_nand_dbg("addr = 0x%08x, len = 0x%08x\n", data_offset, len);

	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);
#if 0
	/* 2. Send opcode */
	switch (read_mode)
	{
		/* 03h */
		case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
			cmdbuf[cmdbufpos++] = _SPI_NAND_OP_READ_FROM_CACHE_SINGLE;
			break;

		/* 3Bh */
		case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
			cmdbuf[cmdbufpos++] = _SPI_NAND_OP_READ_FROM_CACHE_DUAL;
			break;

		/* 6Bh */
		case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
			cmdbuf[cmdbufpos++] = _SPI_NAND_OP_READ_FROM_CACHE_QUAD;
			break;

		default:
			break;
	}
#else
	cmdbuff_push(&cmd, _SPI_NAND_OP_READ_FROM_CACHE_SINGLE);
#endif
	/* 3. Send data_offset addr */
	if( dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND )
		/* dummy byte */
		cmdbuff_push(&cmd, 0xff);

	if (flash_info->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE)
	{
		if( _plane_select_bit == 0)
			cmdbuff_push(&cmd, (data_offset >> 8) & 0xef);
		if( _plane_select_bit == 1)
			cmdbuff_push(&cmd, (data_offset >> 8) | 0x10);
	}
	else
		cmdbuff_push(&cmd, (data_offset >> 8) & 0xff);

	cmdbuff_push(&cmd, data_offset & 0xff);

	/* dummy byte */
	if( dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND )
		cmdbuff_push(&cmd, 0xff);

	/* for dual/quad read dummy byte */
	if( dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND && 
	  ((read_mode == SPI_NAND_FLASH_READ_SPEED_MODE_DUAL) ||
			  (read_mode == SPI_NAND_FLASH_READ_SPEED_MODE_QUAD)))
		cmdbuff_push(&cmd, 0xff);

	spi_controller_write(spi_controller, cmd.buff, cmd.pos, SPI_CONTROLLER_SPEED_SINGLE);

	/* 4. Read n byte (len) data */
	switch (read_mode)
	{
		case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
			spi_controller_read(spi_controller, buf, len, SPI_CONTROLLER_SPEED_SINGLE);
			break;

		case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
			spi_controller_read(spi_controller, buf, len, SPI_CONTROLLER_SPEED_DUAL);
			break;

		case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
			spi_controller_read(spi_controller, buf, len, SPI_CONTROLLER_SPEED_QUAD);
			break;

		default:
			break;
	}

	/* 5. Chip Select High */
	spi_controller_cs_release(spi_controller);

	spi_nand_dbg("read from cache : data_offset = 0x%x, buf = 0x%x\n", data_offset, buf);

	return 0;
}

/*
 * This is a wrapper to read a page from the cache while reissuing the read cache +
 * address after a certain number of bytes to workaround setups that get out of
 * sync.
 */
static int spi_nand_protocol_read_from_cache(const struct flash_cntx *cntx, uint32_t data_offset,
		uint32_t len, uint8_t *buf, uint32_t read_mode, SPI_NAND_FLASH_READ_DUMMY_BYTE_T dummy_mode )
{
	int chunksz = min(len, 128);
	int ret;

	/*
	 * For mstar ddc (with tiny usb i2c?) we seem to loose bytes if the transfer is chunked
	 * without resending the SPI commands. I think the controller is actually
	 * clocking out a byte at the end.
	 * Resending the read command for each block to work around the spi
	 * controller being out of sync.
	 */
	for(int pos = 0; pos < len; pos += chunksz){
		ret = _spi_nand_protocol_read_from_cache(cntx, pos, chunksz,
				buf + pos, read_mode, dummy_mode);
	}

	return ret;
}

static int _spi_nand_protocol_program_load (const struct flash_cntx *cntx,
		uint32_t addr, uint8_t *ptr_data, uint32_t len, uint32_t write_mode, bool continuation)
{
	PREAMBLE(cntx);
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	spi_nand_trace("addr = 0x%08x, len = 0x%08x, cont %d\n", addr, len, continuation);

	/*
	 * The load program data opcode used causes the cache to be filled
	 * with 1s. So we don't need to actually transfer sections of the
	 * page that are all 1s. This reduces the amount of transactions
	 * we need to do over USB, maybe i2c and maybe spi.
	 */
	if(continuation){
		for(int i = 0; i < len; i++){
			if(ptr_data[i] != 0xff)
				goto senddata;
		}
		spi_nand_trace("chunk is all ones, skipping\n");
		return rtn_status;
	}

senddata:
	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);
#if 0
	/* 2. Send opcode */
	switch (write_mode)
	{
		/* 02h */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			spi_controller_write1(spi_controller, _SPI_NAND_OP_PROGRAM_LOAD_SINGLE );
			break;

		/* 32h */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			spi_controller_write1(spi_controller, _SPI_NAND_OP_PROGRAM_LOAD_QUAD );
			break;

		default:
			break;
	}
#else

	/*
	 * First chunk is sent with load single to cause the cache to be set
	 * to all 1s.
	 * All following chunks use load random single to retain the already
	 * loaded data.
	 */
	if(continuation)
		spi_controller_write1(spi_controller, _SPI_NAND_OP_PROGRAM_LOAD_RAMDOM_SINGLE);
	else
		spi_controller_write1(spi_controller, _SPI_NAND_OP_PROGRAM_LOAD_SINGLE );

#endif
	/* 3. Send address offset */
	if( ((flash_info->feature) & SPI_NAND_FLASH_PLANE_SELECT_HAVE) )
	{
		if( _plane_select_bit == 0)
		{
			spi_controller_write1(spi_controller, ((addr >> 8 ) & (0xef)) );
		}
		if( _plane_select_bit == 1)
		{
			spi_controller_write1(spi_controller, ((addr >> 8 ) | (0x10)) );
		}
	}
	else
		spi_controller_write1(spi_controller, (addr >> 8) & 0xff);

	spi_controller_write1(spi_controller, addr & 0xff);

	/* 4. Send data */
	switch (write_mode)
	{
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			spi_controller_write(spi_controller, ptr_data, len, SPI_CONTROLLER_SPEED_SINGLE);
			break;
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			spi_controller_write(spi_controller, ptr_data, len, SPI_CONTROLLER_SPEED_QUAD);
			break;
		default:
			break;
	}
	
	/* 5. Chip Select High */
	spi_controller_cs_release(spi_controller);

	return (rtn_status);
}

static int spi_nand_protocol_program_load(const struct flash_cntx *cntx,
		uint32_t addr, uint8_t *ptr_data, uint32_t len, uint32_t write_mode)
{
	int ret = 0;
	// On some foresee parts the cache has to be written in a single op.
	// trying to chunk it causes the buffer to reset to 0xff
	int chunksz = 2112;
	int pos;

	for(pos = 0; pos != len; pos += min(len - pos, chunksz)){
		ret = _spi_nand_protocol_program_load(cntx, pos, ptr_data + pos,
				min(len - pos, chunksz),write_mode, pos != 0);
	}

	return ret;
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_execute (const struct spi_controller *spi_controller,
		uint32_t addr )
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	spi_nand_trace("addr = 0x%08x\n", addr);

	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Send 10h opcode */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_PROGRAM_EXECUTE );

	/* 3. Send address offset */
	spi_controller_write1(spi_controller, ((addr >> 16  ) & 0xff) );
	spi_controller_write1(spi_controller, ((addr >> 8   ) & 0xff) );
	spi_controller_write1(spi_controller, ((addr)         & 0xff) );

	/* 4. Chip Select High */
	spi_controller_cs_release(spi_controller);

	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_die_select_1(const struct spi_controller *spi_controller,
		uint8_t die_id)
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	/* 1. Chip Select low */
	spi_controller_cs_assert(spi_controller);

	/* 2. Send C2h opcode (Die Select) */
	spi_controller_write1(spi_controller, _SPI_NAND_OP_DIE_SELECT );

	/* 3. Send Die ID */
	spi_controller_write1(spi_controller, die_id );

	/* 5. Chip Select High */
	spi_controller_cs_release(spi_controller);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_die_select_1\n");

	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_die_select_2(const struct spi_controller *spi_controller,
		uint8_t die_id)
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	uint8_t feature;

	rtn_status = spi_nand_protocol_get_status_reg_4(spi_controller, &feature);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("spi_nand_protocol_die_select_2 get die select fail.\n");
		return (rtn_status);
	}

	if(die_id == 0) {
		feature &= ~(0x40);
	} else {
		feature |= 0x40;
	}
	rtn_status = spi_nand_protocol_set_status_reg_4(spi_controller, feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_die_select_2\n");

	return (rtn_status);
}

static void spi_nand_select_die (const struct flash_cntx *cntx,
		uint32_t page_number )
{
	PREAMBLE(cntx);
	uint8_t die_id = 0;

	if( ((flash_info->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) ) {
		/* single die = 1024blocks * 64pages */
		die_id = ((page_number >> 16) & 0xff);

		if (_die_id != die_id)
		{
			_die_id = die_id;
			spi_nand_protocol_die_select_1(spi_controller, die_id);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_1: die_id=0x%x\n", die_id);
		}
	} else if( ((flash_info->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE) ) {
		/* single die = 2plans * 1024blocks * 64pages */
		die_id = ((page_number >> 17) & 0xff);

		if (_die_id != die_id)
		{
			_die_id = die_id;
			spi_nand_protocol_die_select_2(spi_controller, die_id);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_2: die_id=0x%x\n", die_id);
		}
	}
}

static bool spi_nand_check_ecc_status(const struct flash_cntx *cntx, uint32_t page_number)
{
	PREAMBLE(cntx);
	uint8_t status;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	spi_nand_protocol_get_status_reg_3(spi_controller, &status);

	spi_nand_dbg("ecc_fail_check: status = 0x%x\n", status);

	if (!flash_info->ecc_ok) {
		spi_nand_err("ECC check callback is not filled in for this part, don't know how to check ECC status!\n");
		return true;
	}

	return flash_info->ecc_ok(status);
}

static int spi_nand_load_page_into_cache(const struct flash_cntx *cntx,
		uint32_t page_number)
{
	PREAMBLE(cntx);
	uint8_t status;

	spi_nand_dbg("loading page into cache: page number = 0x%x\n", page_number);

	spi_nand_select_die (cntx, page_number);
	spi_nand_protocol_page_read(cntx, page_number);

	/*  Checking status for load page/erase/program complete */
	do {
		spi_nand_protocol_get_status_reg_3(spi_controller, &status);
	} while( status & _SPI_NAND_VAL_OIP) ;

	spi_nand_dbg("loaded page into cache: status = 0x%x\n", status);

	if (!flash_info->ecc_ignore && !spi_nand_check_ecc_status(cntx, page_number))
		return -1;

	return 0;
}

static int spi_nand_block_aligned_check(const struct flash_cntx *cntx, uint32_t addr, uint32_t len )
{
	PREAMBLE(cntx);
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: addr = 0x%x, len = 0x%x, block size = 0x%x \n", addr, len, (flash_info->erase_size));

	if (_SPI_NAND_BLOCK_ALIGNED_CHECK(len, (flash_info->erase_size)))
	{
		len = ( (len/flash_info->erase_size) + 1) * (flash_info->erase_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: erase block aligned first check OK, addr:%x len:%x\n", addr, len, (flash_info->erase_size));
	}

	if (_SPI_NAND_BLOCK_ALIGNED_CHECK(addr, (flash_info->erase_size)) || _SPI_NAND_BLOCK_ALIGNED_CHECK(len, (flash_info->erase_size)) )
	{
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: erase block not aligned, addr:0x%x len:0x%x, blocksize:0x%x\n", addr, len, (flash_info->erase_size));
		rtn_status = SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL;
	}

	return (rtn_status);
}

static int spi_nand_erase_block (const struct flash_cntx *cntx, uint32_t block_index)
{
	PREAMBLE(cntx);
	uint8_t status;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	spi_nand_select_die(cntx, (block_index << _SPI_NAND_BLOCK_ROW_ADDRESS_OFFSET));

	/* 2.2 Enable write_to flash */
	spi_nand_protocol_write_enable(spi_controller);

	/* 2.3 Erasing one block */
	spi_nand_protocol_block_erase(spi_controller, block_index);

	/* 2.4 Checking status for erase complete */
	do {
		spi_nand_protocol_get_status_reg_3(spi_controller, &status);
	} while( status & _SPI_NAND_VAL_OIP) ;

	/* 2.5 Disable write_flash */
	spi_nand_protocol_write_disable(spi_controller);

	/* 2.6 Check Erase Fail Bit */
	if( status & _SPI_NAND_VAL_ERASE_FAIL )
	{
		_SPI_NAND_PRINTF("spi_nand_erase_block : erase block fail, block = 0x%x, status = 0x%x\n", block_index, status);
		rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
	}

	return rtn_status;
}

static int spi_nand_erase_internal(const struct flash_cntx *cntx, uint32_t addr, uint32_t len )
{
	PREAMBLE(cntx);
	uint32_t block_index = 0;
	uint32_t erase_len = 0;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
#if 0
	print_dot  = 0;
#endif

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\nspi_nand_erase_internal (in): addr = 0x%x, len = 0x%x\n", addr, len );

	/* Switch to manual mode*/
	spi_controller_enable_manual_mode(spi_controller);

	/* 1. Check the address and len must aligned to NAND Flash block size */
	if( spi_nand_block_aligned_check(cntx, addr, len) == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		/* 2. Erase block one by one */
		while( erase_len < len )
		{
			/* 2.1 Caculate Block index */
			block_index = (addr/(flash_info->erase_size));

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_erase_internal: addr = 0x%x, len = 0x%x, block_idx = 0x%x\n", addr, len, block_index );

			rtn_status = spi_nand_erase_block(cntx, block_index);

			/* 2.6 Check Erase Fail Bit */
			if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR)
			{
				_SPI_NAND_PRINTF("spi_nand_erase_internal : Erase Fail at addr = 0x%x, len = 0x%x, block_idx = 0x%x\n", addr, len, block_index);
				rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
			}

			/* 2.7 Erase next block if needed */
			addr		+= flash_info->erase_size;
			erase_len	+= flash_info->erase_size;

			ui_statusbar_erase(erase_len, len);
		}
		ui_statusbar_erasedone(erase_len, len);
	}
	else
	{
		rtn_status = SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL;
	}

	return 	(rtn_status);
}

static int spi_nand_read_page (const struct flash_cntx *cntx, uint32_t page_number,
		SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode, uint8_t *buf, size_t bufsz, bool include_oob)
{
	PREAMBLE(cntx);
	uint32_t idx = 0;
	uint32_t i, j;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	struct spi_nand_flash_oobfree *ptr_oob_entry_idx;
	u16 read_addr;
	size_t read_sz = flash_info->page_size + (include_oob ? flash_info->oob_size : 0);

	assert(buf);

	/* read from read_addr index in the page */
	read_addr = 0;

	/* Switch to manual mode*/
	spi_controller_enable_manual_mode(spi_controller);

	/* Load Page into cache of NAND Flash Chip */
	if( spi_nand_load_page_into_cache(cntx, page_number) == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK )
	{
		_SPI_NAND_PRINTF("spi_nand_read_page: Bad Block, ECC cannot recovery detecte, page = 0x%x\n", page_number);
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}

	/* Read whole data from cache of NAND Flash Chip */
	spi_nand_dbg("curren_page_num = 0x%x, page_number = 0x%x\n", _current_page_num, page_number);

	memset(buf, 0, bufsz);

	if(flash_info->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE)
	{
		_plane_select_bit = ((page_number >> 6)& (0x1));

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"spi_nand_read_page: plane select = 0x%x\n",  _plane_select_bit);
	}

	spi_nand_protocol_read_from_cache(cntx, read_addr, read_sz, buf, speed_mode, flash_info->dummy_mode );

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: after read, _current_cache_page:\n");
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], _SPI_NAND_CACHE_SIZE);

	/* Divide read page into data segment and oob segment  */
	{
		//memcpy( &_current_cache_page_data[0], &_current_cache_page[0], (flash_info->page_size) );
		// ~dgp fix this
		//if(!ECC_fcheck)
		goto noecc;
#if 0
		memcpy( &_current_cache_page_oob[0],  &_current_cache_page[(flash_info->page_size)], (flash_info->oob_size) );

		idx = 0;
		ptr_oob_entry_idx = (struct spi_nand_flash_oobfree*) &( (flash_info->oob_free_layout)->oobfree );

		if( _ondie_ecc_flag == 1)   /*  When OnDie ecc is enable,  mapping oob area is neccessary */
		{
			/* Transter oob area from physical offset into logical offset */
			for( i = 0; (i < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (ptr_oob_entry_idx[i].len) && (idx < ((flash_info->oob_free_layout)->oobsize)) ; i++)
			{
				for(j = 0; (j < (ptr_oob_entry_idx[i].len)) && (idx < (flash_info->oob_free_layout->oobsize)) ; j++)
				{
					/* _SPI_NAND_PRINTF("i=%d , j=%d, len=%d, idx=%d, size=%d\n", i, j,(ptr_oob_entry_idx[i].len), idx, (flash_info->oob_free_layout->oobsize) ); */
					_current_cache_page_oob_mapping[idx] = _current_cache_page_oob[(ptr_oob_entry_idx[i].offset)+j];
					idx++;
				}
			}
		}
		else
		{
			memcpy( &_current_cache_page_oob_mapping[0],  &_current_cache_page_oob[0], (flash_info->oob_size) );
		}
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ((flash_info->page_size)+(flash_info->oob_size)));
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page_oob:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob[0], (flash_info->oob_size));
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page_oob_mapping:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob_mapping[0], (flash_info->oob_size));
#endif
	}
noecc:

	return rtn_status;
}

static SPI_NAND_FLASH_RTN_T spi_nand_write_page(const struct flash_cntx *cntx,
		uint32_t page_number,
		uint32_t data_offset,
		uint8_t  *ptr_data,
		uint32_t data_len,
		uint32_t oob_offset,
		uint8_t  *ptr_oob,
		uint32_t oob_len,
		SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode)
{
		PREAMBLE(cntx);
		uint8_t status, status_2;
		uint32_t i = 0, j = 0, idx = 0;
		struct spi_nand_flash_oobfree *ptr_oob_entry_idx;
		SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
		u16 write_addr;

		/* write to write_addr index in the page */
		write_addr = 0;

		/* Switch to manual mode*/
		spi_controller_enable_manual_mode(spi_controller);

		/* Read Current page data to software cache buffer */
		rtn_status = spi_nand_read_page(cntx, page_number, speed_mode,
				flash_info->page_buffer, flash_info->page_buffer_sz, false);
// We should check this..
//		if (rtn_status ) {
//
//		}

// ~dgp fix ecc
#if 0
		if(ECC_fcheck && oob_len > 0 )	/* Write OOB */
		{
			{
				if(_ondie_ecc_flag == 1)	/*  When OnDie ecc is enable,  mapping oob area is neccessary */
				{
					ptr_oob_entry_idx = (struct spi_nand_flash_oobfree*) &( flash_info->oob_free_layout->oobfree );
							
					for( i = 0; (i < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (ptr_oob_entry_idx[i].len) && ((idx < (flash_info->oob_free_layout->oobsize)) && (idx < oob_len))  ; i++)
					{
						for(j = 0; (j < (ptr_oob_entry_idx[i].len)) && (idx < (flash_info->oob_free_layout->oobsize)) && ((idx < (flash_info->oob_free_layout->oobsize)) && (idx < oob_len)) ; j++)
						{
							_current_cache_page_oob[(ptr_oob_entry_idx[i].offset)+j] &= ptr_oob[idx];
							idx++;
						}
					}
				}
				else
				{
					if(oob_len) memcpy( &_current_cache_page_oob[0], &ptr_oob[0], oob_len);
				}
				if(flash_info->oob_size)
					memcpy( &_current_cache_page[flash_info->page_size],  &_current_cache_page_oob[0], flash_info->oob_size );
			}
		}
#endif
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: page = 0x%x, data_offset = 0x%x, date_len = 0x%x, oob_offset = 0x%x, oob_len = 0x%x\n", page_number, data_offset, data_len, oob_offset, oob_len);
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ((flash_info->page_size) + (flash_info->oob_size)));

		if( ((flash_info->feature) & SPI_NAND_FLASH_PLANE_SELECT_HAVE) )
		{
			_plane_select_bit = ((page_number >> 6) & (0x1));

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: _plane_select_bit = 0x%x\n", _plane_select_bit );
		}

		spi_nand_select_die(cntx, page_number);

		bool weafter = false;
		switch(flash_info->mfr_id){
			case _SPI_NAND_MANUFACTURER_ID_GIGADEVICE:
			case _SPI_NAND_MANUFACTURER_ID_PN:
			//case _SPI_NAND_MANUFACTURER_ID_FM:
			case _SPI_NAND_MANUFACTURER_ID_XTX:
			case _SPI_NAND_MANUFACTURER_ID_FISON:
			case _SPI_NAND_MANUFACTURER_ID_TYM:
			case _SPI_NAND_MANUFACTURER_ID_ATO_2:
				weafter = true;
				break;
			case _SPI_NAND_MANUFACTURER_ID_FORESEE:
				switch(flash_info->dev_id) {
				case _SPI_NAND_DEVICE_ID_FS35ND02GS3Y2:
					break;
				default:
					weafter = true;
				}
				break;
			case _SPI_NAND_MANUFACTURER_ID_ATO:
				switch(flash_info->dev_id) {
				case _SPI_NAND_DEVICE_ID_ATO25D2GA:
					weafter = true;
					break;
				}
				break;
		}

		if(weafter)
			spi_nand_trace("Enabling write after loading page\n");
		else
			spi_nand_trace("Enabling write before loading page\n");

		/* Different Manufacturer have different program flow and setting */
		if(weafter) {
			spi_nand_protocol_program_load(cntx, write_addr, flash_info->page_buffer,
					((flash_info->page_size) + (flash_info->oob_size)),
					speed_mode);

			/* Enable write_to flash */
			spi_nand_protocol_write_enable(spi_controller);
		}
		else
		{
			/* Enable write_to flash */
			spi_nand_protocol_write_enable(spi_controller);

			/* Program data into buffer of SPI NAND chip */
			spi_nand_protocol_program_load(cntx, write_addr, flash_info->page_buffer,
					((flash_info->page_size) + (flash_info->oob_size)),
					speed_mode);
		}

		/* Execute program data into SPI NAND chip  */
		spi_nand_protocol_program_execute(spi_controller, page_number);

		/* Checking status for erase complete */
		do {
			spi_nand_protocol_get_status_reg_3(spi_controller, &status);
		} while( status & _SPI_NAND_VAL_OIP) ;

		/*. Disable write_flash */
		spi_nand_protocol_write_disable(spi_controller);

		spi_nand_protocol_get_status_reg_1(spi_controller, &status_2);

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spi_nand_write_page]: status 1 = 0x%x, status 3 = 0x%x\n", status_2, status);

		/* Check Program Fail Bit */
		if( status & _SPI_NAND_VAL_PROGRAM_FAIL )
		{
			_SPI_NAND_PRINTF("spi_nand_write_page : Program Fail at addr_offset = 0x%x, page_number = 0x%x, status = 0x%x\n", data_offset, page_number, status);
			rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
		}

		return (rtn_status);
}

static int spi_nand_write_internal(const struct flash_cntx *cntx,
		uint32_t dst_addr, uint32_t len, uint32_t *ptr_rtn_len, u8* ptr_buf, SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode )
{
	PREAMBLE(cntx);
	uint32_t remain_len, write_addr, data_len, page_number, physical_dst_addr;
	uint32_t addr_offset;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	*ptr_rtn_len = 0;
	remain_len = len;
	write_addr = dst_addr;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_write_internal: remain_len = 0x%x\n", remain_len);

	while( remain_len > 0 )
	{
		physical_dst_addr = write_addr;

		/* Caculate page number */
		addr_offset = (physical_dst_addr % (flash_info->page_size));
		page_number = (physical_dst_addr / (flash_info->page_size));

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,
				"\nspi_nand_write_internal: addr_offset = 0x%x, page_number = 0x%x, remain_len = 0x%x, page_size = 0x%x\n", addr_offset, page_number, remain_len,(flash_info->page_size) );
		if( ((addr_offset + remain_len ) > (flash_info->page_size))  )  /* data cross over than 1 page range */
		{
			data_len = ((flash_info->page_size) - addr_offset);
		}
		else
		{
			data_len = remain_len;
		}

		/*
		 * Check if the target page is all ones and skip it if that's
		 * the case
		 */
		//printf("data len %d\n", data_len);
		for(int i = 0; i < data_len; i++) {
			if(ptr_buf[(len - remain_len) + i] != 0xff)
				goto write;
		}
		goto skip;
write:
		rtn_status = spi_nand_write_page(cntx, page_number, addr_offset,
				&(ptr_buf[len - remain_len]), data_len, 0, NULL, 0 , speed_mode);
skip:
		/* 8. Write remain data if neccessary */
		write_addr += data_len;
		remain_len -= data_len;
		ptr_rtn_len += data_len;

		ui_statusbar_write(len, remain_len);
	}
	ui_statusbar_writedone(len, len - remain_len);

	return (rtn_status);
}

static int spi_nand_read_internal (const struct flash_cntx *cntx,
		uint32_t addr, uint32_t len, uint8_t *buf,
		SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode, SPI_NAND_FLASH_RTN_T *status)
{
	PREAMBLE(cntx);
	int ret = 0;

	spi_nand_dbg("\nspi_nand_read_internal: addr = 0x%lx, len = 0x%x\n", addr, len);

	*status = SPI_NAND_FLASH_RTN_NO_ERROR;

	// fix me, think about oob
	for (unsigned pos = 0; pos < len; pos += flash_info->page_size)
	{
		uint32_t page_number, data_offset;
		uint32_t cur_addr = addr + pos;
		uint32_t copysz = min(len - pos, flash_info->page_size);

		/* Calculate page number */
		data_offset = (cur_addr % (flash_info->page_size));
		// for now don't handle misalignment
		assert(data_offset == 0);

		page_number = (cur_addr / (flash_info->page_size));

		spi_nand_dbg("read: read_addr = 0x%x, page_number = 0x%x, data_offset = 0x%x, copy_sz = 0x%x\n",
				cur_addr, page_number, data_offset, copysz);

		/* Read the page containing the data */
		ret = spi_nand_read_page(cntx, page_number, speed_mode, flash_info->page_buffer,
				flash_info->page_buffer_sz, false);
		if (ret == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK) {
			*status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
			return ret;
		}

		/* Copy the required part of the page into the output */
		memcpy(buf + pos, flash_info->page_buffer, copysz);

		ui_statusbar_read(len, len - pos);

		if (should_abort())
			break;
	}

	return ret;
}

static void spi_nand_manufacturer_init(const struct spi_controller *spi_controller,
		struct spi_nand_priv *ptr_device_t)
{

}

static void spi_nand_populate(struct spi_nand_priv *priv, const struct spi_nand_priv *flash)
{
	int ECC_fcheck = 1;

	bmt_oob_size		= flash->oob_size;
	priv->dummy_mode	= flash->dummy_mode;
	priv->read_mode		= flash->read_mode;
	priv->write_mode	= flash->write_mode;

	erase_oob_size		= (flash->erase_size / flash->page_size) * flash->oob_size;
	priv->erase_size	= ECC_fcheck ? flash->erase_size : flash->erase_size + erase_oob_size;
	priv->page_size		= ECC_fcheck ? flash->page_size : flash->page_size + flash->oob_size;
	priv->oob_size		= ECC_fcheck ? flash->oob_size : 0;

	ecc_size = ((flash->device_size / flash->erase_size) * ((flash->erase_size / flash->page_size) * flash->oob_size));
	priv->device_size = ECC_fcheck ? flash->device_size : flash->device_size + ecc_size;
	memcpy( &(priv->name) , &(flash->name), sizeof(flash->name));
	memcpy( &(priv->oob_free_layout) , &(flash->oob_free_layout), sizeof(flash->oob_free_layout));
	priv->feature = flash->feature;

	priv->ecc_disable = flash->ecc_disable;
	priv->ecc_ignore = flash->ecc_ignore;
	priv->ecc_ok = flash->ecc_ok;
}

static int spi_nand_probe(const struct spi_controller *spi_controller,
		struct spi_nand_priv *ptr_rtn_device_t )
{
	spi_nand_dbg("probe start \n");

	/* Protocol for read id */
	spi_nand_dbg("Trying to get ID\n");
	spi_nand_protocol_read_id(spi_controller, ptr_rtn_device_t );
	spi_nand_protocol_read_id(spi_controller, ptr_rtn_device_t );

	for (int i = 0; i < array_size(spi_nand_flash_tables); i++)
	{
		spi_nand_dbg("spi_nand_probe: table[%d]: mfr_id = 0x%x, dev_id = 0x%x\n",
				i, spi_nand_flash_tables[i].mfr_id, spi_nand_flash_tables[i].dev_id );

		if (ptr_rtn_device_t->mfr_id == spi_nand_flash_tables[i].mfr_id &&
				ptr_rtn_device_t->dev_id == spi_nand_flash_tables[i].dev_id) {
			spi_nand_populate(ptr_rtn_device_t, &spi_nand_flash_tables[i]);
			goto found;
		}
	}

	/* Another protocol for read id  (For example, the GigaDevice SPI NADN chip for Type C */
	spi_nand_protocol_read_id_2(spi_controller, ptr_rtn_device_t );

	for (int i = 0; i < array_size(spi_nand_flash_tables); i++)
	{
		spi_nand_dbg("spi_nand_probe: table[%d]: mfr_id = 0x%x, dev_id = 0x%x\n",
				i, spi_nand_flash_tables[i].mfr_id, spi_nand_flash_tables[i].dev_id );

		if (ptr_rtn_device_t->mfr_id == spi_nand_flash_tables[i].mfr_id &&
				ptr_rtn_device_t->dev_id == spi_nand_flash_tables[i].dev_id)
		{
			spi_nand_populate(ptr_rtn_device_t, &spi_nand_flash_tables[i]);
			goto found;
		}
	}

	/* Another protocol for read id  (For example, the Toshiba/KIOXIA SPI NAND chip */
	spi_nand_protocol_read_id_3( spi_controller, ptr_rtn_device_t );

	for (int i = 0; i < array_size(spi_nand_flash_tables); i++)
	{
		spi_nand_dbg("spi_nand_probe: table[%d]: mfr_id = 0x%x, dev_id = 0x%x\n",
				i, spi_nand_flash_tables[i].mfr_id, spi_nand_flash_tables[i].dev_id );

		if (ptr_rtn_device_t->mfr_id == spi_nand_flash_tables[i].mfr_id &&
				ptr_rtn_device_t->dev_id == spi_nand_flash_tables[i].dev_id){
			spi_nand_populate(ptr_rtn_device_t, &spi_nand_flash_tables[i]);
			goto found;
		}
	}

	return -ENODEV;

found:
	spi_nand_dbg("mfr_id = 0x%x, dev_id = 0x%x\n", ptr_rtn_device_t->mfr_id, ptr_rtn_device_t->dev_id);

	unsigned char feature = 0;
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	spi_nand_dbg("Get Status Register 1: 0x%02x\n", feature);
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	spi_nand_dbg("Get Status Register 2: 0x%02x\n", feature);
	spi_nand_manufacturer_init(spi_controller, ptr_rtn_device_t);

	spi_nand_dbg("probe end \n");

	return 0;
}

static int spi_nand_enable_ondie_ecc(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);
	unsigned char feature;
	uint8_t die_num;

	if(((flash_info->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE)) {
		die_num = (flash_info->device_size / flash_info->page_size) >> 16;

		for(int i = 0; i < die_num; i++) {
			int ECC_fcheck = 1;
			spi_nand_protocol_die_select_1(spi_controller, i);

			spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);
			if (ECC_fcheck)
				feature |= 0x10;
			else
				feature &= ~(1 << 4);
			spi_nand_protocol_set_status_reg_2(spi_controller,feature);

			/* Value check*/
			spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);
		}
	} else if(((flash_info->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE)) {
		die_num = (flash_info->device_size / flash_info->page_size) >> 17;

		for(int i = 0; i < die_num; i++) {
			int ECC_fcheck = 1;
			spi_nand_protocol_die_select_2(spi_controller, i);

			spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);
			if (ECC_fcheck)
				feature |= 0x10;
			else
				feature &= ~(1 << 4);
			spi_nand_protocol_set_status_reg_2(spi_controller,feature);

			/* Value check*/
			spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);
		}
	} else {
		if( ((flash_info->mfr_id) == _SPI_NAND_MANUFACTURER_ID_PN) ||
			((flash_info->mfr_id) == _SPI_NAND_MANUFACTURER_ID_FM) ||
			((flash_info->mfr_id) == _SPI_NAND_MANUFACTURER_ID_FORESEE) ||
			(((flash_info->mfr_id) == _SPI_NAND_MANUFACTURER_ID_XTX) && ((flash_info->dev_id) == _SPI_NAND_DEVICE_ID_XT26G02B)) )
		{
			int ECC_fcheck = 1;
			spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_ECC, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, ecc reg = 0x%x\n", feature);
			if (ECC_fcheck)
				feature |= 0x10;
			else
				feature &= ~(1 << 4);
			spi_nand_protocol_set_feature(spi_controller, _SPI_NAND_ADDR_ECC, feature);

			/* Value check*/
			spi_nand_protocol_get_feature(spi_controller, _SPI_NAND_ADDR_ECC, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, ecc reg = 0x%x\n", feature);
		}
		else
		{
			int ECC_fcheck = 1;
			spi_nand_protocol_get_status_reg_2(spi_controller, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);

			if (ECC_fcheck)
				feature |= 0x10;
			else
				feature &= ~(1 << 4);
			spi_nand_protocol_set_status_reg_2(spi_controller,feature);

			/* Value check*/
			spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg = 0x%x\n", feature);
		}
	}

	int ECC_fcheck = 1;
	if (ECC_fcheck)
		_ondie_ecc_flag = 1;
	else
		_ondie_ecc_flag = 0;
	return (SPI_NAND_FLASH_RTN_NO_ERROR);
}

static int spi_nand_chip_init(const struct flash_cntx *cntx, struct spi_nand_priv *flashinfo)
{
	PREAMBLE(cntx);

	/* Enable Manual Mode */
	spi_controller_enable_manual_mode(spi_controller);

	/* Probe flash information */
	if (spi_nand_probe(spi_controller, flashinfo))
	{
		spi_nand_err("SPI NAND Flash Not Detected!\n");
		return -ENODEV;
	}

	if (flashinfo->ecc_ignore)
		spi_nand_info("Ignoring ECC errors!\n");

	spi_nand_enable_ondie_ecc(cntx);
	spi_nand_info("Detected SPI NAND Flash: %s, Flash Size: %d MB\n",
			flash_info->name,  flash_info->device_size >> 20);

	return 0;
}

static int spi_nand_read(const struct flash_cntx *cntx, uint8_t *buf, uint32_t from, uint32_t len)
{
	PREAMBLE(cntx);
	SPI_NAND_FLASH_RTN_T status;

	timer_start();

	int ret = spi_nand_read_internal(cntx, from, len, buf, flash_info->read_mode, &status);

	timer_end();

	return ret;
}

static int spi_nand_erase(const struct flash_cntx *cntx, uint32_t offs, uint32_t len)
{
	PREAMBLE(cntx);

	timer_start();

	int ret = spi_nand_erase_internal(cntx, offs, len);

	timer_end();

	return ret;
}

static int spi_nand_write(const struct flash_cntx *cntx, uint8_t *buf, uint32_t to, uint32_t len)
{
	PREAMBLE(cntx);
	uint32_t retlen;

	timer_start();

	int ret = spi_nand_write_internal(cntx, to, len, &retlen, buf, flash_info->write_mode);

	timer_end();

	return ret;
}

static int spi_nand_identify(const struct flash_cntx *cntx)
{
	PREAMBLE(cntx);
	uint8_t status2;
	char uniqueidstr[(SPI_NAND_UNIQUEID_LEN * 2) + 1] = { 0 };
	int ret;

	/* Enable OTP access */
	spi_nand_protocol_get_status_reg_2(spi_controller, &status2);
	status2 |= SPI_NAND_STATUS2_OTPE;
	spi_nand_protocol_set_status_reg_2(spi_controller, status2);

	spi_nand_info("Reading unique id from device\n");
	ret = spi_nand_read_page(cntx, SPI_NAND_OTP_PAGE_UNIQUEID,
			SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
			flash_info->page_buffer, flash_info->page_buffer_sz,
			false);
	if (ret)
		goto out;

	for (int i = 0; i < SPI_NAND_UNIQUEID_LEN; i++)
		sprintf(&uniqueidstr[i * 2], "%02x", flash_info->page_buffer[i]);

	spi_nand_info("Unique ID: %s\n", uniqueidstr);

	spi_nand_info("Reading parameter page from device\n");
	ret = spi_nand_read_page(cntx, SPI_NAND_OTP_PAGE_PARAMETERPAGE,
			SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE,
			flash_info->page_buffer, flash_info->page_buffer_sz,
			false);
	if (ret)
		goto out;

	struct spi_nand_parameter_page *parameter_page = (void *) flash_info->page_buffer;
	if (memcmp(parameter_page->signature,
			spi_nand_parameter_page_signature,
			sizeof(spi_nand_parameter_page_signature)) != 0) {

		spi_nand_err("Parameter page signature incorrect: %02x%02x%02x%02x\n",
				parameter_page->signature[0], parameter_page->signature[1],
				parameter_page->signature[2], parameter_page->signature[3]);

		ret = -EINVAL;
		goto out;
	}

out:
	/* Disable OTP access */
	spi_nand_protocol_get_status_reg_2(spi_controller, &status2);
	status2 &= ~SPI_NAND_STATUS2_OTPE;
	spi_nand_protocol_set_status_reg_2(spi_controller, status2);

	return ret;
}

static const struct flash_ops spi_nand_flash_ops = {
	.identify = spi_nand_identify,
	.erase = spi_nand_erase,
	.write = spi_nand_write,
	.read  = spi_nand_read,
};

int spi_nand_init(const struct spi_controller *spi_controller,
		struct flash_cntx *flash,
		const struct ui_parsed_cmdline *cmdline)
{
	assert(spi_controller);
	assert(flash);
	assert(cmdline);

	struct spi_nand_priv *flashinfo = malloc(sizeof(*flashinfo));
	if (!flashinfo)
		return -ENOMEM;
	memset(flashinfo, 0, sizeof(*flashinfo));

	flashinfo->ecc_disable = cmdline->ecc_disable;
	flashinfo->ecc_ignore = cmdline->ecc_ignore;

	flash_set_priv(flash, flashinfo);

	if (!spi_nand_chip_init(flash, flashinfo)) {
		flash->ops = &spi_nand_flash_ops;
		flash->org.device_size = flashinfo->device_size;
		flash->org.block_size = flashinfo->erase_size;
		flashinfo->page_buffer_sz = flashinfo->page_size + flashinfo->oob_size;
		flashinfo->page_buffer = malloc(flashinfo->page_buffer_sz);

		return 0;
	}

	// fixme
	flash->priv = NULL;

	return -ENODEV;
}

void spi_nand_flash_foreach(void (*cb)(const struct spi_nand_priv *flash_info))
{
	for (int i = 0; i < array_size(spi_nand_flash_tables); i++)
		cb(&spi_nand_flash_tables[i]);
}
