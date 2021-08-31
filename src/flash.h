/*
 * flash.h
 */

#ifndef SRC_FLASH_H_
#define SRC_FLASH_H_

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

struct flash_region {
	uint32_t addr_start;
	uint32_t addr_end;
	bool lockable;
	bool locked;

	bool want_to_lock;
	bool want_to_unlock;
};

struct flash_status {
	int num_regions;
	struct flash_region regions[];
};

struct flash_org {
	uint32_t device_size;
	uint32_t block_size;
};

struct flash_cntx;

struct flash_ops {
	int (*identify)(const struct flash_cntx *cntx);
	int (*unlock)(const struct flash_cntx *cntx, uint32_t offset, uint32_t len);
	int (*lock)(const struct flash_cntx *cntx, uint32_t offset, uint32_t len);
	int (*erase)(const struct flash_cntx *cntx, uint32_t offset, uint32_t len);
	int (*read)(const struct flash_cntx *cntx, uint8_t *buf, uint32_t from, uint32_t len);
	int (*write)(const struct flash_cntx *cntx, uint8_t *buf, uint32_t to, uint32_t len);
	int (*verify)(const struct flash_cntx *cntx, const uint8_t *buf, uint32_t off, uint32_t len);
};

struct flash_cntx {
	const struct flash_ops *ops;
	struct flash_org org;
	struct flash_status *status;

	const struct i2c_controller *i2c_controller;
	const struct spi_controller *spi_controller;

	void *priv;
};

static inline int flash_identify(const struct flash_cntx *flash)
{
	if (flash->ops->identify)
		return flash->ops->identify(flash);
	else
		return 0;
}

static inline bool flash_range_contains_region(struct flash_region *region, uint32_t offset, uint32_t len)
{
	uint32_t end = offset + len;

	return (region->addr_start >= offset && region->addr_start < end) &&
			(region->addr_end >= offset && region->addr_end < end);
}

static inline bool flash_region_contains(struct flash_region *region, uint32_t offset, uint32_t len)
{
	uint32_t end = offset + len;

	return ((offset >= region->addr_start && offset <= region->addr_end) ||
			(end >= region->addr_start && end <= region->addr_end));
}

static inline bool flash_contains_protected_blocks(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	if (!flash->status)
		return false;

	for (int i = 0; i < flash->status->num_regions; i++) {
		struct flash_region *region = &flash->status->regions[i];

		/* Is either the start of the operation or the end of the operation inside a locked region? */
		if (region->locked && flash_region_contains(region, offset, len))
				return true;
	}

	return false;
}

static inline void flash_mark_unlock(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	/* Update the regions to mark the places that we want to unlock */
	for (int i = 0; i < flash->status->num_regions; i++) {
		struct flash_region *region = &flash->status->regions[i];

		region->want_to_unlock = region->locked && flash_range_contains_region(region, offset, len);
	}
}

static inline int flash_unlock(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	if (flash->ops->unlock)
		return flash->ops->unlock(flash, offset, len);
	else
		return -EPERM;
}

static inline void flash_mark_lock(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	/* Update the regions to mark the places that we want to lock */
	for (int i = 0; i < flash->status->num_regions; i++) {
		struct flash_region *region = &flash->status->regions[i];

		region->want_to_lock = !region->locked && flash_range_contains_region(region, offset, len);
	}
}

static inline int flash_lock(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	if (flash->ops->lock)
		return flash->ops->lock(flash, offset, len);
	else
		return -EPERM;
}

static inline int flash_erase(const struct flash_cntx *flash, uint32_t offset, uint32_t len)
{
	if (flash->ops->erase)
		return flash->ops->erase(flash, offset, len);
	else
		return -EINVAL;
}

static inline int flash_read(const struct flash_cntx *flash, uint8_t *buf, int32_t from, uint32_t len)
{
	if (flash->ops->read)
		return flash->ops->read(flash, buf, from, len);
	else
		return -EINVAL;
}

static inline int flash_write(const struct flash_cntx *flash, uint8_t *buf, uint32_t to, uint32_t len)
{
	if (flash_contains_protected_blocks(flash, to, len))
		return -EPERM;

	if (flash->ops->write)
		return flash->ops->write(flash, buf, to, len);

	return -EINVAL;
}

static inline int flash_verify(const struct flash_cntx *flash, uint8_t *buf, int32_t from, uint32_t len)
{
	if (flash->ops->verify)
		return flash->ops->verify(flash, buf, from, len);
	else
		return -EINVAL;
}

static inline void flash_set_priv(struct flash_cntx *flash, void *priv)
{
	assert(!flash->priv);

	flash->priv = priv;
}

static inline void* flash_get_priv(const struct flash_cntx *flash)
{
	return flash->priv;
}

#endif /* SRC_FLASH_H_ */
