/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Nastavení ovladače Samsung SCSC pro Motorola One Action (troika). Vychází
 * z gta4xl.h portu gta4xl; liší se umístěním firmwaru, názvem konfigurace
 * rádia (adresář v /vendor/etc/wifi/mx140/conf původního systému) a tím, že
 * Bluetooth se zatím nesestavuje a společná konfigurace common.hcf se
 * nečeká (firmware 6906 ji v obrazu vendor nemá).
 */
#ifndef TROIKA_SCSC_CONFIG_H
#define TROIKA_SCSC_CONFIG_H
#include <linux/version.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/sizes.h>
#include <linux/string.h>
#define CONFIG_SOC_EXYNOS9610 1
#define CONFIG_SCSC_CORE 1
#define CONFIG_SCSC_CORE_CM 1
#define CONFIG_SCSC_PLATFORM 1
#define CONFIG_SCSC_WLAN 1
#define CONFIG_SCSC_WLAN_STA_ONLY 1
#define CONFIG_SCSC_WLAN_SAE_CONFIG 1
#define CONFIG_SCSC_WLAN_MAX_INTERFACES 3
#define CONFIG_SCSC_CORE_FW_LOCATION "motorola/troika"
#define CONFIG_SCSC_CORE_TOOL_LOCATION "/usr/bin"
#define CONFIG_SCSC_WLAN_MAC_ADDRESS_FILENAME "motorola/troika/mac.txt"
#define CONFIG_SCSC_WLBT_CONFIG_PLATFORM "leman_s620_troika_dualfem"
#define CONFIG_SCSC_BUILD_TYPE "upstream-port"
#endif
