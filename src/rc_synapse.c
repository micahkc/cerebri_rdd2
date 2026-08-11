/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * RC from the ManualControl topic over the serial transport. The ground
 * station sends ManualControlData frames at a fixed cadence; each one is
 * decoded here and replayed as input events on the virtual synapse-rc
 * device, so everything downstream (rc_input.c staging, staleness, arming)
 * is identical to a hardware receiver. Mirrors the lockstep RC path,
 * including its arm/mode channel assignment.
 */

#include "rc_synapse.h"

#include "rc_input.h"
#include "topic_bus.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/input/input.h>

#include <csyn/csyn_codec.h>
#include <csyn/csyn_types.h>

#define RC_NODE DT_ALIAS(rc)

void rdd2_rc_synapse_rx(const uint8_t *payload, size_t len)
{
	const struct device *const rc_dev = DEVICE_DT_GET(RC_NODE);
	struct csyn_manual_control manual = {0};
	rdd2_rc_channels_t rc = {0};
	int32_t *channels = rdd2_topic_rc_channels_data(&rc);

	if (!csyn_decode_manual_control(payload, len, &manual.rc, &manual.valid)) {
		return;
	}

	if (!device_is_ready(rc_dev)) {
		return;
	}

	memcpy(channels, csyn_rc_channels_data(&manual.rc), sizeof(rc));
	/* RDD2's existing controller assigns arm/mode to channels 4/5. */
	channels[4] = manual.rc.ch6;
	channels[5] = manual.rc.ch4;

	for (size_t i = 0; i < 16U; i++) {
		(void)input_report_abs(rc_dev, (uint16_t)(i + 1U), channels[i], false, K_FOREVER);
	}

	(void)input_report(rc_dev, INPUT_EV_MSC, RDD2_RC_INPUT_EVENT_LINK_QUALITY,
			   manual.valid ? 100 : 0, false, K_FOREVER);
	(void)input_report(rc_dev, INPUT_EV_MSC, RDD2_RC_INPUT_EVENT_VALID, manual.valid ? 1 : 0,
			   true, K_FOREVER);
}
