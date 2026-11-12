# Ovladač WiFi (Samsung SCSC „Maxwell“) pro troiku

`scsc/` je upravená kopie ovladače, kterou sestavuje balíček kernelu
v pmaports jako moduly `scsc_platform_mif`, `scsc_mx` a `scsc_wlan`.

Původ:

- Původní soubory: zdroje kernelu tabletu Samsung SM-P615 (gta4xl) z
  `LineageOS/android_kernel_samsung_gta4xl`, commit `8d1f7a68b0d0`, adresáře
  `drivers/misc/samsung/scsc`, `drivers/net/wireless/scsc`,
  `drivers/misc/samsung/scsc_bt`, `include/scsc`. Vydání ovladače 11.19.0.46.0.
  Jsou součástí zdrojů kernelu, které Samsung zveřejňuje jako GPL-2.0; moduly
  se hlásí `MODULE_LICENSE("GPL")` nebo `"GPL and additional rights"`. Většina
  souborů ale má v hlavičce jen autorská práva Samsungu bez zmínky o licenci.
- Přenos na Linux 7.3: patche `0001-linux-7.3-port.patch`
  a `0002-bt-linux-7.3-port.patch` z `github.com/arhammahajan/gta4xl-linux`
  (commit `4d4f1fd`). Repozitář nemá licenční soubor; nové soubory portu mají
  značku `GPL-2.0-only`.
- Naše úpravy: umístění firmwaru přes `CONFIG_SCSC_CORE_FW_LOCATION` místo
  cesty tabletu, `troika.h` (nastavení pro troiku), `Makefile` bez Bluetooth.

Obnovení ze zdrojů: `tools/update-wifi-pkg.sh` (potřebuje `refs/gta4xl-kernel`
a `refs/gta4xl-linux`). Firmware rádia tu není; bere se z balíčku
`firmware-motorola-troika`.
