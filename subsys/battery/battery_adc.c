/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Low-rate battery voltage monitor. Samples the vbat voltage-divider alias
 * every RDD2_BATTERY_PERIOD_MS from the system work queue, scales the ADC
 * millivolts back through the divider from its devicetree ratio, and keeps a
 * debounced low-voltage flag: sag spikes from stick punches must persist for
 * RDD2_BATTERY_LOW_DEBOUNCE consecutive samples before the flag asserts, and
 * it clears the same way.
 */

#include "rdd2_battery.h"

#include <zephyr/drivers/adc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(rdd2, LOG_LEVEL_INF);

#define VBAT_NODE DT_ALIAS(vbat)

#if !DT_NODE_EXISTS(VBAT_NODE)
#error "CONFIG_RDD2_BATTERY needs a vbat devicetree alias naming a voltage-divider node"
#endif

#define RDD2_BATTERY_PERIOD_MS     500
#define RDD2_BATTERY_LOW_DEBOUNCE  4

static const struct adc_dt_spec g_vbat_adc = ADC_DT_SPEC_GET(VBAT_NODE);

#define VBAT_FULL_OHMS   DT_PROP(VBAT_NODE, full_ohms)
#define VBAT_OUTPUT_OHMS DT_PROP(VBAT_NODE, output_ohms)

static struct k_spinlock g_battery_lock;
static uint32_t g_battery_mv;
static bool g_battery_valid;
static bool g_battery_low;
static uint8_t g_battery_low_streak;
static uint8_t g_battery_ok_streak;

static void battery_sample_work(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(g_battery_work, battery_sample_work);

static int battery_read_mv(uint32_t *battery_mv)
{
	int16_t raw;
	struct adc_sequence sequence = {
		.buffer = &raw,
		.buffer_size = sizeof(raw),
	};
	int32_t mv;
	int rc;

	adc_sequence_init_dt(&g_vbat_adc, &sequence);

	rc = adc_read_dt(&g_vbat_adc, &sequence);
	if (rc != 0) {
		return rc;
	}

	mv = raw;
	rc = adc_raw_to_millivolts_dt(&g_vbat_adc, &mv);
	if (rc != 0) {
		return rc;
	}
	if (mv < 0) {
		mv = 0;
	}

	*battery_mv = (uint32_t)mv * VBAT_FULL_OHMS / VBAT_OUTPUT_OHMS;
	return 0;
}

static void battery_sample_work(struct k_work *work)
{
	uint32_t battery_mv;
	int rc = battery_read_mv(&battery_mv);

	if (rc == 0) {
		k_spinlock_key_t key = k_spin_lock(&g_battery_lock);

		g_battery_mv = battery_mv;
		g_battery_valid = true;

		if (battery_mv < CONFIG_RDD2_BATTERY_LOW_MV) {
			g_battery_ok_streak = 0U;
			if (g_battery_low_streak < RDD2_BATTERY_LOW_DEBOUNCE) {
				g_battery_low_streak++;
			}
			if (!g_battery_low &&
			    g_battery_low_streak >= RDD2_BATTERY_LOW_DEBOUNCE) {
				g_battery_low = true;
				k_spin_unlock(&g_battery_lock, key);
				LOG_WRN("battery low: %u mV", battery_mv);
				goto reschedule;
			}
		} else {
			g_battery_low_streak = 0U;
			if (g_battery_ok_streak < RDD2_BATTERY_LOW_DEBOUNCE) {
				g_battery_ok_streak++;
			}
			if (g_battery_low &&
			    g_battery_ok_streak >= RDD2_BATTERY_LOW_DEBOUNCE) {
				g_battery_low = false;
			}
		}
		k_spin_unlock(&g_battery_lock, key);
	}

reschedule:
	(void)k_work_reschedule(k_work_delayable_from_work(work),
				K_MSEC(RDD2_BATTERY_PERIOD_MS));
}

void rdd2_battery_health_get(uint16_t *voltage_cv, int8_t *remaining_pct, bool *valid)
{
	k_spinlock_key_t key = k_spin_lock(&g_battery_lock);
	uint32_t mv = g_battery_mv;
	bool have = g_battery_valid;

	k_spin_unlock(&g_battery_lock, key);

	*valid = have;
	if (!have) {
		*voltage_cv = 0U;
		*remaining_pct = -1;
		return;
	}

	*voltage_cv = (uint16_t)MIN(mv / 10U, UINT16_MAX);

	if (mv <= CONFIG_RDD2_BATTERY_EMPTY_MV) {
		*remaining_pct = 0;
	} else if (mv >= CONFIG_RDD2_BATTERY_FULL_MV) {
		*remaining_pct = 100;
	} else {
		*remaining_pct =
			(int8_t)((mv - CONFIG_RDD2_BATTERY_EMPTY_MV) * 100U /
				 (CONFIG_RDD2_BATTERY_FULL_MV - CONFIG_RDD2_BATTERY_EMPTY_MV));
	}
}

bool rdd2_battery_low(void)
{
	k_spinlock_key_t key = k_spin_lock(&g_battery_lock);
	bool low = g_battery_low;

	k_spin_unlock(&g_battery_lock, key);
	return low;
}

static int battery_init(void)
{
	int rc;

	if (!adc_is_ready_dt(&g_vbat_adc)) {
		LOG_ERR("battery adc not ready");
		return -ENODEV;
	}

	rc = adc_channel_setup_dt(&g_vbat_adc);
	if (rc != 0) {
		LOG_ERR("battery adc channel setup failed: %d", rc);
		return rc;
	}

	(void)k_work_schedule(&g_battery_work, K_MSEC(RDD2_BATTERY_PERIOD_MS));
	return 0;
}

SYS_INIT(battery_init, APPLICATION, 90);
