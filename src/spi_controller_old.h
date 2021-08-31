//SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 */
#ifndef __SPI_CONTROLLER_H__
#define __SPI_CONTROLLER_H__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum{
	SPI_CONTROLLER_SPEED_SINGLE = 0,
	SPI_CONTROLLER_SPEED_DUAL,
	SPI_CONTROLLER_SPEED_QUAD

} SPI_CONTROLLER_SPEED_T;

typedef enum{
	SPI_CONTROLLER_RTN_NO_ERROR = 0,
	SPI_CONTROLLER_RTN_SET_OPFIFO_ERROR,
	SPI_CONTROLLER_RTN_READ_DATAPFIFO_ERROR,
	SPI_CONTROLLER_RTN_WRITE_DATAPFIFO_ERROR,
	SPI_CONTROLLER_RTN_DEF_NO
} SPI_CONTROLLER_RTN_T;

typedef enum{
	SPI_CONTROLLER_MODE_AUTO = 0,
	SPI_CONTROLLER_MODE_MANUAL,
	SPI_CONTROLLER_MODE_NO
} SPI_CONTROLLER_MODE_T;

#ifdef CONFIG_MSTAR_DDC
extern const struct spi_controller mstarddc_spictrl;
#endif

SPI_CONTROLLER_RTN_T spi_controller_enable_manual_mode(const struct spi_controller *spi_controller);
int spi_controller_write1(const struct spi_controller *spi_controller,
		uint8_t data);
int spi_controller_write(const struct spi_controller *spi_controller,
						 uint8_t *ptr_data, uint32_t len, SPI_CONTROLLER_SPEED_T speed);
int spi_controller_read(const struct spi_controller *spi_controller,
					    uint8_t *ptr_rtn_data, uint32_t len, SPI_CONTROLLER_SPEED_T speed);
int spi_controller_cs_assert(const struct spi_controller *spi_controller);
int spi_controller_cs_release(const struct spi_controller *spi_controller);

#if 0
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Xfer_NByte( u8 *ptr_data_in, u32 len_in, u8 *ptr_data_out, u32 len_out, SPI_CONTROLLER_SPEED_T speed );
#endif

const struct spi_controller* spi_controller_by_name(const char* name);
void spi_controller_for_each(void (*cb)(const struct spi_controller *spi_controller));

static inline void spi_controller_set_client_data(const struct spi_controller *spi_controller,
						  void *data, bool free)
{
	assert(!spi_controller->client->priv);

	spi_controller->client->free = free;
	spi_controller->client->priv = data;
}

static inline void *spi_controller_get_client_data(const struct spi_controller *spi_controller)
{
	assert(spi_controller->client->priv);

	return spi_controller->client->priv;
}

static inline int spi_controller_shutdown(const struct spi_controller *spi_controller)
{
	if (spi_controller->shutdown)
		return spi_controller->shutdown(spi_controller);

	return 0;
}

static inline bool spi_controller_need_connstring(const struct spi_controller *spi_controller)
{
	if (spi_controller->need_connstring)
		return spi_controller->need_connstring(spi_controller);

	return false;
}
#endif
