# SPDX-License-Identifier: Apache-2.0

if(CONFIG_RDD2_BATTERY)
  target_sources(app PRIVATE
    battery_adc.c
  )
endif()
