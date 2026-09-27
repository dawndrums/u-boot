/*
 * Maxim MAX17048 fuel gauge driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <power/fuel_gauge.h>

#define MAX17048_REG_VCELL	0x02
#define MAX17048_REG_SOC	0x04
#define MAX17048_REG_VERSION	0x08

struct max17048_info {
	struct udevice *dev;
	u16 version;
};

static int max17048_read_word(struct max17048_info *max17048, u8 reg,
			     u16 *value)
{
	u8 buf[2];
	int ret;

	ret = dm_i2c_read(max17048->dev, reg, buf, sizeof(buf));
	if (ret) {
		debug("MAX17048: read reg 0x%02x failed: %d\n", reg, ret);
		return ret;
	}

	/* MAX17048 16-bit registers are transferred MSB first. */
	*value = ((u16)buf[0] << 8) | buf[1];

	return 0;
}

static int max17048_get_soc(struct udevice *dev)
{
	struct max17048_info *max17048 = dev_get_priv(dev);
	u16 soc;
	int ret;

	ret = max17048_read_word(max17048, MAX17048_REG_SOC, &soc);
	if (ret)
		return ret;

	/* Integer percentage is stored in the upper byte. */
	return soc >> 8;
}

static int max17048_get_voltage(struct udevice *dev)
{
	struct max17048_info *max17048 = dev_get_priv(dev);
	u16 vcell;
	int ret;

	ret = max17048_read_word(max17048, MAX17048_REG_VCELL, &vcell);
	if (ret)
		return ret;

	/* MAX17048 VCELL is 78.125 uV/LSB. Return millivolts. */
	return ((u32)vcell * 625) / 8000;
}

static int max17048_bat_is_exist(struct udevice *dev)
{
	/* MAX17048 does not provide a dedicated battery-presence status. */
	return 1;
}

static int max17048_capability(struct udevice *dev)
{
	return FG_CAP_FUEL_GAUGE;
}

static struct dm_fuel_gauge_ops max17048_fg_ops = {
	.bat_is_exist = max17048_bat_is_exist,
	.get_soc = max17048_get_soc,
	.get_voltage = max17048_get_voltage,
	.capability = max17048_capability,
};

static int max17048_fg_probe(struct udevice *dev)
{
	struct max17048_info *max17048 = dev_get_priv(dev);
	int ret;

	max17048->dev = dev;

	ret = max17048_read_word(max17048, MAX17048_REG_VERSION,
				  &max17048->version);
	if (ret)
		return ret;

	printf("MAX17048 fuel gauge, version 0x%04x\n", max17048->version);

	return 0;
}

static const struct udevice_id max17048_ids[] = {
	{ .compatible = "maxim,max17048" },
	{ }
};

U_BOOT_DRIVER(max17048_fg) = {
	.name = "max17048_fg",
	.id = UCLASS_FG,
	.of_match = max17048_ids,
	.probe = max17048_fg_probe,
	.ops = &max17048_fg_ops,
	.priv_auto_alloc_size = sizeof(struct max17048_info),
};
