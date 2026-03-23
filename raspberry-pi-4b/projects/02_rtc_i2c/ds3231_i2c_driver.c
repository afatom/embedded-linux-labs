/*
 * Basic I2C client device driver for DS3231, a low-cost, extremely accurate I2C
 * real-time clock (RTC) chip.
 * This driver utilizes the kernel I2C subsystem.
 *
 * Functionalities:
 *      1) Getting current date and time from the RTC via sysfs attribute "cur_date_time" exposed at:
 *         /sys/bus/i2c/devices/<bus>-<addr>/cur_date_time (/sys/bus/i2c/drivers/my-ds3231-i2c-driver/1-0068/cur_date_time)
 * 		example output: 2026-03-16 (wday=2)  TIME: 13:38:56
 *      2) Setting date and time on the RTC via sysfs attribute "set_date_time" exposed at:
 *         /sys/bus/i2c/devices/<bus>-<addr>/set_date_time (/sys/bus/i2c/drivers/my-ds3231-i2c-driver/1-0068/set_date_time)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/bcd.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/types.h>

#define NUM_DS_DEVS 2
#define DS3231_I2C_ADDR (0x68)
#define DS3231_REG_SEC_ADDR (0x00)
#define DS3231_REG_MIN_ADDR (0x01)
#define DS3231_REG_HOUR_ADDR (0x02)
#define DS3231_REG_DAY_ADDR (0x03)
#define DS3231_REG_DATE_ADDR (0x04)
#define DS3231_REG_MONTH_ADDR (0x05)
#define DS3231_REG_YEAR_ADDR (0x06)
#define DS3231_REG_ALARM1_SEC_ADDR (0x07)
#define DS3231_REG_ALARM1_MIN_ADDR (0x08)
#define DS3231_REG_ALARM1_HOUR_ADDR (0x09)
#define DS3231_REG_ALARM1_DAY_DATE_ADDR (0x0A)
#define DS3231_CTRL_REG_ADDR (0x0E)
#define DS3231_CTRL_STAT_REG_ADDR (0x0F)
#define DS3231_STAT_BIT_OSF (0x80)

struct meta_data {
	char dev_name[20];
	int dev_id;
	u8 majorVersion;
	u8 minorVersion;
};

struct ds3231_client {
	struct i2c_client *client;
};

static int ds3231_clk_init(void);

#ifdef RTC_CLK_INIT
static struct tm tm;
static time64_t now;
static char aux_buffer[128];
#endif

static const char *const driver_name = "my-ds3231-i2c-driver";

/* sysfs show functions */
static ssize_t cur_date_time_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct rtc_time tmp;
	struct i2c_client *client = to_i2c_client(dev);
	int stat;
	u8 sec, min, hour, mday, wday, mon_cent, year, control, status;

	/* Read control register */
	stat = i2c_smbus_read_byte_data(client, DS3231_CTRL_REG_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read control reg\n", driver_name);
		return stat;
	}
	control = stat;

	/* Read status register */
	stat = i2c_smbus_read_byte_data(client, DS3231_CTRL_STAT_REG_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read status reg\n", driver_name);
		return stat;
	}
	status = stat;

	/* Read time and date registers */
	stat = i2c_smbus_read_byte_data(client, DS3231_REG_SEC_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read seconds\n", driver_name);
		return stat;
	}
	sec = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_MIN_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read minutes\n", driver_name);
		return stat;
	}
	min = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_HOUR_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read hours\n", driver_name);
		return stat;
	}
	hour = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_DAY_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read day\n", driver_name);
		return stat;
	}
	wday = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_DATE_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read date\n", driver_name);
		return stat;
	}
	mday = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_MONTH_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read month\n", driver_name);
		return stat;
	}
	mon_cent = stat;

	stat = i2c_smbus_read_byte_data(client, DS3231_REG_YEAR_ADDR);
	if (stat < 0) {
		pr_err("%s: failed to read year\n", driver_name);
		return stat;
	}
	year = stat;

	if (status & DS3231_STAT_BIT_OSF) {
		pr_warn("%s: RTC oscillator has stopped. Clearing OSF flag\n",
			driver_name);
		stat = i2c_smbus_write_byte_data(client,
						 DS3231_CTRL_STAT_REG_ADDR,
						 status & ~DS3231_STAT_BIT_OSF);
		if (stat < 0) {
			pr_err("%s: Failed to clear OSF flag\n", driver_name);
			return stat;
		}
	}

	tmp.tm_sec = bcd2bin(sec & 0x7F);
	tmp.tm_min = bcd2bin(min & 0x7F);
	tmp.tm_hour = bcd2bin(hour & 0x3F);
	tmp.tm_mday = bcd2bin(mday & 0x3F);
	tmp.tm_mon = bcd2bin(mon_cent & 0x1F);
	tmp.tm_year = bcd2bin(year) + ((mon_cent & 0x80) ? 2000 : 1900);
	tmp.tm_wday = bcd2bin(wday & 0x07);
	tmp.tm_yday = 0;
	tmp.tm_isdst = 0;

	pr_info("%s: Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
		driver_name, tmp.tm_year, tmp.tm_mon, tmp.tm_mday, tmp.tm_wday,
		tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
	return scnprintf(buf, PAGE_SIZE,
			 "%4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
			 tmp.tm_year, tmp.tm_mon, tmp.tm_mday, tmp.tm_wday,
			 tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
}

static ssize_t set_date_time_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	u32 wday; /* week day */
	u32 mday; /* month day */
	u32 month;
	u32 year;
	u32 hour;
	u32 minute;
	u32 sec;
	int stat;
	u32 century;

	/*
	 * Format: date: MM/DD/YYYY (wday=X) time: HH:MM:SS
	 * Example: date: 03/16/2026 (wday=2) time: 13:38:56
	 * wday: [1-7], 1=Sunday
	 * Basic input validation (handles most cases for demonstration)
	 */
	if (sscanf(buf, "date: %02u/%02u/%4u (wday=%1u) time: %2u:%02u:%02u",
		   &month, &mday, &year, &wday, &hour, &minute, &sec) != 7 ||
	    mday > 31 || wday > 7 || month > 12 || hour > 23 || minute > 59 ||
	    sec > 59 || year < 2026) {
		pr_err("%s: Invalid date time format\n", driver_name);
		return -EINVAL;
	}

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_YEAR_ADDR,
					 bin2bcd(year % 100));
	if (stat < 0)
		return stat;

	century = (year >= 2000) ? 0x80 : 0;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_MONTH_ADDR,
					 bin2bcd(month | century));
	if (stat < 0)
		return stat;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_HOUR_ADDR,
					 bin2bcd(hour));
	if (stat < 0)
		return stat;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_MIN_ADDR,
					 bin2bcd(minute));
	if (stat < 0)
		return stat;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_SEC_ADDR,
					 bin2bcd(sec));
	if (stat < 0)
		return stat;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_DAY_ADDR,
					 bin2bcd(wday));
	if (stat < 0)
		return stat;

	stat = i2c_smbus_write_byte_data(client, DS3231_REG_DATE_ADDR,
					 bin2bcd(mday));
	if (stat < 0)
		return stat;

	return count;
}

static DEVICE_ATTR_RO(cur_date_time);
static DEVICE_ATTR_WO(set_date_time);

static struct attribute *ds3231_attrs[] = {
	&dev_attr_cur_date_time.attr,
	&dev_attr_set_date_time.attr,
	NULL,
};

static const struct attribute_group ds3231_group = {
	.attrs = ds3231_attrs,
};

static int ds3231_probe(struct i2c_client *client)
{
	struct ds3231_client *dc;
	int stat;
	const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
	const char *name = dev_id ? dev_id->name : "my-ds3231";

	dev_info(&client->dev, "probe: matched id=%s\n", name);

	dc = devm_kzalloc(&client->dev, sizeof(*dc), GFP_KERNEL);
	if (!dc)
		return -ENOMEM;

	dc->client = client;

	i2c_set_clientdata(client, dc);

	ds3231_clk_init();

	/* Create sysfs attributes on device */
	stat = sysfs_create_group(&client->dev.kobj, &ds3231_group);
	if (stat) {
		dev_err(&client->dev, "failed to create sysfs group: %d\n",
			stat);
		return stat;
	}

	dev_info(&client->dev, "DS3231 RTC probed and sysfs created\n");

	return 0;
}

static void ds3231_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &ds3231_group);
	dev_info(&client->dev, "removed device\n");
}

/**
 * Initialize RTC clock by reading current time from kernel timekeeper, parsing it and setting the RTC time accordingly.
 * cuurently, this function only reads and prints the current time from kernel timekeeper for demonstration purposes.
 * This is just for demonstration and can be extended to set the RTC time on initialization.
*/
static int ds3231_clk_init(void)
{
	int len = 0;

#ifdef RTC_CLK_INIT
	/* Get current UTC time from kernel timekeeper */
	now = ktime_get_real_seconds();

	/* Convert to broken-down time (struct tm) */
	time64_to_tm(now, 0, &tm);
	pr_info("%s: Current Date: %04ld-%02d-%02d Current Time (UTC): %02d:%02d:%02d\n",
		driver_name, (long)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	len = snprintf(
		aux_buffer, sizeof(aux_buffer),
		"Current Date: %04ld-%02d-%02d\nCurrent Time (UTC): %02d:%02d:%02d\n",
		(long)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		tm.tm_min, tm.tm_sec);
#endif
	return len;
}

static struct meta_data driver_meta = {
	.dev_name = "DS3231-RTC-time-date",
	.dev_id = 1,
	.majorVersion = 1,
	.minorVersion = 0,
};

static const struct i2c_device_id ds3231_ids[] = {
	{ .name = "my-ds3231", .driver_data = (kernel_ulong_t)&driver_meta },
	{} /* sentinel */
};

MODULE_DEVICE_TABLE(i2c, ds3231_ids);

static struct i2c_adapter *ds3231_rtc_adapter;
static struct i2c_client *ds3231_rtc_client;

static struct i2c_board_info ds3231_i2c_device_info = {
	I2C_BOARD_INFO("my-ds3231", DS3231_I2C_ADDR),
};

/* I2C driver struct */
static struct i2c_driver ds3231_driver = {
	.driver = {
		.name = driver_name,
	},
	.probe = ds3231_probe,
	.remove = ds3231_remove,
	.id_table = ds3231_ids,
};

static int __init ds3231_init(void)
{
	int ret;

	ret = i2c_add_driver(&ds3231_driver);
	if (ret) {
		pr_err("%s: failed to add driver\n", driver_name);
		return ret;
	}

	pr_info("%s: I2C driver registered\n", driver_name);

	/* Get I2C adapter (i2c-1) */
	ds3231_rtc_adapter = i2c_get_adapter(1);
	if (!ds3231_rtc_adapter) {
		pr_err("%s: failed to get adapter\n", driver_name);
		i2c_del_driver(&ds3231_driver);
		return -ENODEV;
	}

	/* Create the I2C device */
	ds3231_rtc_client = i2c_new_client_device(ds3231_rtc_adapter,
						  &ds3231_i2c_device_info);
	i2c_put_adapter(ds3231_rtc_adapter);

	if (IS_ERR(ds3231_rtc_client)) {
		pr_err("%s: failed to create I2C device\n", driver_name);
		i2c_del_driver(&ds3231_driver);
		return PTR_ERR(ds3231_rtc_client);
	}

	pr_info("%s: I2C device created and probed\n", driver_name);

	return 0;
}

static void __exit ds3231_exit(void)
{
	if (ds3231_rtc_client)
		i2c_unregister_device(ds3231_rtc_client);

	i2c_del_driver(&ds3231_driver);
	pr_info("%s: driver unloaded\n", driver_name);
}

module_init(ds3231_init);
module_exit(ds3231_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("DS3231 I2C real-time clock (RTC) device driver");
