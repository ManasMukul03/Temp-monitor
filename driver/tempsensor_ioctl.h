/*
 * tempsensor_ioctl.h
 *
 * Shared between the kernel driver and the user-space application.
 * Defines the simulated register bits and the ioctl commands, so both
 * sides always agree on the interface.
 */
#ifndef TEMPSENSOR_IOCTL_H
#define TEMPSENSOR_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define TEMPSENSOR_DEVICE_PATH  "/dev/tempsensor"

/* ---------------- Simulated hardware registers ----------------
 * A real sensor chip is controlled through registers. The driver keeps
 * the same four registers in memory:
 *   CTRL     - control register (what the software asks the device to do)
 *   STATUS   - status register  (what the device reports back)
 *   DATA     - latest temperature in milli-degrees Celsius
 *   INTERVAL - sampling interval in milliseconds
 */

/* CTRL register bits */
#define TS_CTRL_ENABLE        (1u << 0)   /* 1 = sensor is sampling */
#define TS_CTRL_COOLING       (1u << 1)   /* 1 = cooling fan switched on */

/* STATUS register bits */
#define TS_STATUS_DATA_READY  (1u << 0)   /* new reading not yet read */
#define TS_STATUS_OVERHEAT    (1u << 1)   /* temperature above limit */
#define TS_STATUS_ENABLED     (1u << 2)   /* copy of CTRL enable bit */
#define TS_STATUS_COOLING     (1u << 3)   /* copy of CTRL cooling bit */

#define TS_OVERHEAT_LIMIT_MC  60000       /* 60.000 degC */
#define TS_MIN_INTERVAL_MS    100
#define TS_MAX_INTERVAL_MS    10000

/* Statistics kept inside the driver */
struct tempsensor_stats {
	__s32 min_mc;      /* lowest reading, milli-degC  */
	__s32 max_mc;      /* highest reading, milli-degC */
	__u32 count;       /* number of readings taken    */
};

/* ---------------- ioctl commands ----------------
 * _IO   : no data,  _IOW : user -> kernel,  _IOR : kernel -> user
 */
#define TEMPSENSOR_MAGIC  'T'

#define TS_IOC_SET_INTERVAL  _IOW(TEMPSENSOR_MAGIC, 1, __u32)
#define TS_IOC_GET_INTERVAL  _IOR(TEMPSENSOR_MAGIC, 2, __u32)
#define TS_IOC_GET_STATUS    _IOR(TEMPSENSOR_MAGIC, 3, __u32)
#define TS_IOC_GET_STATS     _IOR(TEMPSENSOR_MAGIC, 4, struct tempsensor_stats)
#define TS_IOC_RESET         _IO(TEMPSENSOR_MAGIC, 5)
#define TS_IOC_SET_ENABLE    _IOW(TEMPSENSOR_MAGIC, 6, __u32)
#define TS_IOC_SET_COOLING   _IOW(TEMPSENSOR_MAGIC, 7, __u32)

#endif /* TEMPSENSOR_IOCTL_H */
