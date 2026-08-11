#ifndef RDD2_BATTERY_H_
#define RDD2_BATTERY_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * Battery state published by subsys/battery. Voltage is in centivolts to
 * match VehicleHealthData.voltage_battery_cv; remaining_pct is -1 while
 * unknown, matching the catalog convention. valid stays false until the
 * first successful ADC conversion, so consumers can tell "no monitor" and
 * "monitor not yet sampled" apart from a real reading.
 */

#if defined(CONFIG_RDD2_BATTERY)

void rdd2_battery_health_get(uint16_t *voltage_cv, int8_t *remaining_pct, bool *valid);
bool rdd2_battery_low(void);

#else

static inline void rdd2_battery_health_get(uint16_t *voltage_cv, int8_t *remaining_pct,
					   bool *valid)
{
	*voltage_cv = 0U;
	*remaining_pct = -1;
	*valid = false;
}

static inline bool rdd2_battery_low(void)
{
	return false;
}

#endif

#endif
