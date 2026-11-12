/****************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 * BT BlueZ interface
 *
 * Linux 7.3 port: the Samsung H4 character device is presented to the
 * upstream Bluetooth stack as a virtual HCI controller.  The kernel's
 * h4_recv_buf() helper is private to hci_uart and needs a struct hci_uart,
 * so the small H4 demultiplexer below is self-contained.
 *
 ****************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include <linux/unaligned.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include <scsc/scsc_logring.h>

#include "scsc_bt_priv.h"

static struct hci_dev			*hdev;
static struct device			*dev_ref;
static const struct file_operations	*bt_fs;
static atomic_t				*error_count_ref;
static wait_queue_head_t		*read_wait_ref;
static struct workqueue_struct		*wq;
static struct delayed_work		read_work;
static u8				receive_buffer[1024];
static bool				terminate_read;

/* H4 receive state */
static struct sk_buff			*rx_skb;
static u8				rx_type;
static u16				rx_hlen;
static u16				rx_dlen;
static bool				rx_header_done;

static void slsi_bt_rx_reset(void)
{
	if (rx_skb) {
		kfree_skb(rx_skb);
		rx_skb = NULL;
	}
	rx_header_done = false;
	rx_hlen = 0;
	rx_dlen = 0;
}

/* Feed H4 bytes into a per-packet skb and hand complete frames to BlueZ. */
static int slsi_bt_recv(struct hci_dev *hdev, const u8 *data, int count)
{
	while (count) {
		int n, need;

		if (!rx_skb) {
			u16 maxlen;

			rx_type = data[0];
			switch (rx_type) {
			case HCI_EVENT_PKT:
				rx_hlen = HCI_EVENT_HDR_SIZE;
				maxlen = HCI_MAX_EVENT_SIZE;
				break;
			case HCI_ACLDATA_PKT:
				rx_hlen = HCI_ACL_HDR_SIZE;
				maxlen = HCI_MAX_FRAME_SIZE;
				break;
			case HCI_SCODATA_PKT:
				rx_hlen = HCI_SCO_HDR_SIZE;
				maxlen = HCI_MAX_SCO_SIZE;
				break;
			default:
				SCSC_TAG_ERR(BT_H4, "unknown H4 packet type 0x%02x\n",
					     rx_type);
				return -EILSEQ;
			}

			rx_skb = bt_skb_alloc(maxlen, GFP_ATOMIC);
			if (!rx_skb)
				return -ENOMEM;

			hci_skb_pkt_type(rx_skb) = rx_type;
			rx_header_done = false;
			rx_dlen = 0;

			/* The type byte is not part of the skb payload. */
			data++;
			count--;
			continue;
		}

		if (!rx_header_done) {
			need = rx_hlen - rx_skb->len;
			n = min(need, count);
			skb_put_data(rx_skb, data, n);
			data += n;
			count -= n;

			if (rx_skb->len == rx_hlen) {
				if (rx_type == HCI_EVENT_PKT)
					rx_dlen = rx_skb->data[1];
				else if (rx_type == HCI_SCODATA_PKT)
					rx_dlen = rx_skb->data[2];
				else
					rx_dlen = get_unaligned_le16(&rx_skb->data[2]);
				rx_header_done = true;
			}
			continue;
		}

		need = (rx_hlen + rx_dlen) - rx_skb->len;
		n = min(need, count);
		skb_put_data(rx_skb, data, n);
		data += n;
		count -= n;

		if (rx_skb->len == rx_hlen + rx_dlen) {
			struct sk_buff *skb = rx_skb;

			rx_skb = NULL;
			rx_header_done = false;

			hdev->stat.byte_rx += skb->len;
			if (hci_skb_pkt_type(skb) == HCI_ACLDATA_PKT)
				hdev->stat.acl_rx++;
			else if (hci_skb_pkt_type(skb) == HCI_SCODATA_PKT)
				hdev->stat.sco_rx++;
			else
				hdev->stat.evt_rx++;

			if (hci_recv_frame(hdev, skb)) {
				hdev->stat.err_rx++;
				SCSC_TAG_ERR(BT_H4, "hci_recv_frame failed\n");
			}
		}
	}

	return 0;
}

static void slsi_bt_fs_read_func(struct work_struct *work)
{
	int ret;

	while ((ret = scsc_bt_shm_h4_read_kernel(receive_buffer,
						 sizeof(receive_buffer))) >= 0) {
		if (terminate_read)
			break;

		if (ret > 0) {
			if (slsi_bt_recv(hdev, receive_buffer, ret) < 0)
				break;
		}
	}

	SCSC_TAG_INFO(BT_COMMON, "BT BlueZ: Exiting %s\n", __func__);
}

static int slsi_bt_open(struct hci_dev *hdev)
{
	int err;

	SCSC_TAG_INFO(BT_COMMON, "enter\n");

	if (!bt_fs)
		return -ENODEV;

	err = bt_fs->open(NULL, NULL);
	if (err) {
		SCSC_TAG_ERR(BT_COMMON, "H4 open failed: %d\n", err);
		return err;
	}

	/* The BlueZ bridge passes kernel buffers through the shared-memory
	 * path; the character device cannot be open at the same time. */
	bt_service.kernel_io = true;

	slsi_bt_rx_reset();
	terminate_read = false;

	if (!wq)
		wq = create_singlethread_workqueue("slsi_bt_bluez_wq");
	if (!wq) {
		bt_service.kernel_io = false;
		bt_fs->release(NULL, NULL);
		return -ENOMEM;
	}

	/* hdev->open() runs before HCI_INIT is set, so wait briefly before
	 * reading.  The firmware may already emit events and hci_recv_frame()
	 * drops frames received before the core is ready to process them. */
	INIT_DELAYED_WORK(&read_work, slsi_bt_fs_read_func);
	queue_delayed_work(wq, &read_work, msecs_to_jiffies(20));

	SCSC_TAG_INFO(BT_COMMON, "done\n");

	return 0;
}

static int slsi_bt_close(struct hci_dev *hdev)
{
	int ret;

	SCSC_TAG_INFO(BT_COMMON, "terminating reader thread\n");

	terminate_read = true;

	if (error_count_ref)
		atomic_inc(error_count_ref);

	if (read_wait_ref)
		wake_up(read_wait_ref);

	cancel_delayed_work_sync(&read_work);
	slsi_bt_rx_reset();

	SCSC_TAG_INFO(BT_COMMON, "releasing service\n");

	if (bt_fs)
		ret = bt_fs->release(NULL, NULL);
	else
		ret = -ENODEV;

	bt_service.kernel_io = false;

	if (wq) {
		destroy_workqueue(wq);
		wq = NULL;
	}

	SCSC_TAG_INFO(BT_COMMON, "done\n");

	return ret;
}

static int slsi_bt_flush(struct hci_dev *hdev)
{
	return 0;
}

static int slsi_bt_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
	u16 len = skb->len;
	u8 type = hci_skb_pkt_type(skb);
	int ret;

	SCSC_TAG_DEBUG(BT_H4, "sending frame(data=%p, len=%u)\n",
		       skb->data, skb->len);

	if (!bt_fs) {
		kfree_skb(skb);
		return -ENODEV;
	}

	memcpy(skb_push(skb, 1), &type, 1);

	ret = scsc_bt_shm_h4_write_kernel(skb->data, skb->len);
	if (ret >= 0) {
		hdev->stat.byte_tx += len;

		switch (type) {
		case HCI_COMMAND_PKT:
			hdev->stat.cmd_tx++;
			break;
		case HCI_ACLDATA_PKT:
			hdev->stat.acl_tx++;
			break;
		case HCI_SCODATA_PKT:
			hdev->stat.sco_tx++;
			break;
		}

		kfree_skb(skb);
	}

	return ret;
}

static int slsi_bt_register(void)
{
	int err;

	if (hdev)
		return 0;

	hdev = hci_alloc_dev();
	if (!hdev)
		return -ENOMEM;

	hdev->bus = HCI_VIRTUAL;

	if (dev_ref)
		SET_HCIDEV_DEV(hdev, dev_ref);

	hdev->open  = slsi_bt_open;
	hdev->close = slsi_bt_close;
	hdev->flush = slsi_bt_flush;
	hdev->send  = slsi_bt_send_frame;

	err = hci_register_dev(hdev);
	if (err < 0) {
		SCSC_TAG_ERR(BT_COMMON,
			     "failed to register hci device (err: %d)\n", err);
		hci_free_dev(hdev);
		hdev = NULL;
		return err;
	}

	SCSC_TAG_INFO(BT_COMMON, "registered %s\n", hdev->name);
	return 0;
}

static void slsi_bt_unregister(void)
{
	if (!hdev)
		return;

	hci_unregister_dev(hdev);
	hci_free_dev(hdev);
	hdev = NULL;
}

void slsi_bt_notify_probe(struct device *dev,
			  const struct file_operations *fs,
			  atomic_t *error_count,
			  wait_queue_head_t *read_wait)
{
	bt_fs           = fs;
	error_count_ref = error_count;
	read_wait_ref   = read_wait;
	dev_ref         = dev;

	SCSC_TAG_INFO(BT_COMMON, "SLSI BT BlueZ probe\n");

	slsi_bt_register();
}

void slsi_bt_notify_remove(void)
{
	SCSC_TAG_INFO(BT_COMMON, "SLSI BT BlueZ remove\n");

	slsi_bt_unregister();

	error_count_ref = NULL;
	read_wait_ref   = NULL;
	dev_ref         = NULL;
}

MODULE_DESCRIPTION("SCSC BT Bluez");
MODULE_AUTHOR("SLSI");
MODULE_LICENSE("GPL and additional rights");
