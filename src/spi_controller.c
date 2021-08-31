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

#define MAX_TRANSFER(len) \
	(spi_controller->max_transfer ? spi_controller->max_transfer(spi_controller) : len)

SPI_CONTROLLER_RTN_T spi_controller_enable_manual_mode(const struct spi_controller *spi_controller)
{
	return 0;
}

int spi_controller_write1(const struct spi_controller *spi_controller, uint8_t data)
{
	assert(spi_controller);

	return spi_controller->send_command(spi_controller, 1, 0, &data, NULL);
}

int spi_controller_cs_release(const struct spi_controller *spi_controller)
{
	assert(spi_controller);

	if(spi_controller->cs_release)
		spi_controller->cs_release(spi_controller);

	return 0;
	//return (SPI_CONTROLLER_RTN_T)enable_pins(false);
}

int spi_controller_cs_assert(const struct spi_controller *spi_controller)
{
	assert(spi_controller);

	if(spi_controller->cs_assert)
		spi_controller->cs_assert(spi_controller);
	return 0;
	//return (SPI_CONTROLLER_RTN_T)enable_pins(true);
}

int spi_controller_read(const struct spi_controller *spi_controller,
											   uint8_t *ptr_rtn_data, uint32_t len, SPI_CONTROLLER_SPEED_T speed)
{
	uint32_t chunk_sz = min(len, MAX_TRANSFER(len));
	int ret;

	/*
	 * Handle chunking the transfer when the controller has a smaller max_transfer than the
	 * requested amount.
	 */
	while(len) {
		int read_sz = min(chunk_sz, len);
		ret = spi_controller->send_command(spi_controller, 0, read_sz, NULL, ptr_rtn_data);
		ptr_rtn_data += read_sz;
		len -= read_sz;
		if(ret)
			break;
	}

	return (SPI_CONTROLLER_RTN_T) ret;
}

int spi_controller_write(const struct spi_controller *spi_controller,
												uint8_t *ptr_data, uint32_t len, SPI_CONTROLLER_SPEED_T speed)
{
	uint32_t chunk_sz = min(len, MAX_TRANSFER(len));
	int ret;

	/*
	 * Handle chunking the transfer when the controller has a smaller max_transfer than the
	 * requested amount.
	 */
	while(len) {
		int write_sz = min(chunk_sz, len);

		ret = spi_controller->send_command(spi_controller, write_sz, 0, ptr_data, NULL);
		ptr_data += write_sz;
		len -= write_sz;
		if(ret)
			break;
	}

	return (SPI_CONTROLLER_RTN_T) ret;
}

#if 0
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Xfer_NByte( u8 *ptr_data_in, u32 len_in, u8 *ptr_data_out, u32 len_out, SPI_CONTROLLER_SPEED_T speed )
{
	return (SPI_CONTROLLER_RTN_T)spi_controller->send_command(len_out, len_in, ptr_data_out, ptr_data_in);
}
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
