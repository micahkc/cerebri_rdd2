/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual RC input device fed by rc_synapse.c from inbound ManualControl
 * frames. Exists so the rc alias names a real device and rc_input.c stays
 * source-agnostic, exactly like the lockstep RC device in simulation.
 */

#define DT_DRV_COMPAT cognipilot_synapse_rc

#include <zephyr/device.h>

static int synapse_rc_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#define RDD2_SYNAPSE_RC_INIT(inst)                                                                 \
	DEVICE_DT_INST_DEFINE(inst, synapse_rc_init, NULL, NULL, NULL, POST_KERNEL,                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL)

DT_INST_FOREACH_STATUS_OKAY(RDD2_SYNAPSE_RC_INIT)
