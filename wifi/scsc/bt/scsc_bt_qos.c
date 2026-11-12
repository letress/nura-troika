/****************************************************************************
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved
 *
 * Bluetooth Quality of Service
 *
 * Linux 7.3 port: the vendor PM QoS classes (device/bus throughput and
 * cluster minimum frequency) are not part of mainline.  The shared-memory
 * transport keeps calling these hooks, so they remain as no-ops instead of
 * pulling the whole vendor interconnect framework into the port.
 *
 ****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include <scsc/scsc_logring.h>

#include "scsc_bt_priv.h"

void scsc_bt_qos_init(void)
{
	SCSC_TAG_DEBUG(BT_COMMON, "QoS boosting not available on this kernel\n");
}

void scsc_bt_qos_deinit(void)
{
}

void scsc_bt_qos_service_stop(void)
{
}

void scsc_bt_qos_update(uint32_t number_of_outstanding_hci_events,
			uint32_t number_of_outstanding_acl_packets)
{
}
