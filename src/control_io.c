/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "control_io.h"

#include "imu_stream.h"
#include "motor_output.h"
#include "rc_input.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/misc/nxp_flexio_dshot/nxp_flexio_dshot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(rdd2, LOG_LEVEL_INF);

#define RC_NODE    DT_ALIAS(rc)
#define IMU_NODE   DT_ALIAS(imu0)
#define MOTOR_NODE DT_ALIAS(motors)

static bool ready_or_log(const struct device *dev, const char *name)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("%s not ready", name);
		return false;
	}

	return true;
}

int rdd2_control_io_init(void)
{
	const struct device *const rc_dev = DEVICE_DT_GET(RC_NODE);
	const struct device *const imu_dev = DEVICE_DT_GET(IMU_NODE);

	printk("rdd2 init:   rc input\n");
	rdd2_rc_input_init();

	ready_or_log(rc_dev, "rc");
	ready_or_log(imu_dev, "imu");
	printk("rdd2 init:   devices checked (rc=%d imu=%d)\n",
	       device_is_ready(rc_dev), device_is_ready(imu_dev));

	/* GNSS is not a control input and brings itself up in
	 * subsys/gnss_source, which reports through `gnss status`. */

#if defined(CONFIG_RDD2_DSHOT)
	{
		const struct device *const motor_dev = DEVICE_DT_GET(MOTOR_NODE);

		if (!ready_or_log(motor_dev, "dshot")) {
			return -ENODEV;
		}

		if (nxp_flexio_dshot_channel_count(motor_dev) != 4U) {
			LOG_ERR("expected 4 dshot channels");
			return -EINVAL;
		}
	}
#else
	if (!rdd2_motor_output_ready()) {
		printk("rdd2 init:   motor outputs NOT ready\n");
		LOG_ERR("motor outputs not ready");
		return -ENODEV;
	}
#endif

	printk("rdd2 init:   imu stream\n");
	return rdd2_imu_stream_init();
}

void rdd2_control_input_wait(rdd2_vec3f_t *gyro, rdd2_vec3f_t *accel,
			     rdd2_rc_channels_t *rc,
			     rdd2_control_status_t *status, float *dt,
			     uint64_t *imu_interrupt_timestamp_ns)
{
	bool rc_valid;

	status->imu_ok = rdd2_imu_stream_wait_next(gyro, accel, dt, imu_interrupt_timestamp_ns);
	status->rc_link_quality = rdd2_rc_input_link_quality_get(DEVICE_DT_GET(RC_NODE));
	rdd2_rc_input_latest_get(rc, &status->rc_stamp_ms, &rc_valid);
	status->rc_valid = rc_valid;
}
