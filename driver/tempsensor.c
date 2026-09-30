// SPDX-License-Identifier: GPL-2.0
/*
 * tempsensor.c - Virtual temperature sensor character device driver
 *
 * Creates /dev/tempsensor. A kernel timer produces a new simulated
 * temperature reading every "interval" milliseconds and stores it in
 * simulated hardware registers.
 *
 *   read()  -> returns the latest temperature as text in milli-degC
 *   ioctl() -> set/get interval, get status, get stats, reset, enable,
 *              cooling fan on/off
 *   /proc/tempsensor -> dump of all registers (for debugging/demo)
 *
 * To use a real sensor, only sensor_read_hw() needs to change.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "tempsensor_ioctl.h"

#define DRIVER_NAME "tempsensor"

/* Sampling interval can be set when loading: sudo insmod tempsensor.ko interval_ms=500 */
static unsigned int interval_ms = 1000;
module_param(interval_ms, uint, 0444);
MODULE_PARM_DESC(interval_ms, "Sampling interval in milliseconds (100-10000)");

/* ---------------------------------------------------------------
 * Device state: simulated registers + statistics
 * --------------------------------------------------------------- */
struct tempsensor_dev {
	/* simulated hardware registers */
	u32 reg_ctrl;
	u32 reg_status;
	s32 reg_data;          /* milli-degC */
	u32 reg_interval;      /* ms */

	/* statistics */
	struct tempsensor_stats stats;

	/* simulation state */
	s32 target_mc;         /* temperature the sensor is drifting towards */
	u32 ticks_to_new_target;

	/* kernel objects */
	struct timer_list timer;
	spinlock_t lock;       /* protects registers + stats (used from timer context) */
	dev_t devno;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct proc_dir_entry *proc_entry;
	atomic_t open_count;
};

static struct tempsensor_dev sensor;

/* ---------------------------------------------------------------
 * "Hardware" layer
 * The ONLY function that would change for a real sensor.
 * Simulates a temperature that slowly drifts towards a random target
 * (a changing heat load), with small noise, so readings look realistic
 * and sometimes overheat. When the cooling fan is on, the device cools
 * down towards 35 degC instead.
 * Called with sensor.lock held.
 * --------------------------------------------------------------- */
#define COOLING_TARGET_MC 35000

static s32 sensor_read_hw(void)
{
	s32 current_mc = sensor.reg_data;
	s32 target, noise;

	/* every 15-40 samples the heat load changes: new target 28-72 degC */
	if (sensor.ticks_to_new_target == 0) {
		sensor.target_mc = 28000 + (s32)get_random_u32_below(44001);
		sensor.ticks_to_new_target = 15 + get_random_u32_below(26);
	}
	sensor.ticks_to_new_target--;

	/* fan on: the device is pulled down towards the cooling target */
	target = sensor.target_mc;
	if ((sensor.reg_ctrl & TS_CTRL_COOLING) && target > COOLING_TARGET_MC)
		target = COOLING_TARGET_MC;

	/* move 10% of the way towards the target, plus noise of +/-0.3 degC */
	noise = (s32)get_random_u32_below(601) - 300;
	current_mc += (target - current_mc) / 10 + noise;

	/* physical limits of a typical sensor */
	return clamp_t(s32, current_mc, -40000, 125000);
}

/* Update DATA/STATUS registers and statistics with a new reading. Lock held. */
static void sensor_store_reading(s32 temp_mc)
{
	sensor.reg_data = temp_mc;
	sensor.reg_status |= TS_STATUS_DATA_READY;

	if (temp_mc > TS_OVERHEAT_LIMIT_MC)
		sensor.reg_status |= TS_STATUS_OVERHEAT;
	else
		sensor.reg_status &= ~TS_STATUS_OVERHEAT;

	if (sensor.stats.count == 0 || temp_mc < sensor.stats.min_mc)
		sensor.stats.min_mc = temp_mc;
	if (sensor.stats.count == 0 || temp_mc > sensor.stats.max_mc)
		sensor.stats.max_mc = temp_mc;
	sensor.stats.count++;
}

/* Timer callback: runs every reg_interval ms in softirq context */
static void sensor_timer_fn(struct timer_list *t)
{
	unsigned long flags;

	spin_lock_irqsave(&sensor.lock, flags);
	if (sensor.reg_ctrl & TS_CTRL_ENABLE) {
		sensor_store_reading(sensor_read_hw());
		mod_timer(&sensor.timer, jiffies + msecs_to_jiffies(sensor.reg_interval));
	}
	spin_unlock_irqrestore(&sensor.lock, flags);
}

/* Reset the simulated sensor to its power-on state. Lock held. */
static void sensor_reset_state(void)
{
	sensor.reg_data = 32000;               /* start at 32 degC */
	sensor.reg_ctrl &= ~TS_CTRL_COOLING;   /* fan off */
	sensor.reg_status &= ~(TS_STATUS_DATA_READY | TS_STATUS_OVERHEAT | TS_STATUS_COOLING);
	sensor.stats.min_mc = 0;
	sensor.stats.max_mc = 0;
	sensor.stats.count = 0;
	sensor.target_mc = 32000;
	sensor.ticks_to_new_target = 0;
}

static void sensor_set_enable(bool enable)
{
	unsigned long flags;

	spin_lock_irqsave(&sensor.lock, flags);
	if (enable) {
		sensor.reg_ctrl |= TS_CTRL_ENABLE;
		sensor.reg_status |= TS_STATUS_ENABLED;
		mod_timer(&sensor.timer, jiffies + msecs_to_jiffies(sensor.reg_interval));
	} else {
		sensor.reg_ctrl &= ~TS_CTRL_ENABLE;
		sensor.reg_status &= ~TS_STATUS_ENABLED;
	}
	spin_unlock_irqrestore(&sensor.lock, flags);
	/* when disabled, the timer simply isn't re-armed by its callback */
}

/* ---------------------------------------------------------------
 * File operations: what happens when user space calls
 * open(), read(), ioctl(), close() on /dev/tempsensor
 * --------------------------------------------------------------- */
static int ts_open(struct inode *inode, struct file *file)
{
	atomic_inc(&sensor.open_count);
	pr_info(DRIVER_NAME ": device opened (users: %d)\n", atomic_read(&sensor.open_count));
	return 0;
}

static int ts_release(struct inode *inode, struct file *file)
{
	atomic_dec(&sensor.open_count);
	pr_info(DRIVER_NAME ": device closed\n");
	return 0;
}

/*
 * read(): returns the latest temperature as text, e.g. "36250\n".
 * Uses *ppos so "cat /dev/tempsensor" prints one value and stops.
 * The application uses pread(fd, buf, len, 0) to always read fresh data.
 */
static ssize_t ts_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char text[16];
	int len;
	unsigned long flags;

	spin_lock_irqsave(&sensor.lock, flags);
	len = scnprintf(text, sizeof(text), "%d\n", sensor.reg_data);
	sensor.reg_status &= ~TS_STATUS_DATA_READY;   /* reading clears the flag */
	spin_unlock_irqrestore(&sensor.lock, flags);

	/* copies to user space safely and updates *ppos */
	return simple_read_from_buffer(buf, count, ppos, text, len);
}

static long ts_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	unsigned long flags;
	struct tempsensor_stats stats_copy;
	u32 value;

	switch (cmd) {
	case TS_IOC_SET_INTERVAL:
		if (copy_from_user(&value, uarg, sizeof(value)))
			return -EFAULT;
		if (value < TS_MIN_INTERVAL_MS || value > TS_MAX_INTERVAL_MS)
			return -EINVAL;
		spin_lock_irqsave(&sensor.lock, flags);
		sensor.reg_interval = value;
		spin_unlock_irqrestore(&sensor.lock, flags);
		pr_info(DRIVER_NAME ": interval set to %u ms\n", value);
		return 0;

	case TS_IOC_GET_INTERVAL:
		spin_lock_irqsave(&sensor.lock, flags);
		value = sensor.reg_interval;
		spin_unlock_irqrestore(&sensor.lock, flags);
		return copy_to_user(uarg, &value, sizeof(value)) ? -EFAULT : 0;

	case TS_IOC_GET_STATUS:
		spin_lock_irqsave(&sensor.lock, flags);
		value = sensor.reg_status;
		spin_unlock_irqrestore(&sensor.lock, flags);
		return copy_to_user(uarg, &value, sizeof(value)) ? -EFAULT : 0;

	case TS_IOC_GET_STATS:
		spin_lock_irqsave(&sensor.lock, flags);
		stats_copy = sensor.stats;
		spin_unlock_irqrestore(&sensor.lock, flags);
		return copy_to_user(uarg, &stats_copy, sizeof(stats_copy)) ? -EFAULT : 0;

	case TS_IOC_RESET:
		spin_lock_irqsave(&sensor.lock, flags);
		sensor_reset_state();
		spin_unlock_irqrestore(&sensor.lock, flags);
		pr_info(DRIVER_NAME ": sensor reset\n");
		return 0;

	case TS_IOC_SET_ENABLE:
		if (copy_from_user(&value, uarg, sizeof(value)))
			return -EFAULT;
		sensor_set_enable(value != 0);
		pr_info(DRIVER_NAME ": sensor %s\n", value ? "enabled" : "disabled");
		return 0;

	case TS_IOC_SET_COOLING:
		if (copy_from_user(&value, uarg, sizeof(value)))
			return -EFAULT;
		spin_lock_irqsave(&sensor.lock, flags);
		if (value) {
			sensor.reg_ctrl |= TS_CTRL_COOLING;
			sensor.reg_status |= TS_STATUS_COOLING;
		} else {
			sensor.reg_ctrl &= ~TS_CTRL_COOLING;
			sensor.reg_status &= ~TS_STATUS_COOLING;
		}
		spin_unlock_irqrestore(&sensor.lock, flags);
		pr_info(DRIVER_NAME ": cooling fan %s\n", value ? "ON" : "OFF");
		return 0;

	default:
		return -ENOTTY;   /* standard error for unknown ioctl */
	}
}

static const struct file_operations ts_fops = {
	.owner          = THIS_MODULE,
	.open           = ts_open,
	.release        = ts_release,
	.read           = ts_read,
	.unlocked_ioctl = ts_ioctl,
};

/* ---------------------------------------------------------------
 * /proc/tempsensor : human-readable register dump
 * --------------------------------------------------------------- */
static int ts_proc_show(struct seq_file *m, void *v)
{
	struct tempsensor_dev snap;
	unsigned long flags;

	spin_lock_irqsave(&sensor.lock, flags);
	snap.reg_ctrl = sensor.reg_ctrl;
	snap.reg_status = sensor.reg_status;
	snap.reg_data = sensor.reg_data;
	snap.reg_interval = sensor.reg_interval;
	snap.stats = sensor.stats;
	spin_unlock_irqrestore(&sensor.lock, flags);

	seq_puts(m, "Virtual Temperature Sensor - register dump\n");
	seq_printf(m, "CTRL     : 0x%08x  (enable=%u cooling=%u)\n", snap.reg_ctrl,
		   !!(snap.reg_ctrl & TS_CTRL_ENABLE),
		   !!(snap.reg_ctrl & TS_CTRL_COOLING));
	seq_printf(m, "STATUS   : 0x%08x  (data_ready=%u overheat=%u enabled=%u cooling=%u)\n",
		   snap.reg_status,
		   !!(snap.reg_status & TS_STATUS_DATA_READY),
		   !!(snap.reg_status & TS_STATUS_OVERHEAT),
		   !!(snap.reg_status & TS_STATUS_ENABLED),
		   !!(snap.reg_status & TS_STATUS_COOLING));
	seq_printf(m, "DATA     : %d m°C  (%d.%03d °C)\n", snap.reg_data,
		   snap.reg_data / 1000, abs(snap.reg_data % 1000));
	seq_printf(m, "INTERVAL : %u ms\n", snap.reg_interval);
	seq_printf(m, "STATS    : min=%d max=%d count=%u\n",
		   snap.stats.min_mc, snap.stats.max_mc, snap.stats.count);
	seq_printf(m, "DEVICE   : major=%d minor=%d open_count=%d\n",
		   MAJOR(sensor.devno), MINOR(sensor.devno), atomic_read(&sensor.open_count));
	return 0;
}

static int ts_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ts_proc_show, NULL);
}

static const struct proc_ops ts_proc_ops = {
	.proc_open    = ts_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* Make /dev/tempsensor readable/writable by normal users (mode 0666) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
static char *ts_devnode(const struct device *dev, umode_t *mode)
#else
static char *ts_devnode(struct device *dev, umode_t *mode)
#endif
{
	if (mode)
		*mode = 0666;
	return NULL;
}

/* ---------------------------------------------------------------
 * Module load / unload
 * --------------------------------------------------------------- */
static int __init tempsensor_init(void)
{
	int ret;

	if (interval_ms < TS_MIN_INTERVAL_MS || interval_ms > TS_MAX_INTERVAL_MS)
		interval_ms = 1000;

	spin_lock_init(&sensor.lock);
	atomic_set(&sensor.open_count, 0);
	sensor.reg_interval = interval_ms;
	sensor_reset_state();

	/* 1. get a major/minor number */
	ret = alloc_chrdev_region(&sensor.devno, 0, 1, DRIVER_NAME);
	if (ret) {
		pr_err(DRIVER_NAME ": alloc_chrdev_region failed (%d)\n", ret);
		return ret;
	}

	/* 2. register our file operations with that number */
	cdev_init(&sensor.cdev, &ts_fops);
	sensor.cdev.owner = THIS_MODULE;
	ret = cdev_add(&sensor.cdev, sensor.devno, 1);
	if (ret) {
		pr_err(DRIVER_NAME ": cdev_add failed (%d)\n", ret);
		goto err_region;
	}

	/* 3. create /sys/class/tempsensor and /dev/tempsensor */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	sensor.class = class_create(DRIVER_NAME);
#else
	sensor.class = class_create(THIS_MODULE, DRIVER_NAME);
#endif
	if (IS_ERR(sensor.class)) {
		ret = PTR_ERR(sensor.class);
		goto err_cdev;
	}
	sensor.class->devnode = ts_devnode;

	sensor.device = device_create(sensor.class, NULL, sensor.devno, NULL, DRIVER_NAME);
	if (IS_ERR(sensor.device)) {
		ret = PTR_ERR(sensor.device);
		goto err_class;
	}

	/* 4. /proc/tempsensor (optional; failure is not fatal) */
	sensor.proc_entry = proc_create(DRIVER_NAME, 0444, NULL, &ts_proc_ops);
	if (!sensor.proc_entry)
		pr_warn(DRIVER_NAME ": could not create /proc entry\n");

	/* 5. start the sampling timer */
	timer_setup(&sensor.timer, sensor_timer_fn, 0);
	sensor_set_enable(true);

	pr_info(DRIVER_NAME ": loaded, /dev/%s major=%d minor=%d interval=%u ms\n",
		DRIVER_NAME, MAJOR(sensor.devno), MINOR(sensor.devno), sensor.reg_interval);
	return 0;

err_class:
	class_destroy(sensor.class);
err_cdev:
	cdev_del(&sensor.cdev);
err_region:
	unregister_chrdev_region(sensor.devno, 1);
	return ret;
}

static void __exit tempsensor_exit(void)
{
	sensor_set_enable(false);
	timer_delete_sync(&sensor.timer);   /* wait until the timer is not running */

	if (sensor.proc_entry)
		proc_remove(sensor.proc_entry);
	device_destroy(sensor.class, sensor.devno);
	class_destroy(sensor.class);
	cdev_del(&sensor.cdev);
	unregister_chrdev_region(sensor.devno, 1);

	pr_info(DRIVER_NAME ": unloaded\n");
}

module_init(tempsensor_init);
module_exit(tempsensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manas Mukul");
MODULE_DESCRIPTION("Virtual temperature sensor character device driver");
MODULE_VERSION("1.0");
