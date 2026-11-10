# FINDINGS – Motorola One Action (troika, Exynos 9609) na mainline

Stav k 2026-11-10. Fáze 1 (průzkum). Nic nebylo sestaveno ani flashováno.

> **Aktualizace 2026-11-10 odpoledne:** uživatel vyčetl data z telefonu
> (`tools/dump-from-phone.sh`). Ověřené hodnoty jsou v nové **kap. 8** a
> promítnuté do kap. 4–6. Body 1 a 4 níže popisují stav před vyčtením.

## 0. Co je potřeba vědět jako první

1. **Původní záloha `~/troika-backup` neexistovala** (teď už ano, viz kap. 8;
   obsahuje LineageOS, ne stock). `pmbootstrap bootimg_analyze` proběhl
   na `boot_a.img` z této zálohy (kap. 4).
   `~/Firmware/boot.img` je z MediaTek MT6735M, s troikou nesouvisí.
2. **Clock driver v3 je podle autora na vyhození.** Chimac 2026-07-23 napsal, že
   ho přepíše od nuly; v3 má 11 nahlášených chyb. v4 na lore není.
3. **DTS série existuje jen ve verzi v1 (2025-09-14)** a nikdy nebyla přeposlána.
4. **Rozložení RAM troiky nemá žádný zdroj v kódu** – downstream DTS neobsahuje
   uzel `memory`, doplňuje ho bootloader. Vyčteno z běžícího telefonu (kap. 8.1).
5. **Bootloader bere DTB ze slotu „second“ v boot.img a aplikuje na něj overlay
   z oddílu `dtbo`.** Stock overlay na mainline DTB nesedne. Protože `dtbo`
   nesmíme flashovat, bude nejspíš nutný mezičlánek (uniLoader). Viz kap. 3.5.
6. **USB síť z existujících patchů nejde** – mainline nemá USB PHY ani dwc3 glue
   pro 9610 a nikdo je neposlal.

Zdrojové stromy jsou v `refs/` (commity v kap. 7). Citace `soubor:řádek`
se vztahují k těmto commitům.

---

## 1. Co je v mainline a co jen v patchích

### 1.1 Přijato (ověřeno v linux-next `next-20261109`, 630059a5a632)

| Část | Kde | Přijato |
|---|---|---|
| chipid 9610 | `drivers/soc/samsung/exynos-chipid.c:71` (`0xE9610000`), binding `hwinfo/samsung,exynos-chipid.yaml:31` | 2025-10-13, krzk/linux 944d40232eb4 + fe31e894c284 (v6.19) |
| pinctrl 9610 | `drivers/pinctrl/samsung/pinctrl-exynos-arm64.c:2024-2139`, `pinctrl-samsung.c:1506`, binding `pinctrl/samsung,pinctrl.yaml:60` | 2026-01-05, pinctrl/samsung 2432700e9c92, 891eb0543f20, 9ce17ff1881f (série v3, v7.0) |
| wakeup-eint 9610 | `pinctrl/samsung,pinctrl-wakeup-interrupt.yaml:53` (fallback `exynos850` + `exynos7`) | totéž |

### 1.2 Jen v patchích (v linux-next chybí – ověřeno grepem)

| Část | Poslední verze | Message-ID | Stav |
|---|---|---|---|
| Clock controller (bindings CMU, sysreg, `pll_1061x`, `clk-exynos9610.c`, MAINTAINERS) | **v3**, 2026-07-22, base 06a508eb7374 | `20260722-exynos9610-clocks-v3-0-c672a2a30889@chimac.ro` | nepřijato, autor ohlásil přepis |
| `exynos9610.dtsi`, `exynos9610-pinctrl.dtsi`, `exynos9610-gta4xl.dts`, board binding | **v1**, 2025-09-14, base 48c4c0b684f3 | `20250914-exynos9610-devicetree-v1-0-2000fc3bbe0b@chimac.ro` | nepřijato, bez v2 |
| CMU/sysreg uzly v DT, clocks pro pinctrl/GIC/simplefb | jen v **clk v1** (patche 4, 6, 7, 8 z 8), 2025-09-14; od clk v2 vypuštěno | `20250915-exynos9610-clocks-v1-0-3f615022b178@chimac.ro` | osiřelé, neodpovídají v3 |
| „Require clocks for Exynos9610 pinctrl nodes“ | v1, 2026-09-22 | `20260922-exynos9610-pinctrl-patch-v1-1-f6d816696591@chimac.ro` | nepřijato, poslední dotaz Krzysztofa (2026-09-28) bez odpovědi |
| USB PHY / dwc3 glue / UART / MCT / PMU pro 9610 | – | – | nikdo neposlal (lore: `exynos9610 usbdrd` = 0 výsledků) |

Clock v3 pokrývá TOP, APM, CAM, CMGP, CORE, CPUCL0, DISPAUD, FSYS, G2D, G3D, PERI,
USB. Chybí SHUB, MIF, ISP, VIPX, MFC a **CPUCL1** (odstraněn ve v2: „PLL
initialization fails with ACG enabled“).

Vyhledávání `exynos9609` a `troika exynos` na lore: 0 výsledků. Pro troiku
ani pro 9609 upstream nic neexistuje.

---

## 2. Nevyřešené připomínky z review

### 2.1 Clock série v3

**sashiko-bot k 4/5 (driver) – 11 bodů.** Jsou to tvrzení automatického
reviewera; sám jsem je proti kódu neověřoval (plný text driveru jsem nestahoval).
Autor je nerozporoval a odpověděl, že driver přepíše.

| # | Závažnost | Problém |
|---|---|---|
| 1 | High | CMGP USI PCLK gate používají offsety IPCLK registrů (USI00–04) |
| 2 | High | CMGP USI clocky mají za rodiče mux místo děliče (`dout_clk_cmgp_usi0x`) |
| 3 | High | `CLK_GOUT_CORE_CCI_OCC_CLK` používá registr běžného CCI gate |
| 4 | High | `CLK_DOUT_CLK_CPUCL0_CPU` má šířku 0 |
| 5 | High | chybí gate `gout_cmu_shub_bus` v APM → osiřelý podstrom |
| 6 | High | v `g2d_gate_clks` chybí `gout_g2d_g2d_clk` a `gout_g2d_mscl_clk` |
| 7 | High | v `*_clk_regs` chybí registry (CAM LHS_ACEL_D, PGEN_LITE_CAM1, FSYS MMC SDCLKIN) |
| 8 | High | `core_cmu_info` neregistruje `core_div_clks` → 25 osiřelých gate |
| 9 | High | překlep `mout_cmu_g3d_SWITCH` (velká písmena) → osiřelý G3D strom |
| 10 | High | fixed clock `oscclk_rco_cmgp` vs. konzument `oscclk_rco` |
| 11 | Low | `CLK_DOUT_CMU_MFD_*` místo `MFC` |

**Lidé:**
- Peter Griffin (4/5): zmínit ACG v commit message; dodat výpis
  `clk_summary` a `clk_orphan_summary`. Nedodáno.
- Krzysztof Kozlowski (1/5): zalomení commit message; compatibles abecedně
  a `if:then:` ve stejném pořadí; smazat komentář a nepoužitý label v příkladu;
  jednotné indexování (CMGP začíná od 0, ostatní od 1); z předmětu vypustit
  druhé „bindings“.
- Alim Akhtar (3/5): zmínit, že PLL_1061x je frakční PLL pro MMC a AUD; komentář,
  že `con_reg + 0xc` je PLL_CON3.
- 2/5 (sysreg binding): Reviewed-by Peter Griffin – bez připomínek.

### 2.2 DTS série v1

- Rob Herring (bot, 1/3): yamllint `samsung-boards.yaml:250` – špatné odsazení.
- Krzysztof (2/3, dtsi): PMU přerušení zapsat přes makra; `clock-frequency`
  u `oscclk` patří do boardu; „What is abox?“; `ramoops` patří do boardu.
- Krzysztof (3/3, gta4xl): hex malými písmeny.
- Krzysztof (clk v1 4/8): DT změny nesmí být zvlášť bez vysvětlené závislosti.

Na žádnou z nich nepřišla nová verze.

### 2.3 „Require clocks“ pro pinctrl

Krzysztof: „ABI break without reason“ a jak to má fungovat pro PINCTRL_TOP,
který clock nemá. Bez odpovědi. Sám Chimac ve vlákně říká, že pinctrl clocky
řídit nepotřebuje, protože CMU mají HWACG.

---

## 3. Chyby a konflikty, které jsem našel

### 3.1 Konflikty mezi patchi

| # | Konflikt | Doklad |
|---|---|---|
| K1 | DT uzly z clk v1 neodpovídají driveru/bindingu v3: mají `cmu_cpucl1` (ve v3 není), nemají `cmu_usb` (ve v3 je), includují `dt-bindings/clock/samsung,exynos9610.h`, ale v3 přidává `samsung,exynos9610-cmu.h` | clk v1 4/8 vs. diffstat v3 0/5 |
| K2 | `cmu_apm` ve v1 má 3 vstupy (`oscclk`, `dll_dco`, `dout_cmu_apm_bus`), binding v3 uvádí 2 (`oscclk`, `dout_cmu_apm_bus`) | clk v1 4/8 vs. citace bindingu v review Krzysztofa k v3 1/5 |
| K3 | clk v1 7/8 přidává `clocks` do pinctrl uzlů, ale binding v mainline je pro 9610 **zakazuje** (`else: clocks: false`) → `dtbs_check` selže | `samsung,pinctrl.yaml:135-151` v linux-next |
| K4 | Kdyby prošel patch „Require clocks“, selže naopak `pinctrl_top` (clock nemá) | vlákno z 2026-09-22 |
| K5 | clk v1 4/8: `clock-controller@0x10800000`, `@0x10900000` – unit-address s `0x` | text patche |
| K6 | clk v1 8/8 dává simplefb clock `CLK_GOUT_DISPAUD_CLK_DISPAUD_DISP`. S aktivním clk driverem a bez tohoto clocku kernel při `late_initcall` vypne „nepoužité“ clocky a displej zhasne. Downstream proto bootuje s `clk_ignore_unused` | downstream `exynos9610.dtsi:121`. Existenci ID ve v3 hlavičce jsem neověřil |

Co naopak **sedí**: aliasy `pinctrl0..5` a pořadí instancí v dtsi v1 odpovídají
přijatému driveru (ALIVE, CMGP, DISPAUD, FSYS, TOP, SHUB); názvy všech 50 bank
se shodují (porovnáno skriptem); compatible řetězec wakeup-eint odpovídá
finálnímu bindingu.

### 3.2 Chyby v `exynos9610.dtsi` v1 (`refs/lore/exynos9610.dtsi.v1`)

| # | Řádek | Problém | Dopad na troiku |
|---|---|---|---|
| D1 | 246-250 | `abox_rmem`: `compatible = "reserved-memory"` není platný compatible; velikost `0x2800000`, downstream má `0x400000` (`exynos9610-rmem.dtsi:63-66`) | rezervace 40 MB navíc, chyba v `dtbs_check` |
| D2 | 252-259 | `ramoops@f9d10000` v SoC dtsi – adresa je z gta4xl | **na troice by se zdědila cizí adresa**; downstream tam má debug-snapshot od `0xF9000000` (`debug-snapshot-table.h`) |
| D3 | 199-204 | GIC `interrupt-controller@12300000`, ale první `reg` je `0x12301000` | varování dtc `unit_address_vs_reg` (downstream má totéž, `exynos9610.dtsi:560-565`) |
| D4 | 135-150 | PMU přerušení jako `<0 82 4>` | review |
| D5 | 157-162 | `clock-frequency` oscilátoru v SoC dtsi | review |
| D6 | – | žádný UART, žádný `stdout-path` | ladění jen přes framebuffer |

### 3.3 Chyby v `exynos9610-pinctrl.dtsi` v1 (`refs/lore/exynos9610-pinctrl.dtsi.v1`)

| # | Problém | Doklad |
|---|---|---|
| P1 | `pinctrl_cmgp` nemá poduzel `wakeup-interrupt-controller`, ale driver pro CMGP volá `exynos_eint_wkup_init`, který bez něj vrátí `-ENODEV` | `pinctrl-exynos-arm64.c:2107-2113`, `pinctrl-exynos.c:791`; downstream ho má (`exynos9610.dtsi:714-716`) |
| P2 | 10 bank CMGP (gpm4, 5, 10, 11, 15, 20, 21, 23, 24, 25) nemá `interrupts`, v driveru jsou `EINTW`; downstream je má (např. gpm4 = SPI 158, `exynos9610-pinctrl.dtsi:141-149`) | skript nad dtsi a driverem |
| P3 | `etc0` a `gpq0` mají `interrupt-controller`, v driveru jsou `EINTN` | `pinctrl-exynos-arm64.c:2026,2030` |

P1–P3 se netýkají banky `gpa1` (tlačítka); ta je v pořádku (SPI 8–15 stejně jako
downstream `exynos9610-pinctrl.dtsi:29-38`).

### 3.4 Rozdíly 9610 vs. 9609

| Oblast | Zjištění | Zdroj |
|---|---|---|
| Chip ID | `0xE9610000` – shodné s tabulkou v mainline. **Ověřeno na zařízení:** downstream driver hlásí `soc_id: EXYNOS9610`, revize 2; bootloader přitom SoC označuje `mmi,sam_hw = "Exynos9609"` | `exynos9610-mot-troika.dtsi:7`, mainline `exynos-chipid.c:71`; `device-info/soc0.txt`, downstream `exynos-chipid.c:88-93`, `device-info/live.dts` (chosen) |
| Kód | downstream zachází s troikou jako s 9610: `compatible = "samsung,exynos9610"`, `TARGET_SOC := exynos9610`; v ovladačích není žádná větev pro 9609 | `exynos9609-troika_evb.dts:19`, `BoardConfigCommon.mk:112-114` |
| Frekvence CPU/GPU | 9609 má nižší maxima (wiki: 2,2 / 1,6 GHz, GPU 850 MHz) | wiki Nura „Samsung Exynos 9610“. Pro minimální boot bez cpufreq nepodstatné; při pozdějším přidání OPP tabulek nutno ošetřit |
| **Bootloader** | gta4xl má Samsung S-Boot, troika jiný (DTB v „second“ + dtbo). Odtud jiná adresa framebufferu (`0xca000000` vs. `0xec000000`), jiné rozložení RAM a rezervací | kap. 5 |
| Časovač | downstream používá arch timer jen jako clocksource a MCT pro události (`use-clocksource-only`, `use-clockevent-only`); dtsi v1 používá jen arch timer | `exynos9610.dtsi:572-584, 594-605`. Platí pro 9610 i 9609; riziko nízké, ale neověřené |

### 3.5 Boot flow – nejdůležitější praktický problém

- Boot image má hlavičku v1 a **základní DTB je ve slotu „second“**
  (`2ndbootloader.mk:6-9`, `TARGET_BOOTLOADER_IS_2ND := true`).
- Bootloader na něj aplikuje overlay z oddílu `dtbo` (`dtboimg.cfg`: `id=0x1`,
  `rev=0x0`; `exynos9609-troika_evb.dts:13,20-21` je `/plugin/` s `board_id 1`).
- Stock overlay cílí na labely jako `&exynos_adc`, `&sar_sx933x`,
  `&audio_micbias1` (`exynos9610-mot-troika.dtsi:31,81,168`). V mainline DTB
  neexistují. **Jak se bootloader zachová, když overlay selže, nevím.**
- Wiki to obchází příkazem `pmbootstrap flasher flash_dtbo` – to je podle
  pravidel zakázané.

Návrh, který flash nepotřebuje: jako „kernel“ v boot.img dát **uniLoader**
s vloženým mainline kernelem a DTB, do „second“ dát **původní stock DTB**
(aby overlay prošel) a výsledek bootloaderu ignorovat. uniLoader má podporu
9610 a gta4xl (`soc/exynos/exynos9610.c`, `board/samsung/board-gta4xl.c`),
troiku ne – board soubor by bylo nutné dopsat. Důsledek: bootloader pak do
našeho DTB nedoplní paměť, takže rozložení RAM musí být v DTS natvrdo.

### 3.6 Existující pmaports balíček

- V `pmaports` (main, 286d3b4ecce0) **není** `device-motorola-troika` ani nic
  pro 9610 mimo archivované `samsung-a50`/`a51` (downstream).
- Existuje uzavřený draft **MR !6458** „Add motorola-troika“ (Sweeistaken,
  uzavřen 2025-12-26): downstream 4.14.113, wiki ho vede jako „not booting“.
  Jeho `deviceinfo` má `header_version="2"` (LineageOS uvádí 1),
  `deviceinfo_dtb="exynos/exynos9609-troika_evb"` (to je overlay, ne základní
  DTB). Jako vzor ho brát nelze. (Původní výtka ke klíči
  `deviceinfo_dtb_second` byla ukvapená – viz kap. 4, pmbootstrap sám
  používá oba názvy.)

---

## 4. `pmbootstrap bootimg_analyze`

**`pmbootstrap bootimg_analyze ~/troika-backup/boot_a.img`** (pmbootstrap
3.12.1, spustil uživatel 2026-11-10 15:18–15:20), doslovný výstup:

```
deviceinfo_kernel_cmdline="loop.max_part=7 androidboot.boot_devices=13520000.ufs"
deviceinfo_generate_bootimg="true"
deviceinfo_flash_pagesize="2048"
deviceinfo_bootimg_qcdt="false"
deviceinfo_dtb_second="true"
deviceinfo_header_version="1"
deviceinfo_flash_offset_base="0x10000000"
deviceinfo_flash_offset_kernel="0x00008000"
deviceinfo_flash_offset_ramdisk="0x00000000"
deviceinfo_flash_offset_second="0x00000000"
deviceinfo_flash_offset_tags="0x00000000"
```

Pozor na dvě věci:
- Analyzovaný image je **LineageOS 22**, ne stock (stock záloha neexistuje).
  Formát ale určuje bootloader, takže hodnoty platí.
- **Název klíče pro DTB ve „second“ je v pmbootstrapu 3.12.1 nejednotný:**
  `bootimg_analyze` i generátor (`pmb/aportgen/device.py:143`) vypisují
  `deviceinfo_dtb_second`, ale parser deviceinfo zná `bootimg_dtb_second`
  (`pmb/parse/deviceinfo.py:212`); v pmaports se vyskytují oba (5× a 8×).
  Který z nich čte `boot-deploy`, je potřeba ověřit před psaním `deviceinfo`
  ve fázi 2. **TODO.**

Stejnou hlavičku jsem nezávisle rozebral nástrojem `unpack_bootimg` (výstup
v `device-info/bootimg_a/`, `bootimg_b/`). Image je **LineageOS 22**
(os version 15.0.0, kernel `4.14.113-g809b8162409a`), ne stock:

| Parametr | `boot_a.img` (aktivní slot) | Shoda s LineageOS BoardConfig |
|---|---|---|
| header version | **1** | ano (MR !6458 s v2 je chybně) |
| page size | 2048 | ano |
| kernel load address | `0x10008000` (base `0x10000000` + `0x8000`) | ano |
| ramdisk / second / tags address | `0x10000000` (offsety 0) | ano |
| kernel | 33 198 092 B, nekomprimovaný arm64 `Image` | ano |
| ramdisk | 15 029 804 B, gzip | – |
| second | 133 209 B, **Device Tree Blob** (má `__symbols__`, nemá uzly `memory`) | ano |
| cmdline | `loop.max_part=7 androidboot.boot_devices=13520000.ufs` | ano |
| recovery dtbo | 0 | – |

`boot_b.img` má stejnou hlavičku i stejný `second`, liší se jen ramdisk
(patch level 2025-02 vs. 2025-04).

`dtbo_a.img` (rozebráno `mkdtboimg dump`, výstup v `device-info/dtbo_a/`):
2 položky, obě `id=1 rev=0`; `dtbo.0` = kane, **`dtbo.1` = troika**
(bootloader předává `androidboot.dtbo_idx=1`). Overlay troiky má 79 fragmentů
a odkazuje na 118 labelů základního DTB – na mainline DTB bez těchto labelů
se aplikovat nedá (potvrzuje kap. 3.5).

Původní náhradní údaje z LineageOS (komentář v souboru říká
„Extracted with libbootimg“):

| Parametr | Hodnota | Zdroj |
|---|---|---|
| header version | 1 | `android_device_motorola_exynos9610-common/BoardConfigCommon.mk:45` |
| base | `0x10000000` | :46 |
| kernel offset | `0x00008000` | :48 |
| page size | 2048 | :49 |
| ramdisk / second / tags offset | `0x00000000` | :50-52 |
| cmdline | `loop.max_part=7 androidboot.boot_devices=13520000.ufs` | :47 |
| DTB | ve „second“ (`exynos9610.dtb`) | `build/tasks/2ndbootloader.mk:6-9` |
| kernel | nekomprimovaný `Image` | :84 |
| dtbo | samostatný oddíl, 1 MiB | :64, :96 |
| boot oddíl | 64 MiB, A/B, AVB | :95, :12-22, :146 |

Rozpor header v1 (LineageOS) vs. v2 (MR !6458) je vyřešen: skutečný image
má **v1**.

---

## 5. Hodnoty pro troika DTS

Stav: **OK** = doložené zdrojem; **OVĚŘIT** = zdroj existuje, ale může se
lišit od skutečnosti; **TODO** = zdroj nemám.

| Položka | Hodnota | Zdroj | Stav |
|---|---|---|---|
| Rozložení RAM (`memory@…`) | 4 oblasti, viz kap. 8.1 | `device-info/live.dts:13-31` (DTB od bootloaderu), souhlasí s `device-info/iomem.txt:98-103` | **OK** |
| Velikost RAM | 4027 MiB celkem | součet oblastí z kap. 8.1 | OK |
| Rezervace pro splash – adresa | `0xec000000` | `device-info/live.dts:2619-2621` (bootloader uzel nepřepisuje); shodně `exynos9610-rmem.dtsi:94-97`, používá `dsim_0` (`exynos9610.dtsi:2600`, `dsim_drv.c:2198-2238`) | **OK** jako oblast. Zobrazovaný buffer ale **nezačíná na její bázi**, viz další řádek |
| Adresa zobrazovaného bufferu | `0xed000000` | přečteno z DPU na telefonu (testy D a G, registr `IDMA_IN_BASE_ADDR_Y` kanálu G0) | **OK** |
| Rezervace pro splash – velikost | `0x4000000` (64 MiB) | tamtéž | OK |
| Panel v tomto kusu | Himax HX83112A, 1080 × 2520 | `ddi_id = <0x1047291>` (`device-info/live.dts:7391`) → `dsim_drv.c:1626-1629` → `exynos9610-display-lcd.dtsi:330-333`; `dmesg.txt` („hix83112a panel“) | OK |
| Rozlišení | 1080 × 2520 | `exynos9610-display-lcd.dtsi:333` (hix83112a), `:369` (nov36672a); `hix83112a_lcd_ctrl.c:34`, `nov36672a_lcd_ctrl.c:34`; `common.mk:81-82` | OK |
| Stride | 4320 (1080 × 4) | odvozeno, platí jen při 32 bpp | TODO (závisí na formátu) |
| Formát pixelů | – | zdroj nemám. gta4xl má `a8r8g8b8`, ale jiný bootloader; `TARGET_RECOVERY_PIXEL_FORMAT := ABGR_8888` (`BoardConfigCommon.mk:126`) se týká recovery, ne bootloaderu | **TODO** |
| Vol+ | `gpa1-5`, kód 115 | `exynos9609-robusta2_common.dtsi:476-480, 1400-1405` | OK |
| Vol− | `gpa1-6`, kód 114 | `:471-475, 1393-1398` | OK |
| Power | `gpa1-7`, kód 116, wakeup | `:481-486, 1407-1412` | OK |
| Řadič tlačítek | `pinctrl@11850000` (ALIVE) | piny jsou v `&pinctrl_0` (`:1378`) = `exynos9610.dtsi:674-676` | OK |
| Pin config tlačítek | funkce `0xf` (EINT), bez pull, drv 0 | `:1393-1412` | OK |
| Polarita tlačítek | **active-low** | v klidu jsou všechny tři vstupy `hi`: `device-info/gpio.txt:11-13` (gpio-19/20/21 = `gpa1` 5/6/7, báze 14). Předpoklad: při vyčítání nebylo žádné tlačítko stisknuté | OK |
| Přerušení `gpa1` | SPI 8–15 | `exynos9610-pinctrl.dtsi:29-38` | OK |
| Timer `clock-frequency` | 26000000 | `exynos9610.dtsi:582` | OK |
| PSCI | `smc`, suspend `0xC4000001`, off `0x84000002`, on `0xC4000003` | `exynos9610.dtsi:343-349` | OK |
| CPU `reg` | A53: `0x0–0x3`, A73: `0x100–0x103` | `exynos9610.dtsi:187-247` | OK |
| GIC | `0x12301000`, `0x12302000`, `0x12304000`, `0x12306000` | `exynos9610.dtsi:560-570` | OK |
| chipid | `0x10000000`, velikost `0x100` | `exynos9610.dtsi:126-129` | OK |
| Adresa initrd | `0x84000000` | `device-info/live.dts:3963` (bootloader tam ramdisk opravdu nahrává); `exynos9610.dtsi:122-123`; uniLoader `configs/gta4xl_defconfig:7` | OK |
| DECON trigger (uniLoader) | `0x148B0000 + 0x70 = 0x1281` | uniLoader `soc/exynos/exynos9610.c:9`; báze sedí s `exynos9610.dtsi:2604-2607` | OVĚŘIT, zda je na troice potřeba (panel je video mode: `display-lcd.dtsi:332,368`, `psr_mode = <0>` na `:2623`) |
| USB dwc3 | `0x13200000`, SPI 186, `dr_mode = "otg"`, high-speed | `exynos9610.dtsi:2935-2948`, `robusta2_common.dtsi:1733-1740` | OK jako údaj, **nepoužitelné** (níže) |
| USB PHY | `0x131D0000` (usb2), `0x131F0000` + `0x131E0000` (usb3), PMU izolace offset `0x704`, maska `0x3` | `exynos9610.dtsi:2972-2981, 3026-3036` | OK jako údaj |
| Ladicí UART | `uart@13820000`; přepínaný přes MUIC na `gpq0-3`/`gpq0-4` | `exynos9610.dtsi:1463`, `robusta2_common.dtsi:1185-1187` | informativní |

Pozn. k rozlišení: výchozí `lcd_info` v board dtsi ukazuje na `nt36672a`
s 1080 × 2246 (`robusta2_common.dtsi:1762`, `display-lcd.dtsi:258-261`).
Skutečný panel vybírá driver podle `ddi_id` od bootloaderu
(`dsim_drv.c:1612-1615`); oba panely troiky mají 2520.

**USB:** mainline `phy-exynos5-usbdrd.c:2911-2944` ani `dwc3-exynos.c:198-219`
compatible pro 9610 nemají. Bylo by potřeba dopsat podporu PHY, PMU syscon
a clocky (CMU_USB je jen v nefunkčním v3). PHY navíc potřebuje napájení z PMIC,
které podle pravidel nepovolujeme. USB síť tedy v první iteraci nebude.

---

## 6. Co nevím a co potřebuji od tebe

### 6.1 Blokující

1. ~~Záloha firmwaru~~ – vyřešeno 2026-11-10: oddíly `boot`, `dtbo`, `vbmeta`
   obou slotů jsou v `~/troika-backup/` (LineageOS 22, ne stock).
   Stock firmware pro úplný návrat by šel získat jen přes Motorola RSA.
2. ~~pmbootstrap, dtc~~ – nainstalováno. Pro fázi 2 zbývá `b4`,
   aarch64 cross-compiler, `dtschema`, `yamllint`. Volné místo je 28 GB
   (disk z 90 % plný) – na kernel + chrooty pmbootstrapu je to na hraně.
3. ~~Rozložení paměti~~ – vyřešeno, kap. 8.1.
4. ~~`pmbootstrap bootimg_analyze`~~ – proběhl, výstup v kap. 4. Zbývá ověřit
   název klíče `dtb_second` vs. `bootimg_dtb_second` v `boot-deploy`.
5. **`fastboot boot`:** bootloader je odemčený (`ro.boot.flash.locked=0`,
   `verifiedbootstate=orange`, `device-info/props.txt`). Zda MBM bootloader
   troiky příkaz `fastboot boot` podporuje a jak při něm naloží se „second“
   a s overlayem, **nevím**. Výstup `fastboot getvar all` (jen čte) by pomohl.

### 6.2 Ověřit při prvním testu

6. Formát pixelů framebufferu (poznáme podle barev) a zda obraz začíná
   přesně na `0xec000000`.
7. ~~Polarita tlačítek~~ – active-low, kap. 8.3.
8. ~~Chip ID~~ – `EXYNOS9610`, kap. 8.3.
9. Zda bootloader nechá displej běžet po předání řízení a zda je potřeba
   zápis do DECON triggeru.

### 6.3 Rozhodnutí pro fázi 2

9. **Clock driver:** doporučuji první boot **bez CMU uzlů v DT** (clocky
   nechat tak, jak je nastavil bootloader). RAM, simplefb ani gpio-keys je
   nepotřebují, gta4xl v1 série tak bootovala taky. v3 s ACG zapisuje do CMU
   registrů a má 11 známých chyb; jeho opravy bych dal jako samostatnou
   volitelnou větev. Souhlasíš, nebo chceš v3 včetně oprav hned?
10. **uniLoader:** souhlasíš s přístupem z kap. 3.5 (včetně dopsání board
    souboru pro troiku)? Alternativa je zkusit holý mainline `Image` + DTB
    ve „second“ a zjistit, co bootloader udělá s nesedícím overlayem.
11. **Compatible:** `"motorola,troika", "samsung,exynos9610"`, nebo zavést
    i `"samsung,exynos9609"`?
12. **USB:** vypustit z fáze 2, nebo ho řešit jako samostatný úkol po prvním
    bootu?

### 6.4 Omezení průzkumu

- lore.kernel.org a wiki.nura.eco blokují `curl`/WebFetch (Anubis); četl jsem
  je přes vestavěný prohlížeč. Pro `git am` ve fázi 2 budu potřebovat `b4`
  nebo patche vytáhnout stejnou cestou.
- Plný text `clk-exynos9610.c` v3 a hlavičky `samsung,exynos9610-cmu.h` jsem
  nestahoval; body sashiko-bota a K6 jsou proto neověřené.
- Zdrojový kód bootloaderu troiky jsem nenašel.
- Chimacův veřejný git strom s novější verzí dtsi jsem nenašel.

---

## 7. Zdrojové stromy v `refs/`

| Adresář | Původ | Commit |
|---|---|---|
| `linux-next/` (sparse) | git.kernel.org next/linux-next | 630059a5a632 (`next-20261109`) |
| `downstream-kernel/` | LineageOS/android_kernel_motorola_exynos9610, `lineage-24.0` (4.14.357) | 0c019791d5ef |
| `lineage/android_device_motorola_exynos9610-common/` | LineageOS, `lineage-24.0` | de82b0312223 |
| `lineage/android_device_motorola_troika/` | LineageOS, `lineage-24.0` | 0f1149874d7d |
| `pmaports/` | gitlab.nura.eco/nura/pmaports, `main` | 286d3b4ecce0 |
| `uniLoader/` | github.com/ivoszbg/uniLoader | 30442c60e34d |
| `lore/` | výtahy vláken z lore.kernel.org (clk v3, dts v1, pinctrl) a z nich vyextrahované `exynos9610.dtsi.v1`, `exynos9610-pinctrl.dtsi.v1` | – |
| `../device-info/` | data vyčtená z telefonu 2026-11-10: `live.dtb`/`live.dts`, `iomem.txt`, `gpio.txt`, `soc0.txt`, `props.txt`, rozbalené `bootimg_a/`, `bootimg_b/`, `dtbo_a/` | – |

Cesty v tabulkách bez prefixu se vztahují k `downstream-kernel/arch/arm64/boot/dts/exynos/`
(DTS), `downstream-kernel/drivers/video/fbdev/exynos/dpu20/` (`dsim_drv.c`,
`panels/*`), resp. k `linux-next/` (mainline ovladače a bindingy).

---

## 8. Hodnoty ověřené na zařízení (2026-11-10)

Zdroj: `device-info/` (vyčteno skriptem `tools/dump-from-phone.sh` z běžícího
LineageOS 22, kernel 4.14.113, bootloader `MBM-2.2-troika_retail-a38dd6e2345-220629`,
slot `_a`, SKU XT2013-2, hwrev `0xC000` PVT). `live.dts` je dekompilovaný
`/sys/firmware/fdt`, tedy DTB tak, jak ho kernelu předal bootloader.

### 8.1 Rozložení RAM (`device-info/live.dts:13-31`)

| Báze | Velikost | Konec | MiB |
|---|---|---|---|
| `0x0 0x80000000` | `0x3c800000` | `0xbc7fffff` | 968 |
| `0x0 0xc0000000` | `0x20000000` | `0xdfffffff` | 512 |
| `0x0 0xe0d00000` | `0x1f300000` | `0xffffffff` | 499 |
| `0x8 0x80000000` | `0x80000000` | `0x8ffffffff` | 2048 |

Celkem `0xfbb00000` = 4027 MiB. `/proc/iomem` ukazuje stejné čtyři oblasti
(`device-info/iomem.txt:98-103`). Díry `0xbc800000–0xbfffffff` a
`0xe0000000–0xe0cfffff` bootloader kernelu nepředává.

Oproti gta4xl (`0x3AB00000`, `0x20000000`, `0xE1900000 + 0x1E700000`,
`0x80000000`) se liší první a třetí oblast – hodnoty z gta4xl by na troice
byly špatně.

### 8.2 Rezervovaná paměť s pevnou adresou (`device-info/live.dts:2576-2714`)

Bootloader uzly proti zdrojovému DTS nezměnil.

| Oblast | Adresa | Velikost | K čemu (downstream) |
|---|---|---|---|
| `seclog_mem` | `0xc0000000` | `0x80000` | log secure světa |
| `abox_rmem` | `0xe9400000` | `0x400000` | audio DSP |
| `bootloader_fb` | `0xec000000` | `0x4000000` | **framebuffer bootloaderu** |
| `modem_if` | `0xf0000000` | `0x8a00000` | sdílená paměť modemu |
| `debug_snapshot` | `0xf9000000` | `0x1610000` (7 podoblastí, konec `0xfa610000`) | ladicí logy; `log_pstore` je na `0xf9c10000` |
| `gnss_if` | `0xfb000000` | `0x800000` | GNSS |
| `chub_rmem` | `0xfd000000` | `0x50000` | sensor hub |

Pro mainline DTS je nutná jen `bootloader_fb`. U ostatních nevím, zda do
nich firmware zapisuje i bez downstream ovladačů (kandidát je hlavně
`seclog_mem`). Bezpečnější je v první iteraci rezervovat všechny jako
`no-map`; stojí to asi 240 MiB ze 4 GB.

`ramoops@f9d10000` z Chimacova dtsi (D2) na troice nesedí – padá doprostřed
`log_pstore` (`0xf9c10000 + 0x200000`).

### 8.3 Ostatní

| Položka | Hodnota | Zdroj |
|---|---|---|
| Chip ID | downstream hlásí `EXYNOS9610`, revize 2 → registr po maskování = `0xE9610000` | `device-info/soc0.txt`; downstream `exynos-chipid.c:88-93`, `exynos-soc.h:30` |
| Označení SoC bootloaderem | `mmi,sam_hw = "Exynos9609"` | `device-info/live.dts:3954` |
| Tlačítka v klidu | gpio-19/20/21 (`gpa1` 5/6/7) = `in hi IRQ` → active-low | `device-info/gpio.txt:11-13` |
| Panel | `ddi_id = 0x01047291` → Himax HX83112A, 1080 × 2520 | `device-info/live.dts:7391`, `dsim_drv.c:1626-1629`, `exynos9610-display-lcd.dtsi:330-333` |
| initrd | `0x84000000` | `device-info/live.dts:3963` |
| Bootloader | odemčený (`flash.locked=0`, `verifiedbootstate=orange`, `bl_state=8`) | `device-info/props.txt` |
| Overlay | `androidboot.dtbo_idx=1` = `dtbo.1` (model „troika“) | `device-info/cmdline.txt`, `device-info/dtbo_a/` |
| cmdline od bootloaderu | obsahuje `clk_ignore_unused` (pochází z `chosen` v základním DTB) | `device-info/cmdline.txt`, `exynos9610.dtsi:121` |

### 8.4 Co z vyčtených dat zjistit nešlo

- **Formát pixelů framebufferu bootloaderu** – z běžícího Androidu ho nelze
  vyčíst; zůstává TODO pro první test.
- **Začátek bootu v `dmesg`** – výpis začíná až ve 120. sekundě (buffer se
  přetočil), takže chybí řádky `fb_rmem_setup`, `Memory:` a hlášení chipid.
  Hodnoty jsou doložené jinak (live DTB, `soc0`), nic podstatného nechybí.
- Zdrojáky v `refs/downstream-kernel` jsou větev `lineage-24.0` (4.14.357),
  telefon běží na `lineage-22` (4.14.113). Klíčové hodnoty jsem proto bral
  z `live.dts`, ne ze zdrojáků.

### 8.5 Soukromí

`device-info/cmdline.txt`, `dmesg.txt`, `live.dtb` a `live.dts` obsahují
sériové číslo a další identifikátory telefonu. Z `props.txt` jsem MAC adresy
a UID odstranil. Před sdílením adresáře (např. do bug reportu) je potřeba
tyto soubory pročistit.

---

## 9. Dodatky z fáze 2 (2026-11-10)

Při práci na větvi kernelu se několik závěrů z fáze 1 upřesnilo nebo změnilo.

### 9.1 Nové chyby v patchích, které našel až `dtbs_check`

Statický průzkum ve fázi 1 je nezachytil; `make CHECK_DTBS=y W=1` na
`exynos9610-gta4xl.dtb` po aplikaci série v1 na 7.3.3 vypsal 299 řádků
(`build/dtbs_check-baseline.log`).

| # | Chyba | Dopad | Oprava |
|---|---|---|---|
| N1 | 53× překlep `samsung.pin-pud` / `samsung.pin-drv` (tečka místo čárky) v řadičích FSYS a TOP | driver vlastnosti nenajde (`pinctrl-samsung.c:42-43`), pull a drive strength se u sd, i2c, uart, spi skupin tiše ignorují | commit „Fix misspelled samsung,pin-pud/pin-drv properties“ |
| N2 | banky `gpa0–gpa2` mají `#interrupt-cells = <3>` | binding i driver (`pinctrl-exynos.c:335`, `irq_domain_xlate_twocell`) čekají 2; hodnota 3 je opsaná z vendor DTS | commit „Use two interrupt cells…“ |
| N3 | uzly `xclkout0`, `xclkout1` nemají příponu `-pins` | chyba schématu | commit „Rename xclkout pin groups…“ |
| N4 | gta4xl nemá povinné `model` | chyba schématu | commit „Add the required model property“ |
| N5 | `framebuffer@ca000000` v `chosen` má unit address bez `reg` | varování dtc | commit „Drop unit address from the simplefb node“ |

Po opravách je `dtbs_check` s `W=1` pro gta4xl i troiku **bez jediného
řádku** (`build/dtbs_check-fixed.log`).

### 9.2 Opravené závěry z fáze 1

- **P1 a P2 (CMGP) jsem neopravoval.** Zdroje si odporují: Motorola
  downstream má přerušení u 24 bank `gpm` (bez `gpm8` a `gpm20`), Chimacovo
  dtsi u 16 a navíc `gpm8 = SPI 162`, které downstream nemá vůbec. Bez
  dokumentace SoC nevím, co platí. P1 bez P2 by jen přidalo chybovou hlášku.
  Řadič CMGP minimální boot nepotřebuje, zůstává tedy jako ve v1 (jeho
  externí přerušení nefungují). **TODO: zeptat se autora.**
- **P3 není chyba.** Mainline `exynos850-pinctrl.dtsi` má `interrupt-controller`
  i u `EINTN` banky `gpq0`; je to zavedená konvence.
- **Klíč `deviceinfo_bootimg_dtb_second` je správně.** `boot-deploy` 0.25.0
  čte právě ten (`boot-deploy-functions.sh:749`). Výstup `bootimg_analyze`
  (`deviceinfo_dtb_second`) je v tomhle zastaralý, takže původní výtka
  k MR !6458 platila.
- **uniLoader do DECON registru nezapisuje.** `soc_init()` v
  `soc/exynos/exynos9610.c` je mrtvý kód – nikde se nevolá (grep přes celý
  strom). uniLoader na troice tedy jen kreslí do framebufferu.
- **Watchdog:** downstream obsluhuje `watchdog_cl0@10050000` a
  `watchdog_cl1@10060000` (`exynos9610.dtsi:2463-2479`). Jestli je bootloader
  nechává běžet, nevím; mainline je bez clocků neobslouží. Možná příčina
  samovolného restartu při testu.

### 9.3 Stav kontrol

| Kontrola | Výsledek |
|---|---|
| `make dtbs` (gta4xl, troika) | OK |
| `make CHECK_DTBS=y W=1` | 0 varování, 0 chyb |
| `make dt_binding_check` (`samsung-boards.yaml`) | OK |
| build kernelu (`make Image dtbs`, gcc 16.2.0, konfigurace balíčku) | OK za 18 min 39 s, 0 varování; `Image` 34,5 MB |
| `pmbootstrap kconfig check linux-motorola-troika-mainline` | „kconfig check succeeded“; 1 poznámka (INPUT_EVDEV `y` místo `m`) |
| build uniLoaderu (`troika_defconfig`) | OK; linker varuje před RWX segmentem (vlastnost uniLoaderu) |
| testovací image A/B/C | hlavičky shodné se zálohou, obsah ověřen (`build/verify/`) |
| build balíčků přes pmbootstrap | **neprovedeno** (sudo) |
| spuštění kernelu (QEMU ani telefon) | **neprovedeno** |
| `scripts/checkpatch.pl` na 17 patchů | bez chyb stylu; hlásí jen chybějící `Signed-off-by` u mých 14 commitů, tvar `Co-Authored-By` a obecné upozornění na MAINTAINERS |

`Signed-off-by` jsem za tebe nepřidával – je to tvoje prohlášení (DCO).
Kernel má pro práci s AI asistenty vlastní pravidla
(`Documentation/process/coding-assistants.rst`); před případným odesláním
upstream je potřeba značky v commitech upravit podle nich.

---

## 10. Výsledky testů na telefonu (2026-11-10)

Vše přes `fastboot boot`, nic se nezapisovalo. Podrobný deník je v PROGRESS.md.

### 10.1 Co je teď ověřené na hardwaru

| Položka | Hodnota | Jak ověřeno |
|---|---|---|
| `fastboot boot` | bootloader MBM-2.2 ho podporuje | testy A, E1, E3, D |
| Formát boot image | hlavička v1, DTB ve „second“, bez AVB patičky stačí | E1 (původní kernel a ramdisk přebalené `mkbootimg`) nabootoval Android |
| uniLoader jako „kernel“ | bootloader ho přijme a spustí (`text_offset 0`, nekomprimovaný, s arm64 hlavičkou) | E3, D |
| Adresa framebufferu | **`0xed000000`** (16 MiB uvnitř oblasti `bootloader_fb` na `0xec000000`) | D a G: aktivní DMA kanál G0 má `addr=ed000000`; text diagnostiky je na displeji. **Oprava:** původně jsem z rozmazané fotky testu D přečetl `ec000000` – chybně |
| Rozlišení a stride | **1080 × 2520, stride 1080 × 4** | D: `src=9d80438`, `img=9d80438` (výška `0x9d8` = 2520, šířka `0x438` = 1080); text není zkosený |
| Formát pixelů | **`a8r8g8b8`** | D: uniLoader zapisuje bajty B, G, R, A (`lib/simplefb/simplefb.c`, větev `FB_FORMAT_ARGB8888`); štítek `[INFO]` (255, 192, 0) je jantarový, `[ERROR]` (255, 0, 0) červený |
| Aktivní kanál displeje | G0 (`en=4` = `IDMA_OP_STATUS` busy), ostatní tři kanály nulové | D |
| PSCI `SYSTEM_RESET` | **firmware nepodporuje** (volání se vrátilo) | D: `PSCI reset returned` |
| Rychlost RAM před kernelem | zápis je pomalý – purpurová výplň byla vidět jako přejíždění shora | D; cache jsou zřejmě vypnuté |

Položky „Formát pixelů“ a „Stride“ v kap. 5 tím přestávají být TODO.

### 10.2 Co bootloader odmítá

Image A (uniLoader + testovací initramfs jako ramdisk boot image) bootloader
nespustil: fastboot menu zůstalo a telefon se vrátil do LineageOS. E3 (stejný
uniLoader, původní androidí ramdisk) spustil. Rozdíl je jen ramdisk; proč
bootloaderu malý neandroidí ramdisk vadí, nevím (velikost? obsah?). Test E2
(původní kernel + testovací ramdisk) by to potvrdil, zatím neproběhl.

Důsledek pro balíčky: boot image pro troiku nesmí mít „podezřelý“ ramdisk;
jestli projde pmOS initramfs, je potřeba vyzkoušet.

### 10.3 Co ještě ověřené není

- **Mainline kernel sám.** E3 skončil podle prvního pozorování „zamrzlý na logu“,
  ale sledován byl jen krátce. Po zkušenosti z D (pomalá RAM) může kopírování
  uniLoaderu a kernelu trvat desítky sekund. E3 je potřeba zopakovat a počkat.
- Tlačítka, počet CPU, velikost paměti z pohledu kernelu.
- Varianty B a C (mainline DTB ve „second“).

### 10.4 Oprava: framebuffer je na `0xed000000` (2026-11-10 večer)

Tvrzení z kap. 10.1 v původním znění („adresa `0xec000000` potvrzena“) bylo
**chybné** – z rozmazané fotky testu D jsem `ed000000` přečetl jako `ec000000`.
Ostrá fotka testu G (`/tmp/.../images/1.png`, výřez zvětšen) ukazuje
`TROIKA DIAG: console at 0xed000000` a `G0 en=4 src=9d80438 img=9d80438 addr=ed000000`.

Tím se vysvětlují všechny předchozí neúspěchy:

| Test | Kam kreslil uniLoader | Výsledek |
|---|---|---|
| E3, F, H (běžná a krokovací varianta) | `0xec000000` (napevno) | nic vidět, zůstalo logo |
| D, G (diagnostika) | adresa přečtená z DPU = `0xed000000` | text vidět |
| G po `Booting kernel...` | kernel kreslil podle DTB na `0xec000000` | nic vidět, text uniLoaderu zůstal |

Purpurová výplň v D a G začínala na `0xec000000`, proto bylo nejdřív chvíli
„nic“ (resp. černá) a přejíždění začalo, až výplň došla k `0xed000000`.

Důsledky:
- Mainline kernel v E3, F, H a G **možná běžel**, jen kreslil mimo zobrazovaný
  buffer. Nevíme to.
- Závěry o zásobníku, rozložení paměti, „zamrzlém“ displeji a triggeru DECON
  (PROGRESS.md) byly slepé uličky. Zápis do registru DECON není potřeba.
- Opraveno: DTS troiky (`framebuffer@ed000000` s `reg`, rezervace 64 MiB od
  `0xec000000` zůstává), `board-troika.c` a `board-troika-step.c` v uniLoaderu,
  patch v pmaports. Nový hlavní test: `out/troika-test-K.img`.
- Proč bootloader používá právě +16 MiB, nevím. Hodnota byla stejná ve dvou
  bězích; jestli je stálá za všech okolností (např. při normálním bootu bez
  fastbootu), ověřené není.

### 10.5 Test K: mainline kernel bootuje (2026-11-10)

`out/troika-test-K.img` (uniLoader + kernel 7.3.3 se 17 patchi + DTS troiky
s framebufferem na `0xed000000`) nabootoval do testovacího initramfs.
Fotka: `device-info/photos/test-K-mainline-boot.png`.

| Co | Výsledek na telefonu |
|---|---|
| RAM | `MemTotal` ≈ 3,78 GB (4 banky z DTS, 7 rezervací `no-map`) |
| CPU | 8 jader online přes PSCI; PMU A53 i A73 se zaregistrovaly |
| chipid | `soc_id: EXYNOS9610 rev 2` |
| simple-framebuffer | `simpledrm` na `ed000000.framebuffer`, konzole 67×78 znaků, barvy správně |
| gpio-keys | Vol+ = 115, Vol− = 114, Power = 116; stisk i uvolnění, správná polarita |
| cmdline | `console=tty0 fbcon=font:TER16x32` (z konfigurace kernelu; uniLoader cmdline bootloaderu nepředává) |

V tabulce kap. 5 tím přecházejí do stavu „ověřeno na hardwaru“: rozložení RAM,
rezervace, adresa/rozměr/formát framebufferu, GPIO a polarita tlačítek,
timer, PSCI (zapínání jader), GIC, CPU `reg`, chipid.

Neověřeno zůstává: uspání/probuzení, restart a vypnutí z kernelu, dlouhodobá
stabilita (test běžel jen krátce), varianty B a C, chování při normálním
(ne-fastboot) bootu.

### 10.6 Testy A, B, C: co bootloader přijme (2026-11-10)

| Co je v boot image | Výsledek |
|---|---|
| uniLoader + malý neandroidí ramdisk + původní DTB (A) | bootuje |
| uniLoader + původní ramdisk + **mainline DTB** (B) | `fastboot boot` → `FAILED (remote: '')` |
| holý mainline kernel + **mainline DTB** (C) | `FAILED (remote: '')` |

Bootloader tedy vyžaduje ve „second“ DTB, na který mu sedne overlay z `dtbo`
(nebo ho jinak kontroluje – hláška je prázdná). Kap. 3.5 to předpokládala;
teď je to změřené. Dřívější domněnka z kap. 10.2, že bootloaderu vadí malý
ramdisk, **neplatí**.

`deviceinfo` v balíčku (`deviceinfo_dtb="exynos/exynos9610-troika"` +
`deviceinfo_bootimg_dtb_second="true"`) popisuje přesně variantu C, která
neprojde. Balíček je potřeba přepracovat (návrh v PROGRESS.md).
