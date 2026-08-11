#ifndef RDD2_RC_SYNAPSE_H_
#define RDD2_RC_SYNAPSE_H_

#include <stddef.h>
#include <stdint.h>

/*
 * Inbound ManualControlData handler for the serial transport: decodes the
 * wire payload and reports it as input events on the synapse-rc device that
 * the rc alias names, so rc_input.c consumes WiFi/serial RC exactly as it
 * consumes a CRSF receiver. Runs in transport-thread context.
 */
void rdd2_rc_synapse_rx(const uint8_t *payload, size_t len);

#endif
