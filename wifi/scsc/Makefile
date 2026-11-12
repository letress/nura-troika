# SPDX-License-Identifier: GPL-2.0-only
# Jako Makefile portu gta4xl, bez Bluetooth a s nastavením pro troiku.
ccflags-y += -I$(src)/include -I$(src)/core -include $(src)/troika.h
obj-m += scsc_platform_mif.o scsc_mx.o scsc_wlan.o
scsc_platform_mif-y := core/platform_mif_module.o core/platform_mif_9610.o
scsc_mx-y := core/mxlog_transport.o core/fw_panic_record.o core/panicmon.o core/suspendmon.o core/mxlog.o core/mxproc.o core/scsc_service.o core/scsc_mx_module.o core/scsc_mx_impl.o core/mxman.o core/miframman.o core/mifmboxman.o core/mifproc.o core/mifintrbit.o core/fwhdr.o core/fwimage.o core/cpacket_buffer.o core/mifstream.o core/mxmgmt_transport.o core/gdb_transport.o core/scsc_lerna.o core/mxsyserr.o core/mxfwconfig.o core/mx140_file.o
scsc_wlan-y := wlan/hip4.o wlan/mbulk.o wlan/dev.o wlan/cfg80211_ops.o wlan/netif.o wlan/rx.o wlan/tx.o wlan/ba.o wlan/sap_mlme.o wlan/sap_ma.o wlan/sap_dbg.o wlan/sap_test.o wlan/mib.o wlan/mib_text_convert.o wlan/debug.o wlan/debug_frame.o wlan/procfs.o wlan/mgt.o wlan/mlme.o wlan/udi.o wlan/log_clients.o wlan/src_sink.o wlan/fw_test.o wlan/cac.o wlan/scsc_wifi_fcq.o wlan/traffic_monitor.o wlan/reg_info.o wlan/scsc_wlan_mmap.o wlan/hip.o wlan/cm_if.o
