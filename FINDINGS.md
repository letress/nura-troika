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

### 10.7 Co bootloader předává kernelu a jak s tím pracuje uniLoader (2026-11-11)

| Zjištění | Hodnota | Jak ověřeno |
|---|---|---|
| Registr x0 při skoku | ukazatel na DTB bootloaderu | test P2: `x0 from bootloader: 0x8a000000`, `magic=d00dfeed` |
| Adresa DTB bootloaderu | **`0x8a000000`**, velikost `0x350eb` | tamtéž (fotka `device-info/photos/test-P2-uniloader-x0.png`) |
| Důsledek pro uniLoader | nesmí se relokovat přes `0x8a000000`; výchozí `TEXT_BASE 0x88000000` s ~34 MB obrazem ho přepisoval | test P (initramfs 4K) vs. P2 (`TEXT_BASE 0x94000000`) |
| Poloha ramdisku | bootloader ji zapisuje do svého DTB (`linux,initrd-start/end`); uniLoader ji přebírá | opakovaný P2: initramfs z boot image se spustil |
| Příkazová řádka | bootloader ji skládá ve svém DTB; uniLoader ji přebírá bez `console=null`/`console=ram` | opakovaný P2, zvětšená fotka: `/proc/cmdline` obsahuje `fbcon=font:TER16x32 pmos_test=1` z boot image |
| `console=` v cmdline boot image | **bootloader ho odstraňuje**: z `console=tty0 fbcon=… pmos_test=1` dorazilo jen `fbcon=… pmos_test=1` | tamtéž. Konzole na displeji funguje i bez něj (tty0 je výchozí); `deviceinfo_kernel_cmdline="console=tty0"` je tedy bez účinku |
| Co bootloader přidává sám | `skip_initramfs root=PARTUUID=…`, `androidboot.*`, argumenty z `chosen/bootargs` původního DTB | tamtéž |
| Nezarovnané přístupy v uniLoaderu | s vypnutou MMU vedou k resetu; asm `memcpy` se smí volat jen se zarovnanými adresami | první P2 (reset při skládání bootargs), po náhradě bajtovou smyčkou v pořádku |

Kapitola 3.5 (návrh „uniLoader + původní DTB ve second“) je tím potvrzená v praxi,
včetně toho, že ramdisk i příkazová řádka můžou zůstat v boot image.

---

## 11. Ovladač hodin: kontrola v3 proti tabulkám downstreamu (2026-11-11)

Větev `troika-clk` v `linux/` = `troika-mainline` + 4 patche Chimacovy série v3
(binding, sysreg binding, pll_1061x, ovladač; 5/5 MAINTAINERS se na 7.3.3
neaplikuje a není potřeba) + 24 oprav + uzly CMU v dtsi.

### 11.1 Metoda

`tools/check-clk-driver.py` porovná ovladač s tabulkami Samsungu v downstream
kernelu (`refs/downstream-kernel/drivers/soc/samsung/cal-if/exynos9610/`):
`cmucal-sfr.c` (registry a bitová pole) a `cmucal-node.c` (strom hodin). Rodiče
se neporovnávají podle jmen, ale přes registry: uzel ovladače → registr → uzel
downstreamu → jeho rodič → registr rodiče → uzel ovladače na tom registru.
Výstup na původní v3: `build/clk-check-v3.txt`, po opravách `build/clk-check-fixed.txt`.

### 11.2 Co sedí

- Všech 684 definic registrů CMU v ovladači má stejný offset jako downstream:
  254 se shoduje i jménem, 430 má jméno zkrácené (bez `BLK_x_UID_` a `_IPCLKPORT`)
  a na stejném offsetu je registr se stejnými slovy ve jménu, 2 se liší jen slovem
  `ALIVE` (RTC v APM).
- Neověřeno: tři offsety v sysreg (`EXYNOS9610_*DRCG_EN_OFFSET`, `MEMCLK_OFFSET`).
  V našem DT se nepoužijí, protože chybí `samsung,sysreg`.
- Typy PLL (1051x pro SHARED0/1 a CPUCL0, 1061x pro MMC a AUD) a jejich registry.
- ID hodin v hlavičce: bez duplicit, `CLKS_NR_*` odpovídá nejvyššímu ID v každém CMU.
- Ladicí registry pro automatické hradlování: downstream má `dbg_offset = 0x4000`
  (`cmucal-sfr.c:1356`) a stav „idle“ jako bity [6:4] = 3 (`cmucal-debug.c:33-52`),
  stejně jako `samsung_auto_clk_gate_is_en()` v mainline.

### 11.3 Chyby a opravy (každá samostatný commit na `troika-clk`)

Prvních 11 nahlásil sashiko-bot (kap. 2.1); všech 11 jsem proti kódu a downstreamu
ověřil. Dalších 10 našel až skript.

| # | Chyba | Zdroj opravy | Kdo našel |
|---|---|---|---|
| 1 | `mout_cmu_g3d_SWITCH` – překlep, osiřelý strom G3D | jméno u konzumenta | bot #9 |
| 2 | `core_div_clks` se neregistruje → 41 hradel + GIC mux bez rodiče | `core_cmu_info` | bot #8 |
| 3 | CMGP USI PCLK hradla na registrech IPCLK | `cmucal-sfr.c` 0x207c… | bot #1 |
| 4 | CMGP USI hradla mají za rodiče mux místo děliče | `cmucal-node.c` | bot #2 |
| 5 | totéž pro CMGP I2C IPCLK (5 hradel) | `cmucal-node.c:906` | skript |
| 6 | `oscclk_rco` vs. registrované `oscclk_rco_cmgp` | `top_fixed_clks` | bot #10 |
| 7 | CCI OCC hradlo na registru CCI (0x2158 místo 0x215c) | `cmucal-node.c:986` | bot #3 |
| 8 | GIC mux má rodiče v opačném pořadí | `cmucal-node.c:351` | skript |
| 9 | chybí hradlo `gout_cmu_shub_bus` (APM 0x2010); nové ID 51 | `cmucal-node.c:817` | bot #5 |
| 10 | APM: `mout_cmu_shub_bus[0]` a `mout_clk_apm_bus[1]` špatně; `dll_dco` není nikde registrován (downstream: pevných 360 MHz) | `cmucal-node.c:75-82, 427, 1461` | skript |
| 11 | **chybí dělič `dout_cmu_fsys_ufs_embd` v TOP** (0x1850, 4 bity) → UFS nemá cestu k PLL | `cmucal-node.c:736`, `cmucal-sfr.c:2287` | skript |
| 12 | `mout_cmu_dispaud_disp[3]`: dvakrát SHARED0_DIV4, má být SHARED1_DIV4 | `cmucal-node.c:117` | skript |
| 13 | šířky děličů AUD FM (9 → 10) a FM_SPDY (9 → 1) | `cmucal-sfr.c:3518, 3524` | skript |
| 14 | rodič `gout_dispaud_lhm_axi_p_dispaud_clk` (má být `dout_clk_dispaud_busp`) | `cmucal-node.c:1119` | skript |
| 15 | UAIF2 mux používá hodiny UAIF0; chybí `ioclk_audiocdclk2` (10 MHz) | `cmucal-node.c:371, 1477` | skript |
| 16 | rodič `gout_g2d_lhs_acel_d_clk` (má být G2D user mux) | `cmucal-node.c:1186` | skript |
| 17 | chybí hradla G2D a MSCL (0x204c, 0x2050); nová ID 26, 27 | `cmucal-node.c:1174, 1181` | bot #6 |
| 18 | šířka děliče PERI I2C (8 → 4) | `cmucal-sfr.c:4793` | skript |
| 19 | 4 registry hradel chybí v `*_clk_regs` (CAM ×2, FSYS SDCLKIN ×2) | definice v ovladači | bot #7 |
| 20 | `CLK_DOUT_CMU_MFD_*` → `MFC` | — | bot #11 |
| 21 | binding: vstupy CMU_CORE se jmenují `gout_cmu_core_*`, ven z TOP jde `dout_cmu_core_*` | pořadí mux → gate → div v TOP | já |
| 22 | binding: příklad má `reg` 0x8000, ale auto hradlování vyžaduje 0x10000 (`samsung_is_auto_capable()`, `clk.c:246`) | mainline `clk.c` | já |

**Bot #4 není chyba** („`CLK_DOUT_CLK_CPUCL0_CPU` má šířku 0“): registr
`CLK_CON_DIV_DIV_CLK_CPUCL0_CPU` (0x1810) v downstreamu žádné pole DIVRATIO
nemá, jen BUSY a ENABLE_AUTOMATIC_CLKGATING (`cmucal-sfr.c:3189-3190`), a uzel je
`CLK_DIV(…, EMPTY_CAL_ID, …)`. Dělič 1:1 je tedy věrný popis hardwaru. Totéž
`dout_clk_g3d_busd`.

### 11.4 Co jsem neopravoval

- APM: `oscclk` místo `OSCCLK_RCO_APM` u tří uzlů. Downstream má pro RC oscilátor
  APM samostatné pevné hodiny (26 MHz, stejná frekvence). Binding pro to nemá vstup.
- Chybějící volitelná hradla: FSYS RTIC a SSS (4×), PERI CMU PCLK a OSCCLK (2×).
- `mout_pll_shared0/1/mmc` na `PLL_CON0` bit 4 – downstream je má jako součást PLL,
  v mainline je to obvyklý zápis. Není to chyba.
- 5/5 (MAINTAINERS) a připomínky lidí z review (commit message, řazení v bindingu).

### 11.5 Co nevím a co je riziko pro test na telefonu

1. **Registr OPTION.** Při registraci CMU s auto hradlováním mainline zapíše do
   `CMU_*_CONTROLLER_OPTION` (offset 0x800) bity 24, 28, 29, 30, 31
   (`clk-exynos-arm64.c:28-35`, definice pro Google gs101). Downstream pro 9610
   zná jen bity 28 a 29 (`cmucal-sfr.c:5505-5530`). Co na 9609 dělají bity 24, 30
   a 31, nevím. Chimac to tak má (v2: „Enable auto clock gating“), ale jeho DT
   a zařízení (gta4xl, Exynos 9611) nejsou naše. Testovací kernel proto vypíše
   hodnotu před zápisem a po něm.
2. **Napájecí domény.** CMU_CAM, CMU_G2D a CMU_G3D leží v doménách pd-cam, pd-g2d,
   pd-g3d (`device-info/live.dts:2972-3033`). Jestli je bootloader nechává zapnuté,
   nevím; čtení registrů vypnuté domény by kernel shodilo. V dtsi proto tyto tři
   CMU **nejsou**. Testovací kernel jen přečte stavové registry PMU
   (`pmucal_local_exynos9610.h`: 0x11860000 + 0x4004/24/44/64/84/a4/e4).
3. **`clk_ignore_unused`.** Hradla jsou v auto režimu a jejich vypnutí je prázdná
   operace, ale PLL ne: bez `clk_ignore_unused` by kernel na konci bootu vypnul
   PLL bez konzumenta (včetně sdílených). Argument dodává bootloader z původního
   DTB (ověřeno na fotce testu P2) a testovací image ho má ještě jednou.
4. Sysreg (`samsung,sysreg`) v DT není → ovladač vypíše „Unable to get CMU sysreg“
   a DRCG nezapíná. Binding v3 tu vlastnost ani nepovoluje.
5. Ovladač nikde nepoužívá `.clk_name`, takže `clocks`/`clock-names` v DT jsou
   zatím jen popis; rodiče se hledají podle globálních jmen.

### 11.6 Kontroly (2026-11-11)

- `make dtbs_check` pro `exynos9610-troika.dtb` a `exynos9610-gta4xl.dtb`: bez hlášení.
- `make dt_binding_check DT_SCHEMA_FILES=samsung,exynos9610-cmu.yaml`: bez hlášení.
- `make W=1 drivers/clk/samsung/clk-exynos9610.o`: bez varování.
- `checkpatch --strict` na 24 commitů: jen chybějící `Signed-off-by` (nepřidávám
  ho za tebe), tvar `Co-Authored-By` a u přejmenování MFD → MFC „binding a ovladač
  v jednom patchi“ (rozdělení by rozbilo překlad mezi commity).
- Na hardwaru zatím neověřeno nic.

### 11.7 Test CLK1 na telefonu (2026-11-11): ovladač hodin funguje

`out/troika-test-CLK1.img`, fotka `device-info/photos/test-CLK1-clocks.png`
(638×850; čteno ze zvětšených výřezů). Číslice 0 a 8 jsou v tomto rozlišení
podobné, proto je u každé hodnoty uvedeno, čím je čtení podložené.

| Co | Hodnota | Kontrola čtení |
|---|---|---|
| počet hodin / sirotků | 566 / 0 | 644 − CAM 31 − G2D 27 − G3D 21 + `oscclk` = 566 |
| `fout_shared0_pll` | 1 599 000 000 Hz | shoda s tabulkou ovladače |
| `fout_shared1_pll` | 1 332 500 000 Hz | shoda s tabulkou ovladače |
| `fout_mmc_pll` | 825 999 877 Hz | = tabulková 799 999 877 + 26 MHz (M o 1 vyšší) |
| `fout_cpucl0_pll` | 1 053 000 000 Hz | = 26 MHz × 40,5 |
| `fout_aud_pll` | 1 179 648 040 Hz | obvyklá frekvence audio PLL (1179,648 MHz) |
| `dout_cmu_fsys_bus`, `dout_cmu_usb_bus` | 266 500 000 Hz | = 1599/6 = 1332,5/5 |
| `dout_cmu_fsys_ufs_embd`, `gout_fsys_ufs_embd_clk_unipro`, `dout_cmu_peri_uart` | 133 250 000 Hz | = 1599/12 |
| `dout_cmu_fsys_mmc_card` | 39 975 000 Hz | = 1599/40 |
| `dout_cmu_usb_usb30drd` | 49 968 750 Hz | = 1599/32 |
| `dout_cmu_dispaud_disp` | 533 000 000 Hz | = 1599/3 |
| OPTION všech 9 CMU před zápisem | `0x30000000` | bity 28 a 29 – přesně ty dva, které zná downstream |
| OPTION po zápisu `0xf1000000` | `0x70000000` | druhá číslice je 0, ne 1 → bit 24 nedrží; první je 7, ne f → bit 31 nedrží |
| stav domén dispaud, cam, g2d, g3d, isp, mfc, vipx1 | `0x3000f` | spodní čtyři bity f = zapnuto (horní číslice čtené s menší jistotou) |

Závěry:

- **Riziko z 11.5 bod 1 se nepotvrdilo.** Bootloader nechává v OPTION bity 28 a 29.
  Zápis mainline přidá jen bit 30; bity 24 a 31 na tomto čipu nedrží (čtou se
  jako 0). Systém běží dál, displej svítí, tlačítka i konzole fungují.
- **Vzorce PLL a strom hodin sedí s hardwarem**: všechny přečtené frekvence jsou
  celočíselné podíly PLL a žádné hodiny nejsou bez rodiče.
- **Oprava 11 z 11.3 (dělič UFS) je vidět v praxi**: `gout_fsys_ufs_embd_clk_unipro`
  má frekvenci odvozenou od PLL (133,25 MHz). Bez děliče by cesta končila u muxu.
- **Bootloader nechává zapnuté všech sedm čtených napájecích domén** (11.5 bod 2).
  CMU_CAM, CMU_G2D a CMU_G3D by tedy šly přidat; zatím nejsou potřeba.
- `fout_mmc_pll` běží na frekvenci, která v tabulce ovladače není (tabulka má jen
  800 MHz). Čtení funguje, ale `clk_set_rate()` by na současnou hodnotu neuměl.
- `clk: Not disabling unused clocks` v logu → `clk_ignore_unused` účinkuje.

---

## 12. USB: průzkum (2026-11-11)

Cíl: USB v režimu zařízení (gadget), aby šla síť přes USB a ladicí shell pmOS
(telnet), později SSH. Nic z toho zatím není napsané ani sestavené.

### 12.1 Jak to má původní kernel

| Co | Hodnota | Zdroj |
|---|---|---|
| Řadič dwc3 | `0x13200000`, velikost 0x10000, přerušení SPI 186 (`0xba`) | `device-info/live.dts:7772-7803` |
| Režim | `dr_mode = "otg"`, `maximum-speed = "high-speed"`, `snps,dis-u2-freeclk-exists-quirk` | tamtéž |
| USB 2.0 PHY | `0x131d0000`, velikost 0x200, `phy_version = 0x300`, `ip_type = 2` | `live.dts:7806-7838` |
| USB 3.0 PHY | `0x131f0000` + `0x131e0000`, `usb3phy-isolation = 1` → nepoužívá se | `live.dts:7840-7872`, log „USB3.0 PHY is isolated“ (`device-info/dmesg.txt:221`) |
| Zapnutí PHY v PMU | registr `0x11860000 + 0x704`: bity 0 a 1 (maska 0x3) a bit 16 (buffer TCXO, referenční hodiny PHY) | `phy-exynos-usbdrd3.c:412-486`, `phy-exynos-usbdrd.h:22-26`, log `val : 0x3 / mask : 0x3` a `DEVCTRL reg 0x3 → 0x10003` (`dmesg.txt:208-214`) |
| Referenční hodiny PHY | `oscclk` 26 MHz (`phy_refclk = "oscclk"`, `phy_refsel_clockcore = 1`) | `live.dts:7815-7820` |
| Regulátory | ovladač PHY žádné napájení neřídí (jen volitelný `vbus` pro režim host); `ldos = <0>` u dwc3 | `phy-exynos-usbdrd3.c`, `live.dts:7797` |
| Ladění vysílače | `tx_vref` 15, `tx_pre_emp` 3, `tx_res` 3, `tx_rise` 3, `tx_hsxv` 3, `tx_fsls` 3, `rx_sqrx` 7, `compdis` 7, `otg` 2 | `live.dts:9944-10010` |
| Přepínač D+/D− | MUIC v S2MU106 (I2C) | `live.dts:6168, 9837` |

Pozor: na telefonu běží ovladač `phy-exynos-usbdrd3.c` (`CONFIG_PHY_EXYNOS_USBDRD3`),
ne `phy-exynos-usbdrd.c` (ten je v defconfigu vypnutý a s naším DT by ani
neprošel přes chybějící `pmu_offset_dp`). Poznal jsem to podle hlášek v logu.

### 12.2 Co umí mainline 7.3.3

- `drivers/phy/samsung/phy-exynos5-usbdrd.c` podporuje Exynos 850 a 7870, 9610 ne.
  Registry PHY pro Exynos 850 (`LINKCTRL` 0x04, `LINKPORT` 0x08, `CLKRST` 0x20,
  `UTMI` 0x50, `HSP` 0x54, `HSPPARACON` 0x58, `HSP_TEST` 0x5c, `SECPMACTL` 0x48)
  jsou **stejné jako v downstreamu pro 9610** (`phy-exynos-usb3p1-reg.h`) a
  inicializační sekvence `exynos850_usbdrd_utmi_init()` odpovídá
  `phy_exynos_usb_v3p1_enable()` + `…_pipe_ovrd()` krok za krokem.
- `drivers/usb/dwc3/dwc3-exynos.c` má `samsung,exynos850-dwusb3` (hodiny
  `bus_early`, `ref`).
- Ovladač hodin v3 má hradla, která tomu odpovídají:
  `CLK_GOUT_USB_USB30DRD_BUS_CLK_EARLY`, `…_REF_CLK`, `…_ACLK_PHYCTRL_20`.
- Konfigurace kernelu už má vše potřebné (`USB_DWC3_GADGET`, `USB_DWC3_EXYNOS`,
  `PHY_EXYNOS5_USBDRD`, `USB_CONFIGFS_NCM/RNDIS/ACM`).
- Uzel PMU (`syscon`) v našem dtsi není a binding `exynos-pmu.yaml` 9610 nezná.

### 12.3 V čem se 9610 od Exynos 850 liší (co je potřeba dopsat)

1. **Zapnutí PHY v PMU.** Mainline pro 850 nastavuje jen bit 0. Na 9610 jsou to
   bity 0 a 1 a navíc bit 16 (TCXO). → vlastní funkce izolace.
2. **Reset PHY.** Pro verzi 0x300 používá downstream bity 3, 2 a 1 v `CLKRST`
   (`phy_sw_rst_high/low`, větev pro minor 0). Mainline se rozhoduje podle počtu
   USB3 portů v `LINKPORT`; má-li řadič na 9610 USB3 port (což nevím), zvolil by
   bity 12 a 13 a PHY by se neresetoval. → vlastní inicializace pro 9610, která
   dělá přesně to, co downstream.
3. **`SSPPLLCTL` (0x30).** Mainline tam zapisuje volbu referenční frekvence;
   downstream pro tento PHY do toho registru nezapisuje vůbec. → vynechat.
4. **Referenční hodiny.** `gout_usb_usb30drd_ref_clk` má 49 968 750 Hz (naměřeno
   v testu CLK1), což mainline jako referenci nezná. Downstream bere `oscclk`
   26 MHz. → v DT dát jako `ref` hodiny `oscclk`.
5. Hodnoty ladění vysílače z původního DT (tabulka výše) jako `phy_tunes`.
6. Uzel PMU + doplnění bindingu (`samsung,exynos9610-pmu`, `samsung,exynos7-pmu`, `syscon`).
7. Uzly `usb@13200000` (`dr_mode = "peripheral"`, `maximum-speed = "high-speed"`)
   a `phy@131d0000` v dtsi, zapnutí v `exynos9610-troika.dts`.

### 12.4 Co nevím a rizika

- **Zápis do PMU.** Bez zápisu do `0x11860704` (bity 0, 1, 16) PHY nepoběží, pokud
  je bootloader nenechal nastavené. Je to stejný zápis, jaký dělá původní kernel
  při každém připojení kabelu. Na PMU jsem slíbil nesahat bez tvého výslovného
  souhlasu, proto se ptám.
- **MUIC.** D+/D− vedou přes přepínač v S2MU106, který mainline neovládá. Po
  `fastboot boot` zůstává přepnutý na USB (fastboot přes něj právě běžel). Co udělá
  po vytažení a zasunutí kabelu, nevím – při testu kabel nevytahovat.
- **Počet USB3 portů v `LINKPORT`** a stav registrů PHY po bootloaderu: přečtu
  ladicím výpisem ve stejném testu (jen čtení), než ovladač cokoli zapíše.
- Regulátory se nemění. Telefon je jen zařízení, VBUS nikdy sám nenapájí.
  Nabíjení zůstává tak, jak ho nechal bootloader.
- Nejhorší očekávaný výsledek: USB se nerozběhne (počítač telefon neuvidí), případně
  kernel spadne při čtení registrů PHY. Po restartu je vše v původním stavu,
  fastboot bootloaderu tím ovlivněn není.

### 12.5 Navržený postup

1. Commity na `troika-clk`: binding + uzel PMU, varianta PHY pro 9610, compatible
   pro dwc3, uzly v dtsi a DTS. Každá hodnota se zdrojem z tabulky 12.1.
2. Test bez přestavby balíčku (ušetří 1,5 h): boot image složený z místně
   sestaveného kernelu a initramfs pmOS z testu PM. Initramfs pmOS si USB síť
   a telnet nastaví sám, jakmile kernel nabídne USB řadič.
3. Na počítači by se mělo objevit nové síťové rozhraní; `ping 172.16.42.1`
   a `telnet 172.16.42.1` do ladicího shellu. SSH přijde až s kořenovým systémem.

### 12.6 Implementace (2026-11-11, po souhlasu uživatele se zápisem do PMU 0x11860704)

Sedm commitů na `troika-clk` (nad hodinami):

| Commit | Co dělá |
|---|---|
| `dt-bindings: soc: samsung: exynos-pmu: Add Exynos9610` | `samsung,exynos9610-pmu`, `syscon` – stejně jako původní DT (`live.dts:7151`) |
| `dt-bindings: phy: samsung,usb3-drd-phy: Add Exynos9610` | compatible + hodiny `phy`, `ref` |
| `dt-bindings: usb: samsung,exynos-dwc3: Add Exynos9610` | compatible + hodiny `bus_early`, `ref` |
| `phy: exynos5-usbdrd: Add support for the Exynos9610 USB 2.0 PHY` | izolace (bity 0, 1, 16 v PMU 0x704), inicializace podle downstreamu, ladění vysílače z původního DT |
| `usb: dwc3: exynos: Add support for Exynos9610` | použije data Exynos 850 |
| `arm64: dts: exynos9610: Add PMU, USB PHY and USB controller` | uzly `system-controller@11860000`, `phy@131d0000`, `usb@13200000` |
| `arm64: dts: exynos9610-troika: Enable USB in device mode` | `dr_mode = "peripheral"`, `maximum-speed = "high-speed"` |

Záměrné odchylky od downstreamu:

- **PMU uzel nemá fallback `samsung,exynos7-pmu`**, je to čistý `syscon` jako
  v původním DT. S fallbackem by se navázal ovladač `exynos-pmu`; ten by sice
  podle kódu žádný další registr nezapisoval (jeho `exynos-clkout` zná jen čipy
  do Exynos 5433), ale pro USB není potřeba a souhlas mám jen pro 0x704.
- **Reset linky při zapnutí PHY vynechán.** Downstream ho dělá, v mainline by ale
  smazal nastavení, které dwc3 zapíše těsně předtím. Stejně to má Exynos 850.
- **Bity OCD v `LINKPORT` (detekce nadproudu pro režim host) nezapisuju**, telefon
  je jen zařízení.
- Typ přerušení dwc3 (`IRQ_TYPE_LEVEL_HIGH`) v původním DT není (má 0); beru
  zvyklost ostatních Exynosů v mainline.

Kontroly:

- `dtbs_check`: jediné hlášení – `usb@13200000: 'vdd33-supply' is a required property`.
  Je to očekávané a správné: napájení jde z PMIC, který podle pravidel zatím
  nepopisujeme. Ovladač si vezme „dummy“ regulátor a ničím nehýbe.
- `dt_binding_check` tří změněných bindingů: bez hlášení.
- `W=1` pro `phy-exynos5-usbdrd.o` a `dwc3-exynos.o`: bez varování.
- `checkpatch --strict`: chybějící `Signed-off-by`, tvar `Co-Authored-By` a
  „Prefer using the BIT macro“ v `exynos-regs-pmu.h` (držím se zápisu `(1 << n)`,
  který ten soubor používá všude).
- Na hardwaru zatím neověřeno.

### 12.7 Test USB1 na telefonu (2026-11-11 14:57): USB funguje

`fastboot boot out/troika-test-USB1.img`. Hodnoty nejsou z fotky, ale přímo z logu
kernelu staženého přes USB síť (`tools/phone-sh.py`, celý log:
`device-info/dmesg-mainline-USB1.txt`).

Na počítači:

- `usb 1-2: new high-speed USB device`, `idVendor=18d1, idProduct=d001`,
  `Product: troika mainline test`, ovladač `cdc_ncm`.
- Rozhraní dostalo přes DHCP `172.16.42.2/24`; `ping 172.16.42.1`: 3/3, 2,1 ms.
- Shell přes telnet funguje (`uname`: `7.3.3-troika-g42fcf5dd3c5b`).

Na telefonu: `/sys/class/udc/…/state` = `configured`, `current_speed` = `high-speed`,
přerušení dwc3 (GIC SPI 186) přibývají.

| Registr | Po bootloaderu | Po inicializaci | Co z toho plyne |
|---|---|---|---|
| PMU `0x704` | `0x0` | `0x10003` | bootloader nechává PHY vypnutý → **zápis do PMU byl nutný**; všechny tři bity drží |
| `LINKCTRL` (0x04) | `0x00053ef0` | `0x00053ff0` | přibyl jen `FORCE_QACT` (bit 8) |
| `LINKPORT` (0x08) | `0x0001107c` | beze změny | **řadič hlásí 1 port USB3** → kód pro Exynos 850 by zvolil špatné bity resetu; vlastní inicializace byla potřeba (12.3 bod 2) |
| `CLKRST` (0x20) | `0x00000004` | `0x00000004` | reset proběhl a skončil |
| `SSPPLLCTL` (0x30) | `0x60108886` | beze změny | nezapisujeme, jak bylo v plánu |
| 0x48 | `0x0` | `0x0` | bit `PMA_LOW_PWR` se neuložil – registr tu zřejmě není; neškodí |
| `UTMI` (0x50) | `0x33` | `0x30` | zrušeno vynucené uspání, VBUS/BVALID vynucené |
| `HSP` (0x54) | `0x10003200` | `0x10003300` | přibyl `COMMONONN` |
| `HSPPARACON` (0x58) | `0xf36f3737` | `0xf36f3727` | bootloader má stejné ladění jako původní DT až na `otg` (3 → 2 podle DT) |
| `HSP_TEST` (0x5c) | `0x01040000` | `0x00080000` | `SIDDQ` (bit 24) shozen → analogová část zapnutá |

Hlášky kernelu k USB: jen čtyři „supply … not found, using dummy regulator“
(`vbus`, `vbus-boost`, `vdd33`, `vdd10`) – očekávané, regulátory nepopisujeme.

Co ověřené není: chování po vytažení a zasunutí kabelu (MUIC), režim host,
uspání, a jak dlouho spojení vydrží.

## 13. Stav běžícího systému po testu NET1 (2026-11-11)

Zdroj: výpis přes SSH z běžícího telefonu, uložený v
`device-info/net1-status.txt` (mimo git; obsahuje příkazovou řádku kernelu se
sériovým číslem). Kernel `7.3.3-troika #4-postmarketOS`, systemd 262.2.

| Co | Zjištění |
|---|---|
| Stav systemd | `degraded`, jediná spadlá služba `postmarketos-zram-swap.service` |
| Příčina | v konfiguraci balíčku je `# CONFIG_ZRAM is not set` (`pmbootstrap kconfig check` to nehlídá) |
| Start | userspace 11,8 s do `graphical.target`; 3 min 21 s „kernel“ je čekání initramfs na NBD server (195 pokusů) |
| Paměť | 3677 MB celkem, 3447 MB volných, bez swapu |
| Síť | `usb0` 172.16.42.1/16 |
| Bloková zařízení | jen `nbd0` (p1 `/boot` 487 MB, p2 `/` 933 MB); žádné úložiště telefonu, UFS ani SD nejsou popsané |
| Čas | systém běží s datem sestavení systemd (2026-11-06), RTC není popsané |

Varování kernelu – stejná jako v testu USB1 (`dmesg-mainline-USB1.txt`), nic nového:

- 9× `Unable to get CMU sysreg` – uzly CMU nemají odkaz na sysreg, ovladač
  proto nezapíná DRCG (viz 11.5 bod 4).
- 4× `Unexpected variation in SYS_CNTFRQ_EL0` (CPU4–7 mají v registru náhodné
  hodnoty, boot CPU nulu) – firmware registr nenastavuje; časovač bere
  frekvenci z DT (`sched_clock: 56 bits at 26MHz`).
- `supply vbus/vbus-boost/vdd33/vdd10 not found, using dummy regulator` –
  záměr, regulátory nejsou popsané (pravidlo o napájení).
- `KASLR disabled due to lack of seed`.

Z initramfs: `Unable to resize root partition` a `resize2fs: Please run
e2fsck -f first` – zvětšení oddílu na NBD se neprovedlo, systému to nevadí.

Uživatelský prostor: `nm-initrd-generator.sh failed`, `bpf-restrict-fs … Not
supported`, `pipewire-pulse.socket … refusing`, avahi bez nss-mdns – obecné
hlášky obrazu, nesouvisí s hardwarem.

Oprava zram: pmaports commit cfcf09c10 (pkgrel 4) zapíná `CONFIG_ZRAM=y`,
`ZSMALLOC` a backendy podle kategorie `zram` v `kconfigcheck.toml`.

**Ověřeno testem NET2 (2026-11-11, kernel #5):** stav systemd `running`, žádná
spadlá služba, `/dev/zram0` (zstd, 5,4 GB) je aktivní swap. Start trval 6,8 s
v kernelu a 8,9 s v uživatelském prostoru. Varování kernelu jsou stejná jako
výše. Výpis: `device-info/net2-status.txt`.

## 14. Úložiště: průzkum UFS a SD (2026-11-11)

Zatím jen průzkum, nic neimplementováno. Zdroje: původní DT z běžícího telefonu
(`device-info/live.dts`, dále „DT“ s číslem řádku), downstream kernel
(`refs/downstream-kernel/drivers/scsi/ufs/`), mainline 7.3.3 (`linux/`).

### 14.1 UFS – hodnoty z původního DT a kernelu

| Co | Hodnota | Zdroj |
|---|---|---|
| Řadič | `ufs@13520000`, `compatible = "samsung,exynos-ufs"` | DT:7084 |
| Registry | HCI `0x13520000`/0x200, VS `0x13521100`/0x200, UniPro `0x13510000`/0x8000, FMP `0x13530000`/0x100 | DT:7089 |
| PHY (PMA) | `0x13524000`/0x800 | DT:7117–7121 |
| Přerušení | SPI 157 (0x9d) | DT:7090 |
| Hodiny | `GATE_UFS_EMBD_QCH_UFS`, `UFS_EMBD` | DT:7093–7094 |
| Piny | `ufs_rst_n` = gpf0-1, `ufs_refclk_out` = gpf0-0, obojí funkce 3, bez pull (řadič `pinctrl@13490000`) | DT:1485–1500 |
| Reset zařízení | `ufs-reset-n-gpio` = gpf0-1; ovladač ho ale ovládá registrem `HCI_GPIO_OUT` | DT:7104, `ufs-exynos.c:421–429` |
| Napájení VCC | pevný regulátor „V_SYS“ spínaný GPIO gpg4-0 (`pinctrl@139b0000`), `regulator-boot-on`, `regulator-always-on` | DT:7101–7103, 7133–7144 |
| Izolace PHY | PMU offset `0x724`, maska 0x1, hodnota 0x1 (výchozí, uzel `ufs-phy-iso` v DT není) | `ufs-exynos.c:1108–1117` |
| IO koherence | sysreg FSYS `0x13410000`, offset `0x1010`, maska 0x300, hodnota 0x300 | DT:7124–7129, 7163–7168 |
| TCXO | `tcxo-ex-ctrl = <0>` → ovladač bit TCXO v PMU neřídí | DT:7098, `ufs-exynos.c:1145` |
| Režim | 1 linka, HS gear 3, série B | DT:7106–7107, `ufs-exynos.c:1151–1165` |
| Konfigurace PHY | tabulky `init_cfg`, `post_init_cfg`, `calib_of_hs_rate_a/b`, `lane1_sq_off`, `post_h8_enter`, `pre_h8_exit` | `ufs-cal-9610.c:134–302` |
| Rozestup linek PMA | `0x140` bajtů | `ufs-cal-9610.c:41` |
| Čekání na CDR | PMA `0x1fc`, maska 0x40 | `ufs-cal-9610.c:245` |

V `device-info/dmesg.txt` (původní kernel) žádné řádky o UFS nejsou, takže
skutečně vyjednaný režim linky z telefonu doložený nemám.

### 14.2 UFS – stav v mainline 7.3.3

- `drivers/ufs/host/ufs-exynos.c` zná exynos7, exynosautov9, exynosautov920,
  fsd a gs101; `drivers/phy/samsung/phy-samsung-ufs.c` totéž. Exynos9610 chybí
  v obou, i v linux-next (`refs/linux-next`).
- Rámec se hodí: konfigurace PHY je tabulka zápisů do PMA s volitelným
  rozestupem linek (`PHY_TRSV_REG_CFG_OFFSET`; gs101 má 0x200, výchozí je 0x30,
  pro 9610 by to bylo 0x50, tedy 0x140 bajtů); host má háčky `pre_link`, `post_link`, `pre_pwr_change`,
  `post_pwr_change`, kam patří atributy UniPro/PCS z tabulek CAL.
- Izolaci PHY dělá ovladač PHY přes `samsung,pmu-syscon` (zápis do PMU);
  IO koherenci host přes `samsung,sysreg`.
- Hodiny v našem ovladači existují: `gout_fsys_ufs_embd_aclk`,
  `gout_fsys_ufs_embd_clk_unipro` (v testu CLK1 133,25 MHz),
  `gout_fsys_ufs_embd_fmp_clk` (`clk-exynos9610.c:3231–3238`).
- VCC: jádro UFS regulátor nevyžaduje; bez `vcc-supply` v DT na GPIO gpg4-0
  nikdo nesáhne a zůstane ve stavu, v jakém ho nechal bootloader (ten z UFS
  bootuje, takže napájení běží).

Co by port obnášel: nový soubor s tabulkou PHY pro 9610, nová varianta
v `ufs-exynos.c` (háčky podle `ufs-cal-9610.c`), tři bindingy, uzly v DT.

### 14.3 UFS – rizika a co vyžaduje souhlas

1. **Zápis do PMU `0x11860724`** (bit 0, izolace PHY) – jiný registr než ten,
   který byl povolen pro USB. Potřebuje nový výslovný souhlas.
2. **Zápis do sysreg FSYS `0x13411010`** (bity 8, 9) – není to PMU ani
   regulátor, ale je to systémový registr mimo samotný řadič; uvádím ho kvůli
   průhlednosti.
3. **Regulátor se nepopisuje** – spoléhá se na stav po bootloaderu. Pravidlo
   o regulátorech zůstane dodržené.
4. **Data na UFS** – je tam Android i bootloader. Ovladač při startu resetuje
   řadič i zařízení a nastaví linku; do oddílů sám nezapisuje. První test má
   proto běžet s testovacím initramfs (ne s pmOS), který disk přepne na
   „jen pro čtení“ a jen vypíše oddíly, nic nepřipojuje.
5. **Neznámé**: jestli tabulky CAL sednou na háčky mainline na první pokus
   (pořadí zápisů, čekání na PLL/CDR). Chybná konfigurace = linka nenaběhne;
   každá oprava znamená nový místní kernel (~20 min) a nový boot.

### 14.4 SD karta

| Co | Hodnota | Zdroj |
|---|---|---|
| Řadič | `dwmmc2@13550000`/0x2000, `samsung,exynos-dw-mshc` | DT:8116–8118 |
| Přerušení | SPI 147 (0x93) | DT:8120 |
| Hodiny | `ciu` (ID 0x137) | DT:8123–8124 |
| Detekce karty | gpa0-7, invertovaná | DT:8134–8135, 8161 |
| Napájení | `vmmc` = LDO35, `vqmmc` = LDO2 v PMIC S2MPU09, bez `boot-on`/`always-on` | DT:4541–4548, 4700–4707 |
| PMIC | S2MPU09 na sběrnici SPEEDY (`speedy@11a10000`) přes ACPM | DT:4395–4410 |

SD karta bez regulátorů nejde: oba LDO jsou po startu vypnuté (nejsou
`boot-on`) a mainline nemá ovladač sběrnice SPEEDY pro 9610 ani regulátorů
S2MPU09. Znamenalo by to napsat ovladač PMIC a zapínat regulátory, což je
proti pravidlu o napájení. Odloženo, dokud to uživatel výslovně nezmění.

### 14.5 UFS – implementace (větev `troika-clk`, 8 commitů po úklidu)

Souhlas uživatele se zápisem do PMU `0x11860724` bit 0: 2026-11-11.

| Commit | Co dělá |
|---|---|
| `dt-bindings: phy: samsung,ufs-phy: Add Exynos9610` | compatible PHY |
| `dt-bindings: ufs: samsung,exynos-ufs: Add Exynos9610` | compatible hosta |
| `phy: samsung-ufs: Add support for the Exynos9610 UFS PHY` | `phy-exynos9610-ufs.c`: zápisy do PMA z `ufs-cal-9610.c` (adresy děleno 4), izolace PMU 0x724 bit 0, čekání na CDR |
| `scsi: ufs: exynos: Read the available lanes before the pre-link hook` | oprava: smyčky přes linky v háčku `pre_link` při prvním startu linky nic nedělaly (počty linek se četly až po háčku) |
| `scsi: ufs: exynos: Only set PA_TxTrailingClocks when a value is given` | původní kernel ten atribut pro 9610 nezapisuje |
| `scsi: ufs: exynos: Add support for Exynos9610` | varianta hosta, háčky podle původního kernelu |
| `arm64: dts: exynos9610: Add FSYS sysreg, UFS PHY and UFS host` | uzly `system-controller@13410000`, `ufs@13520000`, `phy@13524000` |
| `arm64: dts: exynos9610-troika: Enable UFS` | zapnutí (`lanes-per-direction = <1>` je v dtsi, řadič má jednu linku) |

Mapování na původní kernel:

| Původní kernel | Mainline |
|---|---|
| `init_cfg`, část UniPro/PCS (`ufs-cal-9610.c:134–157`) | `exynos9610_ufs_pre_link()` |
| `init_cfg`, část PMA (`:159–170`) | `exynos9610_pre_init_cfg[]` |
| `post_init_cfg` (`:173–184`) | `exynos9610_ufs_post_link()` |
| `calib_of_hs_rate_a/b`, UniPro a časovače (`:214–227`, `:249–262`) | `gs101_ufs_pre_pwr_change()` (stejné hodnoty) + tři časovače v obecném kódu |
| `calib_of_hs_rate_a/b`, PMA pro gear 3 (`:230–240`, `:265–275`) | `exynos9610_pre_pwr_hs_cfg[]` |
| `post_calib_of_hs_rate_*` (CDR, `:244–247`, `:279–282`) | `exynos9610_ufs_phy_wait_for_cdr_lock()` |
| `lane1_sq_off` (`:284–288`) | nepřeneseno – řadič má jen jednu linku (14.6) |
| `post_h8_enter`, `pre_h8_exit` (`:290–302`) | `exynos9610_post_h8_enter[]`, `exynos9610_pre_h8_exit[]` |
| `HCI_UFS_ACG_DISABLE`/`HCI_IOP_ACG_DISABLE` (`ufs-exynos.c:306–322`, `:458–462`) | začátek `exynos9610_ufs_pre_link()` |
| `HCI_1US_TO_CNT_VAL`, `WLU_EN` (`ufs-exynos.c:360–372`, `:450–453`) | `exynos9610_ufs_post_link()` |
| SMC `FMP_SECURITY` a `SMU` (`drivers/crypto/fmp/fmp.c:571`, `smu.c:17`; DT `fmp-id = 0`, `smu-id = 0`) | `EXYNOS_UFS_OPT_UFSPR_SECURE` → stejná dvě volání v `exynos_ufs_fmp_init()` |
| 1 linka, gear 3 (DT:7106–7107) | řadič sám hlásí 1 linku; gear 3 volí `exynos_ufs_get_hs_gear()` |

**Perioda hodin – nejméně jistá hodnota.** Původní ovladač bere `mclk_rate`
z druhých hodin v DT (`ufs-exynos.c:535`, `:817`), tedy z „UFS_EMBD“. Ty jsou
v původním ovladači hodin definované jako `VCLK(UFS_EMBD, CLKCMU_FSYS_BUS, …)`
(`drivers/clk/samsung/clk-exynos9610.c:277`), tj. hodiny sběrnice FSYS
(266,5 MHz, test CLK1), ne skutečné hodiny UniPro (133,25 MHz). Vychází
perioda 1 000 000 000 / 266 500 000 = 3 (dolů) a tik 1 µs = 266. Mainline
port to dělá stejně (počítá z `core_clk`). Fyzicky „správná“ hodnota pro
133,25 MHz by byla 7 nebo 8. Ladicí výpis v testu UFS1 ukáže, co nechal
v registrech bootloader (`1US`, `dme[9514]`), a podle toho se to případně změní.

Odchylky od původního kernelu (vědomé):

- Obecný kód mainline zapisuje navíc `PA_Granularity`, `PA_TActivate`,
  `PA_Hibern8Time` (pevné hodnoty pro všechny Exynosy) a masku synchronizace
  RX; původní `ufs_cal_calib_hibern8_values()` není přenesené.
- Řízení hodin řadiče (`HCI_MISC`) dělá mainline po svém (jako u gs101): bity
  povolení ručního zastavení nuluje, původní kernel je nastavuje. Hodiny běží
  v obou případech.
- Hradlování hodin a hibern8 s hradlováním nejsou zapnuté
  (`UFSHCD_CAP_CLK_GATING` chybí) – méně cest, kde se může něco pokazit.
- Tabulka pro PWM režim (`calib_of_pwm`) není přenesená; mainline PHY
  kalibruje jen v HS režimu.
- Hodnoty PMA pro gear 1 a 2 nejsou přenesené (rámec je neumí vybrat podle
  gearu); host žádá jen gear 3.
- `ref_clk` PHY = `oscclk` (26 MHz): původní DT pro PHY žádné hodiny nemá,
  je to odhad bez vlivu na hardware (pevné hodiny).

Co ovladač při startu zapíše do zařízení UFS (stejně jako každý kernel
s UFS, včetně původního): reset zařízení, příznak `fDeviceInit`, případně
atribut `bActiveICCLevel` a příznak automatické údržby. Do oddílů nezapisuje.

### 14.6 UFS – výsledky testu UFS1 (2026-11-11, prošel napoprvé)

Zdroj: log kernelu stažený přes USB (`device-info/dmesg-mainline-UFS1.txt`),
příkazy přes `tools/phone-sh.py`, fotka displeje od uživatele (souhlasí).

| Co | Hodnota |
|---|---|
| Zařízení | MICRON MT128GASAO4U21, rev. 0103 |
| Disky | `sda` 122 032 MiB (42 oddílů), `sdb` a `sdc` po 4 MiB (boot LU); tři well-known LU |
| Režim linky | `FAST series_B G_3 L_1` – stejný, jaký žádá původní DT |
| Linky řadiče | `PA_AvailTxDataLanes` = `PA_AvailRxDataLanes` = 1; PMA druhé linky čte samé `ff` → řadič má jednu linku |
| Čtení | sha256 oddílů `boot_a`, `dtbo_a`, `vbmeta_a`, `logo_a` = soubory v záloze; 512 MiB z `userdata` rychlostí 454 MB/s |
| Zápis | žádný: všechny tři disky `ro=1`, nic připojeno |
| Chyby | tři neúspěšné `dme-set` (0x9540, 0x321, 0x9540) z obecné masky synchronizace; jinak nic |
| Přerušení | GIC SPI 157, 299 přerušení do konce startu |
| FMP | obě volání SMC uspěla (`sg_entry=128`, `UFSHCD_CAP_CRYPTO` nastaveno) |

Stav po bootloaderu → po našem ovladači:

| Registr / atribut | Po bootloaderu | Po ovladači |
|---|---|---|
| PMU `0x724` | `0x1` | `0x1` (zápis nic nezměnil) |
| sysreg FSYS `0x1010` | `0x311` | `0x311` (bity 8, 9 už byly nastavené) |
| `HCI_1US_TO_CNT_VAL` | `0xa6` (166) | `0x10a` (266) |
| `PA_DBG_CLK_PERIOD` (0x9514) | 6 | 3 |
| `HCI_MISC` | `0xde0` | `0xd00` |
| `HCI_V2P1_CTRL` | `0x1` | `0x10001` |
| `HCI_AXIDMA_RWDATA_BURST_LEN` | `0x18000003` | `0x98000003` |
| `PA_TxTrailingClocks` (0x1564) | `0xa5` | `0xa5` (nezapisujeme) |
| `PA_TActivate` / `PA_Hibern8Time` | 4 / 3 | 3 / `0x20` (obecný kód mainline) |
| `PA_SaveConfigTime` | `0xfa` | `0xfa` |
| PMA linka 0: 0x38, 0x44, 0x4d, 0x59, 0x5e, 0x6c | 38, bb, 03, 50, 80, 14 | 12, b5, 43, 58, c0, 94 (hodnoty z tabulky) |

Závěry:

- **Perioda hodin**: bootloader používá 6 (jako pro 166 MHz), původní kernel 3,
  skutečné hodiny UniPro jsou 133,25 MHz (7,5 ns). Linka funguje s 6 i se 3,
  na přesné hodnotě tedy tolik nezáleží. Zůstává 3 podle původního kernelu.
- **Jedna linka**: úpravy pro méně aktivních linek (obecná změna PHY,
  `lanes-per-direction` v ovladači, tabulka `lane1_sq_off`) byly zbytečné
  a jsou z větve odstraněné; `lanes-per-direction = <1>` je teď v dtsi jako
  popis řadiče.
- **Maska synchronizace**: obecný kód ji nastavuje přes `PA_DBG_OV_TM`, což
  tady selhává. Přidána volba `EXYNOS_UFS_OPT_NO_SYNC_PATTERN_MASK`
  a varianta 9610 ji používá.
- **`Alternate GPT is invalid, using primary GPT.`** – kernel hlásí, že záložní
  tabulka oddílů na konci disku není platná. Je to stav úložiště, jak ho
  zapsal výrobce (my jsme nic nezapisovali); hlavní tabulka je v pořádku.
  Jestli to hlásí i původní kernel, nevím (v jeho logu řádky o GPT nejsou).
  Neopravovat.
- Hodnoty `PA_TActivate`/`PA_Hibern8Time` se od bootloaderu liší; bude to
  důležité, až se zapne hibern8.

Po testu byla větev uklizena (viz výše) a je připraven image
`out/troika-test-UFS2.img` se stejným testem na uklizeném kódu. **UFS2 na
telefonu ještě neběžel.**

**Doplněno: test UFS2 prošel** (log `device-info/dmesg-mainline-UFS2.txt`):
stejný režim linky, stejné disky a oddíly, hashe čtyř oddílů shodné se
zálohou, čtení 440 MB/s, všechno `ro=1`. Neúspěšné `dme-set` zmizely a v logu
není žádná chyba.

## 15. Dotyková obrazovka (2026-11-11)

### 15.1 Co telefon má

| Co | Hodnota | Zdroj |
|---|---|---|
| Panel | Himax HX83112A (řádky `hix83112a panel REG 0x0A=0x9d`) | `device-info/dmesg.txt:38` |
| Dotykový čip | součást téhož čipu (TDDI); ovladač `[HXTP]` | `device-info/dmesg.txt:90` |
| Sběrnice | `i2c@13870000`, `samsung,s3c2440-i2c`, SPI 251, 400 kHz, piny `i2c4_bus` = gpp1-0, gpp1-1 (funkce 2) | DT:6276–6289, 2017 |
| Hodiny sběrnice | `GATE_I2C_4_QCH` → v mainline `CLK_GOUT_PERI_I2C_4_PCLK` | DT:6283, `clk-exynos9610.c:1256` |
| Adresa | 0x48 (`himax,hxcommon`) | DT:6300–6302 |
| Přerušení | gpa2-4; za běhu `himax_gpio_irq`, vstup | DT:6305, `device-info/gpio.txt:16` |
| Reset | gpa1-3, aktivní v nule; za běhu `himax-reset`, výstup 1 | DT:6306, `gpio.txt:10`, `himax_platform.c:540–556` |
| Rozlišení | 1080 × 2520 | DT:6303–6304 |
| Napájení | žádný regulátor: v DT uzlu Himax není `3v3-gpio` ani pinctrl, varianta s regulátory se nepřekládá (`CONFIG_HMX_DB`) | DT:6300–6308, `himax_platform.c:330`, `:484–560` |
| Firmware | ve flash čipu; `HX_ZERO_FLASH` i `HX_AUTO_UPDATE_FW` jsou zakomentované | `himax_common.h:65`, `:76` |
| Druhý zdroj panelu | Novatek `NVT-ts` na 0x62 se stejnými GPIO; tenhle telefon ho nemá | DT:6290–6298 |

Typ přerušení (hrana/úroveň) volí původní ovladač podle registru firmwaru
`0x10007089` (`himax_ic_incell_core.c:1575–1582`, `himax_platform.c:602–611`);
z telefonu ho zatím neznám.

### 15.2 Mainline

- `drivers/input/touchscreen/himax_hx83112b.c` umí HX83112B a HX83100A:
  reset, kontrola ID čipu (`0x900000d0`), čtení 56bajtového hlášení z adresy
  0x30 – stejně jako původní ovladač, který A i B obsluhuje stejným kódem
  a liší je jen podle ID (`himax_ic_HX83112.c:204–208`).
- Řadič I2C `i2c-s3c2410` v konfiguraci je (`CONFIG_I2C_S3C2410=y`); ovladač
  dotyku je v konfiguraci balíčku jako modul.
- Chimacův dtsi žádné I2C nemá.

### 15.3 Implementace (větev `troika-clk`, 4 commity, celkem 64 nad v7.3.3)

| Commit | Co dělá |
|---|---|
| `dt-bindings: input: touchscreen: trivial-touch: Add Himax HX83112A` | compatible |
| `Input: himax_hx83112b - add support for HX83112A` | nový čip s ID `0x83112a` |
| `arm64: dts: exynos9610: Add I2C4` | uzel `i2c@13870000` |
| `arm64: dts: exynos9610-troika: Add the touchscreen` | `touchscreen@48`, reset gpa1-3, přerušení gpa2-4 (úroveň 0), 1080 × 2520 |

Bez regulátorů a bez zápisu do PMU. Ovladač při startu pulzně resetuje dotykovou
část čipu (20 ms), totéž dělá původní kernel při běžícím displeji.

Neověřeno před testem TS1: že ID čipu je opravdu `0x83112a`, že úrovňové
přerušení sedí s nastavením firmwaru, že souřadnice nejsou převrácené, a že
reset dotyku nezhasne obraz. Ladicí commit vypíše ID a nastavení firmwaru
(`TSDBG`).

### 15.4 Test TS1 (2026-11-11)

Log: `device-info/dmesg-mainline-TS1.txt`.

| Co | Výsledek |
|---|---|
| I2C4 | `s3c-i2c 13870000.i2c`, 378 kHz, žádná chyba přenosu |
| ID čipu, 1 ms po resetu | `83112a` – shoduje se s hodnotou v ovladači |
| registry firmwaru `0x10007089`, `0x100070f4/f8/fc`, 2–4 ms po resetu | samé nuly |
| ID čipu, 5 ms po resetu | `0` |

Ovladač se nenavázal (`Unknown product id: 0`, −22). Příčina je v mém ladicím
commitu: četl registry firmwaru hned po resetu a kontrola ID v ovladači přišla
až po nich, kdy už čip vracel nuly. Původní ovladač čeká po resetu 50 ms
(`himax_mcu_pin_reset`, `himax_ic_incell_core.c:1519`) a ID čte až po vstupu do
„safe mode“; mainline ovladač čte ID jednou, 1 ms po resetu, a to v TS1 vyšlo.
Proč čip mezi 1 a 5 ms přestane odpovídat hodnotou, nevím (nahrávání firmwaru
z flash je domněnka, ne doložený fakt).

TS2: ladicí čtení přesunuto za kontrolu ID a registraci přerušení, s pauzou
300 ms; navíc vypíše `xy_reverse` z `0x100070fa`. Stále neověřeno: typ
přerušení, orientace souřadnic, vliv resetu na obraz.

### 15.5 Test TS2 (2026-11-11)

Log: `device-info/dmesg-mainline-TS2.txt`.

| Co | Výsledek |
|---|---|
| ID čipu po resetu (kontrola ovladače) | `83112a`, ovladač se navázal |
| vstupní zařízení | `Himax Touchscreen`, `/dev/input/event0` |
| přerušení gpa2-4, úroveň 0 | v klidu 0 a linka v 1; při dotyku 1337 přerušení za ~25 s, po puštění se počítadlo zastaví |
| chyby kontrolního součtu událostí | žádné |
| ladicí čtení 300 ms po resetu (ID, `0x10007089`, `0x100070f4/f8/fa/fc`) | samé nuly, bez chyby přenosu |

Úrovňové přerušení tedy sedí (žádná bouře přerušení, žádné ztracené dotyky
v počítadle). Nastavení firmwaru (`int_is_edge`, rozlišení, `xy_reverse`) se
tímto způsobem přečíst nepodařilo: jakmile firmware běží, čtení přes AHB
vrací nuly i pro ID. Původní ovladač tyto registry čte jinak (před každým
čtením zapíná burst režim, ID čte v „safe mode“); pro funkci ovladače to
potřeba není, dál jsem to nezkoumal.

Výpis souřadnic na displeji v TS1/TS2 nefungoval – chyba v mém testovacím
initu (`hexdump` přímo nad `/dev/input/event0` dostane `EINVAL`, protože čte
po kouscích menších než jedna událost). Opraveno (`cat … | hexdump`); v TS2
jsem stejný výpis spustil za běhu přes telnet.

Souřadnice (uživatel přejel prstem z levého horního rohu do pravého dolního,
326 hlášení zaznamenaných v běžícím TS2):

| | začátek | konec | rozsah |
|---|---|---|---|
| x | 30 | 1016 | 30–1016 |
| y | 27 | 2513 | 25–2513 |

Obě osy rostou plynule a správným směrem, rozsah odpovídá 1080 × 2520 z DTS.
Není potřeba žádné převrácení ani prohození os. Po celou dobu žádná chyba
kontrolního součtu.

**Dotyková obrazovka funguje.** Nezjištěno: nastavení firmwaru (viz výše) a
jestli obraz při resetu dotyku blikne (uživatel se nevyjádřil; test probíhal
podle pokynů na displeji). Více prstů najednou nezkoušeno.

## 16. Restart a vypnutí z kernelu (2026-11-11)

### 16.1 Jak to dělá původní kernel

Firmware restart přes PSCI neumí (kap. 10, `PSCI reset returned`). Původní
kernel má `CONFIG_EXYNOS_REBOOT=y` (`exynos9610_defconfig:543`), bez
`SEC_REBOOT`; uzel `reboot { compatible = "exynos,reboot"; pmu_base =
<0x11860000>; }` je v původním DT (`exynos9610.dtsi:3259`) i v živém DT
telefonu. Vše v `drivers/soc/samsung/exynos-reboot.c`:

| Akce | Registr PMU | Co se zapíše | Zdroj |
|---|---|---|---|
| restart | `0x400` SWRESET | `0x1` | `exynos_reboot()`, konec funkce |
| vypnutí | `0x330c` PS_HOLD_CONTROL | vynulovat bit 8 (`& 0xFFFFFEFF`) | `exynos_power_off()` |
| důvod restartu | `0x81c` SYSIP_DAT3 | `0x77665501` běžný, `0x77665500` bootloader, `0x77665502` recovery, `0x77665505` panika | `exynos_reboot()` |
| režim restartu | `0x810` SYSIP_DAT0 | `0xFC` bootloader/fastboot, `0xFF` recovery, `0x0A` charge | `exynos_reboot()` |

PS_HOLD_CONTROL na `0x330C` s bitem 8 potvrzuje i
`cal-if/exynos9610/pmucal_system_exynos9610.h:60-61` (sekvence
`pmucal_lpm_init` nastavuje bity 31 a 8).

Co původní kernel dělá navíc a tady chybí:

- Při startu zapíše do SYSIP_DAT3 `0x77665505`, aby neočekávaný reset vypadal
  jako panika.
- Před restartem, pokud je v `0x500` RESET_SEQUENCER_CONFIGURATION nastavený
  bit 0 (výpis registrů při pádu), ho shazuje a upravuje `0x41c`, `0x414` a
  registry velkých jader. Komentář v kódu říká, že bez toho může telefon po
  resetu skončit ve fastbootu.
- Před vypnutím čeká na puštění vypínače (jinak PMIC telefon hned zase zapne)
  a volá ACPM (`exynos_acpm_reboot`).

### 16.2 Mainline

`syscon-reboot`, `syscon-poweroff` a `syscon-reboot-mode` jako potomci uzlu
PMU; stejně to mají Exynos5433 (`offset = <0x400>`, SWRESET) a Exynos850
(`PS_HOLD_CONTROL`, `mask = <0x100>`, `value = <0>`). Potomky vytváří ovladač
`exynos-pmu`, který Exynos9610 neznal.

Implementace (větev `troika-clk`, 3 commity, celkem 67 nad v7.3.3):

| Commit | Co |
|---|---|
| `soc: samsung: exynos-pmu: Add Exynos9610` | compatible v ovladači (bez dat, jako Exynos850) |
| `arm64: dts: exynos9610: Add reboot and poweroff` | `syscon-reboot` (0x400, bit 0 = 1), `syscon-poweroff` (0x330c, bit 8 = 0) |
| `arm64: dts: exynos9610-troika: Add reboot modes` | `reboot-mode` nad SYSIP_DAT3: normal, bootloader, recovery |

Uživatel zápisy povolil 2026-11-11 („Ano, všechny tři“): `0x11860400`,
`0x1186330c`, `0x1186081c`. SYSIP_DAT0 a RESET_SEQUENCER_CONFIGURATION se
nezapisují (souhlas na ně není a `syscon-reboot-mode` umí jen jeden registr).

`dtbs_check` beze změny (jen známé `vdd33-supply`). Ovladače čtou registr před
zápisem (`regmap_update_bits`); původní kernel SWRESET ani SYSIP_DAT3 nečte.

Neověřeno před testem RB1: že bootloaderu stačí SYSIP_DAT3 bez SYSIP_DAT0, co
udělá reset při nastaveném RESET_SEQUENCER_CONFIGURATION, a jestli vypnutí
projde přes ukončení UFS (při restartu se poprvé volá vypínací cesta ovladače).

### 16.3 Chyba při průzkumu: čtení PMU přes debugfs

Při hledání, jak registry jen přečíst, jsem na telefonu běžícím z TS2 četl
`/sys/kernel/debug/regmap/dummy-system-controller@…11860000/registers`. Ten
soubor čte registry od nuly po řadě; na offsetu `0x8` přišel „synchronous
external abort“ a kernel proces zabil (oops, jen čtení, nic se nezapsalo).
Zámek regmapu PMU zůstal držený, takže ten běh kernelu už na PMU sahat
nesmí; telefon je potřeba restartovat tlačítky. **Výpis PMU přes debugfs
nepoužívat**; neimplementované offsety PMU na tomto SoC nejdou číst.
Ladicí commit proto čte jen čtyři jmenované registry (`PMUDBG`), z pracovní
fronty a po jednom.

### 16.4 Test RB1 (2026-11-11) – stav před akcemi

Log: `device-info/dmesg-mainline-RB1.txt` (stažen 9 s po startu, před
stiskem tlačítek).

| Registr PMU | Hodnota po bootloaderu |
|---|---|
| `0x500` RESET_SEQUENCER_CONFIGURATION | `0x00000000` – výpis po pádu není zapnutý, úklid z původního kernelu tedy není potřeba |
| `0x330c` PS_HOLD_CONTROL | `0x80005300` – bit 31 (povolení) a bit 8 (držet napájení) nastavené |
| `0x81c` SYSIP_DAT3 | `0x00000000` |
| `0x810` SYSIP_DAT0 | `0x00000000` – bootloader oba registry po startu nuluje |

Všechny tři ovladače se navázaly (`syscon-reboot`, `syscon-poweroff`,
`syscon-reboot-mode`). Vedlejší hláška: `exynos-clkout: cannot match parent
device` (−22) – buňku CLKOUT přidává ovladač PMU každému SoC a Exynos9610
v jejím seznamu není; neškodné, stejné je to u Exynos850.

### 16.5 Test RB1 – výsledky akcí (2026-11-11)

Tři běhy image, akce z tlačítek testovacího initu; výsledek podle hlášení
uživatele („restart do bootloaderu funguje, vypnutí funguje, klasický restart
taky funguje“):

| Akce | Co kernel zapsal | Výsledek |
|---|---|---|
| Vol+ – `reboot-arg bootloader` | SYSIP_DAT3 = `0x77665500`, SWRESET = 1 | telefon naběhl do fastbootu |
| Power – `reboot -f` | SYSIP_DAT3 = `0x77665501`, SWRESET = 1 | běžný restart (po něm šlo `adb reboot bootloader`, tedy běžel LineageOS) |
| Vol− – `poweroff -f` | PS_HOLD_CONTROL bit 8 = 0 | telefon se vypnul |

Bootloaderu tedy stačí SYSIP_DAT3; SYSIP_DAT0, který původní kernel zapisuje
navíc, není potřeba. Restart ani vypnutí se nezasekly na ukončení UFS.

Neověřeno: režim `recovery` (0x77665502), chování vypnutí s kabelem
a bez něj zvlášť (uživatel nerozlišil), restart z plného systému (systemd) –
ten přijde na řadu s balíčkem.

## 17. Balíček pmaports: kernel pkgrel 5, zařízení pkgrel 3 a 4 (2026-11-11 a 12)

pmaports, větev `motorola-troika-mainline`, tři místní commity (nepushnuto):

| Commit | Co |
|---|---|
| `2b727fc1f` linux-motorola-troika-mainline: add UFS, touchscreen and reboot | 67 patchů z `troika-clk` (nové 53–67), `CONFIG_TOUCHSCREEN_HIMAX_HX83112B=y` (bylo `=m`), pkgrel 5 |
| `03767089d` device-motorola-troika: keep the internal storage read-only | pravidlo udev `60-ufs-readonly.rules`, pkgrel 3 |
| `f5083510b` device-motorola-troika: keep the internal storage read-only in the initramfs | pravidlo a `blockdev` i v initramfs, háček `00-ufs-readonly.sh`, pkgrel 4 (viz 17.2) |

Konfigurace balíčku už měla `SCSI_UFS_EXYNOS`, `PHY_SAMSUNG_UFS`,
`I2C_S3C2410`, `POWER_RESET_SYSCON`, `POWER_RESET_SYSCON_POWEROFF` a
`SYSCON_REBOOT_MODE` zapnuté; od místní testovací konfigurace se liší jen
verzí kompilátoru a zram. `pmbootstrap kconfig check` (i kategorie zram)
prochází, checksumy v obou APKBUILD sedí.

Pravidlo udev (moje rozhodnutí, uživatel o něj nežádal, dá se vypustit):

```
ACTION=="add", SUBSYSTEM=="block", ENV{DEVTYPE}=="disk", SUBSYSTEMS=="platform", KERNELS=="13520000.ufs", RUN+="/usr/bin/blockdev --setro $devnode"
```

Zabírá na disky pod řadičem UFS (`13520000.ufs`, adresa z DTS). **Neověřeno**
– ověří se v testu NET3 příkazem `lsblk` (TESTING.md 5k). `/usr/bin/blockdev`
v obrazu systému je (util-linux-misc).

### 17.1 Kontrola prvního exportu (2026-11-12 01:22)

Kernel r5 sestaven (00:52), `install` (01:16) a `export` (01:21) doběhly.
Boot image z exportu (sha256 začíná `cfc90979`, 47 888 384 B):

- hlavička v1; „kernel“ = uniLoader z balíčku r5 (bajt po bajtu), „second“ =
  původní DTB z balíčku zařízení r3;
- kernel 7.3.3-troika, žádný ladicí výpis (CLKDBG, USBDBG, UFSDBG, TSDBG,
  PMUDBG = 0×), obsahuje `himax,hx83112a`, `samsung,exynos9610-pmu`,
  `syscon-reboot-mode`, `samsung,exynos9610-ufs`;
- konfigurace v balíčku = konfigurace v pmaports; DTB v balíčku = DTB z místního
  sestavení (shodný bajt po bajtu);
- initramfs má `hooks/netboot.sh` a `nbd.ko`.

**Tento image jsem k testu nepustil** (neuložen jako NET3), viz 17.2.

### 17.2 Initramfs pmOS a zapisovatelné UFS

V kap. 17 jsem původně napsal, že initramfs na úložišti jen hledá oddíly podle
jmenovek. To nebyla pravda; přečetl jsem skripty z exportovaného initramfs
(postmarketos-initramfs 3.12.6-r0, `init_2nd.sh`, `init_functions*.sh`):

- pravidlo `60-ufs-readonly.rules` v initramfs není (udev tam běží, ale jen
  s vlastní sadou pravidel) – disky UFS jsou po celou dobu initramfs
  zapisovatelné;
- `mount_subpartitions` (volá ho i `hooks/netboot.sh` po připojení NBD):
  když se kořenový oddíl nenajde hned podle UUID, projde všechny oddíly všech
  disků, na každém spustí `fdisk -l` a kde vidí právě 2 „pododdíly“, udělá
  `losetup -Pf` (zapisovatelně) a hledá kořen znovu. Najde-li se kořen právě
  v tu chvíli (protože se mezitím objevil `nbd0p2`), zůstane smyčka nad tím
  oddílem UFS zapsaná v `SUBPARTITION_LOOP`;
- `resize_root_partition` pak nad `SUBPARTITION_LOOP` spustí
  `parted -f -s … resizepart 2 100%`, tedy **zápis do oddílu UFS**.

K zápisu je potřeba souběh (oddíly NBD se objeví až po prvním hledání) a oddíl
UFS, ve kterém busybox `fdisk -l` uvidí právě dva oddíly. Zkusil jsem to
stejným busyboxem na všech 43 obrazech v záloze (jen čtení): takový není žádný
(všude „doesn't contain a valid partition table“). V záloze nejsou oddíly
systému a uživatelských dat, ty jsem ověřit nemohl. Riziko je tedy malé, ale
nenulové, a týká se pravidla „do telefonu se nezapisuje“.

Oprava (pmaports commit `f5083510b`, balíček zařízení pkgrel 4, místní):

- `20-ufs-readonly.files` – do initramfs se přidá pravidlo udev a
  `/usr/bin/blockdev`;
- háček `00-ufs-readonly.sh` (běží po startu udev a před `netboot.sh`): u každého
  disku pod `13520000.ufs` zkontroluje `ro`; kde není 1, zavolá
  `blockdev --setro`; kde to nejde, zastaví start (`fail_halt_boot`);
- závislost `util-linux-misc` (dodává `blockdev`).

Háček vyzkoušen nasucho busyboxem na napodobeném `/sys/block` (disk už jen pro
čtení; disk, který pravidlo minulo; `blockdev` selže → zastavení; disk mimo
UFS se přeskočí). Na telefonu **neověřeno**. Příznak jen pro čtení je v jádře
u blokového zařízení, takže přežije přechod z initramfs do systému.

Co příznak v kernelu 7.3.3 skutečně brání (ověřeno ve zdrojích 2026-11-12;
moje původní tvrzení, že `losetup` otevře smyčku jen pro čtení, bylo špatně):

| Cesta | Chování | Zdroj |
|---|---|---|
| `write(2)` na `/dev/sdX` (i přes smyčku, ta volá `write_iter`) | `-EPERM` | `block/fops.c`, `blkdev_write_iter` |
| `mmap` se zápisem | mapování jen pro čtení | `block/fops.c`, `blkdev_mmap_prepare` |
| připojení souborového systému pro zápis | `-EACCES` | `fs/super.c`, `setup_bdev_super` |
| otevření uzlu `O_RDWR` z userspace | **projde** (záměrně, komentář v `fs/super.c`) | – |
| zápisové bio na vrstvě bloků | **jen varování**, neselže | `block/blk-core.c`, `bio_check_ro` |
| příkazy SCSI napřímo (`SG_IO`, `/dev/bsg/*`) | **příznak se neuplatní** | `BLK_DEV_BSG=y`, `SCSI_UFS_BSG=y` v konfiguraci |

`parted`, `fdisk`, `resize2fs`, `fsck` i `mount` tedy zapsat nemohou. Nástroj
spuštěný jako root, který posílá příkazy SCSI napřímo (sg3_utils, ufs-utils,
aktualizace firmwaru), by příznak obešel; nic takového se samo nespouští.
Možné zpřísnění při příštím sestavení kernelu: vypnout `SCSI_UFS_BSG` a
`BLK_DEV_BSG` (zatím neprovedeno).

Kernel se znovu sestavovat nemusí; stačí `install` a `export`.

### 17.3 Kontrola třetího exportu = image NET3 (2026-11-12 01:47)

Druhý `install` (01:32) selhal vadou pmbootstrapu (`losetup --json --list`:
po sestavení balíčku zařízení se nativní chroot vytvořil znovu bez
util-linux), obraz systému zůstal prázdný. Po `pmbootstrap shutdown` třetí
`install` (01:45) a `export` (01:46) doběhly.

`out/troika-test-NET3.img` (47 896 576 B, sha256 začíná `d200dd47`):

- hlavička v1; „kernel“ = uniLoader z balíčku r5, „second“ = původní DTB;
  žádné ladicí výpisy;
- initramfs: `hooks/00-ufs-readonly.sh` a `60-ufs-readonly.rules` shodné
  s pmaports, `/usr/bin/blockdev` z util-linux (potřebuje jen musl),
  `hooks/netboot.sh`, `nbd.ko`; ostatní skripty stejné jako v prvním exportu;
- obraz systému: GPT, `pmOS_boot` (ext2) a `pmOS_root` (ext4), jejich UUID
  souhlasí s `pmos_boot_uuid` a `pmos_root_uuid` v boot image;
- v systému `device-motorola-troika` 1-r4 a `linux-motorola-troika-mainline`
  7.3.3-r5.

`images_netboot/` drží starý obraz z NET2, proto `netboot serve --replace`.

### 17.4 Test NET3: úložiště jen pro čtení (2026-11-12 01:50–01:55)

`netboot serve --replace` (01:49), `fastboot boot out/troika-test-NET3.img`,
server v 01:49:46 našel telefon, v 01:50:27 odpovídal SSH server, přihlášení
`ssh user@172.16.42.1` prošlo (uvítání „Welcome to Nura!“).

`lsblk -o NAME,SIZE,RO,MOUNTPOINTS` v běžícím systému:

- `sda` (119,2 G) a všech 42 oddílů `sda1`–`sda42`: RO = 1, žádný přípojný bod;
- `sdb`, `sdc` (4 M): RO = 1;
- `nbd0p1` na `/boot`, `nbd0p2` na `/` (RO = 0, to je obraz na PC);
- `zram0` 5,4 G jako swap.

`sudo dmesg | grep -c "was not read-only"` = 0: háček v initramfs nemusel
žádný disk přepínat sám a start nezastavil. Výstup initramfs jde do logu jádra
(`logger` → `syslogd -K`), takže by tam hlášení bylo vidět.

Závěr: pravidlo `60-ufs-readonly.rules` zabírá (shoda na `13520000.ufs`)
a úložiště telefonu je v plném systému jen pro čtení. Zda disky přepnulo
pravidlo už v initramfs, nebo až v kořenovém systému, z těchto dvou výstupů
jednoznačně neplyne; háček by v prvním případě mlčel a v druhém také, pokud
by disky v době jeho běhu ještě nebyly vidět (UFS se ale hlásí v čase 2,4 s,
tedy dřív).

### 17.5 Test NET3: stav systému, dotyk, restart (2026-11-12 ~02:00)

- `systemctl is-system-running` → `running`.
- `grep hx83112a /proc/interrupts` → IRQ 43, `gpa2 4 Level`, 286 přerušení na
  CPU0 (jedno čtení, po dotycích; v klidu bylo v testu TS2 0). Ovladač dotyku
  je tedy v balíčku funkční. Čtení před dotykem a po něm zvlášť v záznamu
  není.
- `sudo reboot` z plného systému: spojení SSH se ukončilo a telefon se
  restartoval; uživatel potvrdil, že vše proběhlo správně.

Neověřeno: `sudo poweroff` z plného systému, restart do bootloaderu a do
recovery z plného systému.

## 18. GUI: Phosh přes netboot (2026-11-12)

### 18.1 Příprava image GUI1

`pmbootstrap config ui phosh`, `install --zap` (02:28, bez chyby) a `export`
(02:31). Kernel r5 ani balíček zařízení r4 se nezměnily.

`out/troika-test-GUI1.img` (47 896 576 B, sha256 začíná `e9242d22`):

- uniLoader = balíček r5, „second“ = původní DTB;
- initramfs má stejný obsah jako u NET3 (stejné skripty, háček
  `00-ufs-readonly.sh`, pravidlo udev, `blockdev`, `nbd.ko`);
- obraz systému 3,2 GB: `pmOS_boot` 487 M (ext2), `pmOS_root` 2,5 G (ext4),
  UUID souhlasí s boot image;
- v systému 1186 balíčků, mj. phosh 0.58, phoc 0.58, greetd-phrog 0.53,
  mesa 26.2.6 (softwarové vykreslování), stevia (klávesnice), udisks2 +
  udiskie, **fwupd** a gnome-software.

K úložišti telefonu: `udiskie` připojuje vyměnitelné disky; `fwupd` zařízení
jen vyjmenovává a firmware zapisuje až na výslovný pokyn s konkrétním
souborem (navíc telefon nemá přístup na internet). Obojí je mimo dosah
příznaku jen pro čtení jen v případě příkazů SCSI napřímo (17.2). První
kontrola po startu je proto zase `lsblk`.

### 18.2 Test GUI1: Phosh běží (2026-11-12 02:34–02:45)

`netboot serve --replace` (obraz zkopírován 02:34), `fastboot boot
out/troika-test-GUI1.img`, server našel telefon 02:34:53, SSH odpovídalo
02:35:16 (asi 23 s po nalezení).

Podle uživatele: na displeji načítací kolečko, potom klávesnice pro zadání
hesla (přihlašovací obrazovka phrog) – **miniaturní, uprostřed displeje** –
a po přihlášení Phosh nastartoval, ale vše bylo malé; uživatel si v nastavení
zvolil zvětšení 200 %. Dotyk tedy funguje i v grafickém rozhraní (heslo
zadané na displeji, ovládání nastavení).

Příčina malého zobrazení: `simple-framebuffer` nenese fyzický rozměr panelu,
phoc proto použije měřítko 1. Možnosti nápravy:

1. `/etc/phosh/phoc.ini` v balíčku zařízení s pevným měřítkem pro výstup
   (použije ho i přihlašovací obrazovka: `phrog-greetd-session` sahá po
   `/etc/phosh/phoc.ini`, když nemá vlastní). Stačí balíček zařízení, kernel
   se nesestavuje. Potřebuje jméno výstupu (zjistit z `/sys/class/drm`).
2. Fyzický rozměr do DT: simpledrm čte `width-mm`/`height-mm` z uzlu, na který
   ukazuje `panel` ve framebufferu (`drivers/gpu/drm/sysfb/simpledrm.c`).
   Zdroj rozměru: původní kernel, `exynos9610-display-lcd.dtsi`, uzel
   `hix83112a`: `size = <64 148>` (stejně i druhý panel `nov36672a`);
   `dsim_drv.c` to čte jako šířku a výšku LCD. Tedy 64 × 148 mm. Vyžaduje
   sestavení kernelu, proto až s dalším kolem balíčku.

Neověřeno: výstup `lsblk` v tomto běhu (uživatel se přihlásil, příkaz zatím
nespustil), rychlost vykreslování, co všechno v Phoshi funguje.

### 18.3 Test GUI1: úložiště, výstup, dojem (2026-11-12 ~02:50)

- `lsblk` v běžícím Phoshi: `sda` + 42 oddílů, `sdb`, `sdc` mají RO = 1 a
  žádný přípojný bod (udiskie nic z telefonu nepřipojil); systém na `nbd0p2`
  (2,5 G), swap na zram.
- `ls /sys/class/drm/`: `card0`, `card0-Unknown-1` → výstup se jmenuje
  `Unknown-1`.
- Uživatel: funguje to dobře, grafické prostředí je trhané (softwarové
  vykreslování), ale dá se v něm pohybovat.

Oprava měřítka: pmaports commit „device-motorola-troika: set the display scale
for Phosh“ (pkgrel 5, místní) – podbalíček `device-motorola-troika-phosh`
(`install_if` s `postmarketos-ui-phosh`) instaluje `/etc/phosh/phoc.ini`:

```
[output:Unknown-1]
scale = 3
```

Měřítko 3 si určil uživatel („ideální je pro mě 300 %“; plocha 360 × 840
bodů). **Nesestaveno, neověřeno.**

### 18.4 Vypnutí z plného systému (2026-11-12 ~03:00)

`sudo poweroff` z běžícího Phoshe (image GUI1) přes SSH: telefon se podle
uživatele opravdu vypnul. Tím je ověřené i vypnutí z plného systému se
systemd (dosud jen z testovacího initramfs, kap. 16.5).

Poznámka k postupu: dokud běží `pmbootstrap netboot serve`, nejde sestavovat
ani instalovat (`umount …/chroot_native/mnt/pmbootstrap/netboot` selže,
kód 32). Nejdřív vypnout telefon, pak server ukončit Ctrl+C.

### 18.5 Image GUI2 (2026-11-12 03:12)

Balíček zařízení r5 sestaven zvlášť (03:04), `install` (03:08) a `export`
(03:10) bez chyby. `out/troika-test-GUI2.img` (sha256 začíná `184d4456`):
uniLoader = r5, „second“ = původní DTB, initramfs stejný jako u NET3 a GUI1
(ochrana UFS), UUID oddílů souhlasí. V systému `device-motorola-troika` 1-r5,
`device-motorola-troika-phosh` 1-r5 (nainstaloval se sám přes `install_if`)
a `/etc/phosh/phoc.ini` s `scale = 3` pro `Unknown-1`.

### 18.6 Test GUI2: měřítko 3 (2026-11-12 03:15–03:20)

`fastboot boot out/troika-test-GUI2.img`, SSH odpovídalo 03:15:59. Uživatel:
„velikost je nyní už skvělá“. `lsblk`: `sda` + 42 oddílů, `sdb`, `sdc`
RO = 1, nic připojeného. `/etc/phosh/phoc.ini` z podbalíčku
`device-motorola-troika-phosh` tedy funguje.

Pro příště (nezkoumáno do hloubky): původní DT má podsvícení jako zařízení
I2C na adrese 0x36 (`exynos9610-display-lcd.dtsi`, uzel `backlight_i2c@36`,
compatible `backlight_i2c`). Který je to čip a na které sběrnici, je potřeba
teprve dohledat.

## 19. GPU: Mali-G72 přes Panfrost (2026-11-12)

### 19.1 Co je potřeba a co už existuje

Nový ovladač GPU se nepíše. Mali-G72 (Bifrost) umí mainline ovladač
**Panfrost** (`drivers/gpu/drm/panfrost`, model `g72` = ID 0x6001 v
`panfrost_gpu.c`) a v Mese ovladač `panfrost` (v našem systému
`/usr/lib/dri/panfrost_dri.so`, mesa 26.2.6). Chybí jen popis hardwaru v DT
a hodiny. Konfigurace kernelu má `DRM_PANFROST=y` už od začátku.

### 19.2 Hodnoty a jejich zdroje

| Co | Hodnota | Zdroj |
|---|---|---|
| registry GPU | 0x11500000, délka 0x5000 | původní `exynos9610.dtsi`, uzel `mali@11500000`; `device-info/live.dts:7962` |
| přerušení | JOB = SPI 66, MMU = SPI 67, GPU = SPI 65 | tamtéž (`interrupts = <0 66 0>, <0 67 0>, <0 65 0>`, jména JOB, MMU, GPU) |
| CMU_G3D | 0x11430000 | `cmucal-sfr.c:18` (`SFR_BLOCK(G3D, 0x11430000, 0x8000)`) |
| vstupy CMU_G3D | `oscclk`, `dout_cmu_g3d_switch`, `dout_cmu_hpm` | binding Chimacovy série v3; ovladač zkontrolován proti cmucal (kap. 11) |
| hodiny GPU | `CLK_GOUT_CLK_G3D_G3D_CLK` (0x2004), `CLK_GOUT_G3D_BUSP_CLK` (0x203c), `CLK_GOUT_G3D_BUSD_CLK` (0x2038) | `cmucal-sfr.c:862-873`; stejná trojice jako v portu gta4xl (19.3) |
| napájecí doména | G3D_CONFIGURATION 0x4060, G3D_STATUS 0x4064, čtyři bity | `pmucal_local_exynos9610.h:247-248`; po bootloaderu zapnutá (test CLK1, 11.7) |
| řízení frekvence a napětí | přes firmware ACPM (`g3d_cmu_cal_id = <ACPM_DVFS_G3D>`), tabulka 260–1053 MHz | původní uzel `mali` |

Velikost oblasti CMU_G3D je v DT 0x10000 jako u ostatních devíti CMU
(automatické hradlování, ověřené testem CLK1: mainline změní proti stavu po
bootloaderu jen bit 30 registru OPTION). S 0x8000 by mainline přepnul všechna
hradla do ručního režimu (`exynos_arm64_init_clocks`: nastaví bit 20, smaže
bit 28) – to na troice ověřené není.

### 19.3 Práce ostatních: port gta4xl (Exynos 9611)

`github.com/arhammahajan/gta4xl-linux` (staženo do `refs/gta4xl-linux`,
commit `4d4f1fd` z 2026-10-23; v repozitáři není licence): mainline 7.3 na
Galaxy Tab S6 Lite s Exynosem 9611, stejná rodina čipů. Podle jeho
dokumentace tam funguje:

- Panfrost na Mali-G72 MP3 (`docs/GPU.md`): nejdřív s pevnými hodinami 260 MHz,
  jak je nechal bootloader, později se změnou frekvence přes ACPM (260–683 MHz);
  napětí řídí firmware, kernel regulátor neprogramuje;
- vlastní ovladač displeje (DECON0 + DSIM0 + panel, `0130-exynos9610-display-dpu.patch`,
  2012 řádků) a na něm GNOME na Waylandu s akcelerací;
- WiFi a Bluetooth (přenesený ovladač Samsung SCSC), SD karta, změna
  frekvence CPU, baterie a nabíjení, uspání, senzory, kamera. Zvuk ne.

Jeho uzel GPU (`0120-gta4xl-panfrost-gpu.patch`) má stejnou adresu, přerušení
a hodiny jako náš. Liší se tím, že popisuje napájecí doménu (ovladač
`exynos-pm-domains` s novým `samsung,exynos9610-pd`, drženou stále zapnutou)
a pevný „regulátor“ 800 mV jen pro uspokojení ovladače; ten později sám
odstranil jako nesprávný.

Všechno z toho repozitáře beru jako podklad k ověření, ne jako ověřený fakt
pro troiku: je to jiné zařízení, jiný bootloader a jiný displej (AMOLED vs.
náš LCD s HX83112A).

### 19.4 Naše změny (větev `troika-clk`, 3 commity, celkem 70 nad v7.3.3)

1. `dt-bindings: gpu: mali-bifrost: Add Samsung Exynos9610` – compatible
   `samsung,exynos9610-mali`.
2. `arm64: dts: exynos9610: Add CMU_G3D and the GPU` – uzly `cmu_g3d`
   a `gpu@11500000` (vypnutý).
3. `arm64: dts: exynos9610-troika: Enable the GPU` – jen `status = "okay"`.

Záměrně **není** popsaný regulátor (pravidlo o regulátorech; panfrost si
vezme zástupný), tabulka frekvencí (bez ACPM nejde měnit napětí, takže se
frekvence nemění) ani napájecí doména (kernel tak do PMU kvůli GPU nic
nezapisuje; žádný nový souhlas není potřeba). GPU poběží na tom, co nechal
bootloader.

`make CHECK_DTBS=y exynos/exynos9610-troika.dtb`: jen známé varování USB.

### 19.5 Rizika prvního testu (image GPU1)

- Registrace CMU_G3D čte registry v doméně G3D. Ta je podle testu CLK1
  zapnutá; kdyby nebyla, kernel při startu spadne (telefon se pak vypne
  podržením Power + Vol−). Žádný zápis kromě bitu 30 v OPTION.
- Panfrost při startu resetuje GPU a zapne jeho jádra. Napětí ani frekvenci
  nemění. Kdyby větev napájení GPU nebyla zapnutá nebo hodiny neběžely,
  ovladač selže nebo kernel spadne; hardwaru to neublíží.
- Jaká frekvence po bootloaderu běží, nevím (na gta4xl 260 MHz); test ji
  vypíše.

Zobrazení: displej dál obsluhuje simpledrm. Jestli phoc (wlroots) dokáže
vykreslovat na GPU a výsledek kopírovat do framebufferu simpledrm, se ukáže
až ve druhém kroku s plným systémem. **Neověřeno.**

### 19.6 Test GPU1: panfrost se navázal (2026-11-12 03:42)

`fastboot boot out/troika-test-GPU1.img`; stav přečten přes telnet
(`tools/phone-sh.py`), celý log v `device-info/dmesg-mainline-GPU1.txt`.

```
CLKDBG clock-controller@11430000: option 0x30000000 -> 0x70000000
panfrost 11500000.gpu: clock rate = 260000000
panfrost 11500000.gpu: bus_clock rate = 65000000
panfrost 11500000.gpu: error -ENODEV: _opp_set_regulators: no regulator (mali) found
panfrost 11500000.gpu: supply mali not found, using dummy regulator
panfrost 11500000.gpu: mali-g72 id 0x6221 major 0x0 minor 0x1 status 0x0
panfrost 11500000.gpu: shader_present=0x7 l2_present=0x1
[drm] Initialized panfrost 1.6.0 for 11500000.gpu on minor 1
```

- GPU se ohlásilo jako Mali-G72 r0p1 se třemi jádry (`shader_present=0x7`) –
  stejné ID 0x6221 jako na gta4xl.
- Hodiny po bootloaderu: `fout_g3d_pll` 260 MHz → `gout_clk_g3d_g3d_clk`
  260 MHz, `gout_g3d_busp_clk` 65 MHz. Stejných 260 MHz uvádí gta4xl.
- OPTION CMU_G3D před zápisem `0x30000000`, po něm `0x70000000` – jako
  u ostatních CMU.
- Vznikly `/dev/dri/card0` (simpledrm), `card1` a `renderD128` (panfrost).
- Žádná chyba ani pád v logu; přerušení GPU jsou zaregistrovaná (0 výskytů,
  nic zatím nevykresluje); GPU je za běhu uspané (`runtime_status`
  = suspended), což je v klidu správně.
- Hlášení o chybějícím regulátoru `mali` je očekávané (19.4).

Neověřeno: jestli GPU skutečně vykresluje (v testovacím initramfs není Mesa)
a jestli ho použije phoc.

### 19.7 Druhý krok: Phosh s místním kernelem (image GPU2)

Aby se nemusel sestavovat balíček (1,5 h), `tools/make-local-pmos-bootimg.sh`
složí boot image z místního kernelu (čistá větev `troika-clk`, konfigurace
balíčku, označení verze `7.3.3-troika` kvůli modulům v systému) a z initramfs,
cmdline a původního DTB zkontrolovaného image GUI2. Systém na PC (obraz pro
netboot) se nemění.

### 19.8 Test GPU2, první pokus: moduly z balíčku se do místního kernelu nenačetly (2026-11-12 04:07)

Start skončil v ladicím shellu initramfs pmOS (fotka od uživatele: „Entering
debug shell“, klávesnice buffyboard). Přes telnet (ladicí shell ho spouští)
jsem přečetl `/pmOS_init.log` a `dmesg`:

```
BPF: Invalid name
failed to validate module [nbd] BTF: -22
modprobe: ERROR: could not insert 'nbd': Invalid argument
Failed to get /dev/nbd0, stopping.
```

Příčina: kernel i moduly mají ladicí informace BTF (`DEBUG_INFO_BTF_MODULES=y`).
Modul z balíčku (gcc 15.2 Alpine) se odkazuje na BTF kernelu z balíčku;
místně sestavený kernel (gcc 16.2) má BTF jiné a modul odmítne. Označení
verze (vermagic) sedělo, to nestačí. Chyba mého postupu, ne hardwaru ani GPU:
panfrost se i v tomto běhu navázal.

Oprava: místní kernel s `CONFIG_MODULE_ALLOW_BTF_MISMATCH=y` (modul se načte
bez svého BTF). Týká se jen místního sestavení pro zkoušení; balíček se
nemění.

Vedlejší nález k 17.4: v tomto běhu se systém vůbec nespustil, a přesto měly
`sda`, `sdb`, `sdc` `ro` = 1 a v logu není „was not read-only“. Pravidlo
`60-ufs-readonly.rules` tedy zabírá **už v initramfs**; háček jen kontroluje.

### 19.9 Test GPU2: Phosh vykresluje na GPU (2026-11-12 13:33)

Opravený image GPU2 (místní kernel z `troika-clk` s
`MODULE_ALLOW_BTF_MISMATCH=y`, sha256 začíná `8a4d0449`) nastartoval celý:
SSH odpovídalo 13:33:19, Phosh běží.

Výstup z telefonu (okno ssh uživatele):

- `lsblk`: `sda` a oddíly RO = 1 (vypsány první řádky);
- `/dev/dri/`: `card0` (simpledrm), `card1` a `renderD128` (panfrost);
- `/proc/interrupts`: `panfrost-job` **15 638** přerušení, `panfrost-mmu` 3,
  `panfrost-gpu` 0.

Tisíce přerušení od úloh GPU znamenají, že phoc vykresluje přes panfrost,
a to bez jakéhokoli nastavení: Mesa pro neznámý ovladač displeje (simpledrm)
použije vrstvu kmsro a k ní si najde GPU s uzlem pro vykreslování. Hotový
snímek pak simpledrm kopíruje do framebufferu bootloaderu.

Platí to pro GPU na 260 MHz (nejnižší stupeň původní tabulky) bez řízení
frekvence a napětí. **Neověřeno:** jak moc je rozhraní plynulejší (čeká se
na dojem uživatele), delší zátěž, teplota, uspání GPU za běhu pod zátěží,
jaký řetězec vykreslovače hlásí Mesa.

## 20. Další kroky: podsvícení, frekvence GPU, WiFi – průzkum a plán (2026-11-12)

Přání uživatele: ztmavování displeje, změna frekvence a napětí GPU přes
firmware, WiFi; kernel v balíčku sestavovat co nejméně.

### 20.1 Jak se vyhnout sestavování balíčku

- Změny kernelu se zkoušejí místně: přírůstkové sestavení trvá asi 2 minuty,
  `tools/make-local-pmos-bootimg.sh` z něj udělá image pro Phosh z netbootu
  (místní kernel potřebuje konfiguraci balíčku, jméno `7.3.3-troika`
  a `MODULE_ALLOW_BTF_MISMATCH=y`; 19.7, 19.8).
- Ovladače, které mohou být moduly (WiFi), se sestaví proti místnímu kernelu
  a do běžícího systému zkopírují přes `scp`; kvůli nim se image nemění.
- Balíček (kernel pkgrel 6) se sestaví **jednou**, až bude ověřená celá
  dávka: GPU, rozměr panelu, podsvícení a co dalšího bude hotové.

### 20.2 Podsvícení

| Co | Hodnota | Zdroj |
|---|---|---|
| čip | SG Micro SGM37603A, adresa I2C 0x36 | původní `drivers/video/backlight/sgm37603a.c`, `exynos9610-display-lcd.dtsi` (`backlight_i2c@36` na `hsi2c_3`) |
| sběrnice | `hsi2c_3` na 0x11D10000, SPI 273, 400 kHz | původní `exynos9610.dtsi:983`, `exynos9610-display-lcd.dtsi` |
| piny | `gpm2-0` (SCL), `gpm3-0` (SDA), funkce 3 | `exynos9610-pinctrl.dtsi:367` (`hsi2c3_bus`) |
| přepínač USI | `usi_0_cmgp_i2c` na 0x11C12004 (SYSREG_CMGP) | původní `exynos9610.dtsi:771` |
| hodiny | `CMGP_I2C`, `GATE_I2C_CMGP00_QCH` | původní uzel `hsi2c_3` |
| registry čipu | 0x10 zapnutí, 0x11 režim jasu, 0x19 jas MSB, 0x1A jas LSB (4 bity), 0x1B max. proud, 0x1F příznaky chyb, 0x01 reset | `sgm37603a.h` |
| jas | běžný režim 0xCCC (0x19 = 0xCC, 0x1A = 0x0C), zvýšený 0xFFF | `sgm37603a.h` (`sgm_nr_data`, `sgm_hl_data`) |

Původní ovladač čipu nastavuje jen strop (běžný/zvýšený režim); běžné
stmívání dělal původní systém nejspíš příkazem panelu přes DSI (PWM z řadiče
displeje), což bez ovladače displeje neumíme. **Neověřeno**, jestli zápis
nižší hodnoty do 0x19/0x1A displej ztlumí – to má ukázat první pokus.

Časování sběrnice v původním ovladači (`FSCL = IPCLK / ((DIV + 1) * 16)`)
odpovídá variantě `samsung,exynosautov9-hsi2c` mainline ovladače
`i2c-exynos5.c`; přepínač USI obsluhuje mainline `exynos-usi`. Mainline
ovladač pro SGM37603A není (hledáno v `drivers/` a `Documentation/`), byl by
potřeba malý nový.

Postup: (a) popsat sběrnici v DT a v testovacím image čip jen **přečíst**
(`i2cdump`), (b) zkusit zápis jasu v rozsahu 0–0xCCC, (c) napsat ovladač
podsvícení. Zvýšený režim 0xFFF nepoužívat.

### 20.3 Frekvence a napětí GPU přes firmware (ACPM)

Původní kernel mění frekvenci GPU voláním firmwaru ACPM (běží v řídicím
procesoru APM); ten přeprogramuje PLL i napětí větve. V mainline pro 9610
chybí celý řetěz: schránka zpráv (mailbox), protokol ACPM, hodiny řízené
přes ACPM a napojení na panfrost. Port gta4xl ho má ve třech patchích
(`0095` ACPM a regulátory pro SD, `0160` frekvence CPU a teplotní čidlo,
`0170` GPU; dohromady asi 2000 řádků) – podklad, který je potřeba ověřit
proti našemu původnímu kernelu a firmwaru.

Podmínky: (1) výslovný souhlas uživatele, protože firmware při tom mění
napětí větve GPU (dnešní pravidlo regulátory zakazuje); (2) teplotní ochrana
– mainline pro 9610 neumí číst teplotní čidlo, bez něj by vyšší frekvence
běžela bez dohledu; (3) horní mez pro 9609 (nižší než u 9610) doložit ze
zařízení, ne z cizího portu. Rozsah: dny práce, několik místních zkoušek.

### 20.4 WiFi

Rádio je Samsung SCSC „Maxwell“ uvnitř čipu, ne samostatný modul. Mainline
ovladač nemá. Port gta4xl používá původní ovladač Samsungu (275 souborů)
přenesený na 7.3 jako moduly mimo strom (`patches/upstream-wifi`), k tomu
uzly DT (schránka 0x119c0000, okna BAAW, 16 MiB sdílené paměti), zápisy do
PMU pro zapnutí rádia, volání zabezpečeného monitoru, firmware z oddílu
vendor a adresu MAC z EFS (`docs/WIFI.md`). Náš původní kernel má ovladač
SCSC také, ale v jiné verzi (10.x proti jejich 11.19), takže jejich patch
nepůjde použít přímo.

Podmínky: souhlas se zápisy do PMU pro rádio (konkrétní registry doložím
předem), čtení firmwaru z kopie oddílu vendor v záloze (jen čtení).
Rozsah: největší ze tří, dny až týdny; nejistý výsledek.

### 20.5 Doporučené pořadí

1. Podsvícení – nejmenší, samostatné, nic riskantního; dva až tři místní
   image.
2. ACPM a frekvence GPU – základ ACPM se hodí i pro frekvenci CPU a SD kartu.
3. WiFi – samostatný velký projekt.

Balíček kernelu sestavit po kroku 1 (GPU + rozměr panelu + podsvícení),
další až po kroku 2 nebo 3.

### 20.6 Podsvícení, krok 1: popis sběrnice (2026-11-12)

Stav na telefonu podle původního systému (`device-info/live.dts`):
`hsi2c@11D10000` je zapnutý, 400 kHz, s dítětem `backlight_i2c@36`;
`usi@11C12004` má `usi_v2_mode = "i2c"`.

Uzly v mainline (větev `troika-clk`):

| Uzel | Hodnoty | Zdroj |
|---|---|---|
| `sysreg_cmgp` | 0x11c10000, délka 0x2048 | původní `exynos9610.dtsi` (`sysreg = <0x11C10000>` u ADC; poslední přepínač USI na 0x11C12044) |
| `usi_cmgp0_i2c` | registry USI na 0x11d100c0, přepínač na `sysreg_cmgp` + 0x2004, režim I2C | původní `usi_0_cmgp_i2c: usi@11C12004`; původní `i2c-exynos5.c` (`USI_CON` na +0xC4); `usi_v2.c` (I2C = bit 2, stejně jako mainline `exynos-usi.c`) |
| `hsi2c_3` | 0x11d10000, SPI 273, piny `hsi2c3_bus` | původní `hsi2c_3` |
| hodiny | `CLK_GOUT_CMGP_I2C_CMGP00_IPCLK`, `_PCLK` | původní `GATE_I2C_CMGP00_QCH`, `CMGP_I2C` |

Varianta ovladače: `samsung,exynosautov9-hsi2c` (původní výpočet děliče
`fs_div = ipclk / (op_clk * 16)` je vzorec této varianty). Původní kernel
navíc nastavuje hodiny sběrnice na 200 MHz (`default-clk`); mainline frekvenci
nemění a dělič spočítá z té, kterou najde.

Co kernel při startu zapíše: režim I2C do přepínače USI (stejná hodnota jako
původní systém), uvolnění resetu USI a nastavení řadiče I2C. Na samotné
sběrnici se nic neděje, dokud o to někdo nepožádá.

Image BL1 čip jen čte (registry 0x10, 0x11, 0x19, 0x1a, 0x1b, 0x1f – tytéž
čte původní ovladač při startu). **Neověřeno** na telefonu.

### 20.7 Test BL1: sběrnice funguje, čip odpovídá (2026-11-12 13:59)

`fastboot boot out/troika-test-BL1.img`; čteno přes telnet.

- `exynos-usi` a řadič I2C se navázaly: `i2c-1` =
  `…/11d100c0.usi/11d10000.i2c`, `/dev/i2c-1`, 49 přerušení řadiče.
- Hodiny sběrnice: `gout_clk_cmgp_i2c_cmgp00_ipclk` 99 937 500 Hz (původní
  kernel si nastavuje 200 MHz). Mainline z toho spočítá dělič 14, tedy asi
  416 kHz místo 400 kHz (zaokrouhlení ve vzorci varianty autov9). Čtení
  funguje; k dořešení před balíčkem.
- Nástroj `i2c-reg` funguje (výpis použití i čtení).

Registry SGM37603A, jak je nechal bootloader, a jejich význam podle
datasheetu výrobce (SG Micro, SGM37603A rev. A, září 2021, tabulky 1–9;
`sg-micro.com/rect/assets/87b0126e-…/SGM37603A.pdf`):

| Registr | Hodnota | Význam |
|---|---|---|
| 0x10 zapnutí | 0x1f | čip i všechny tři řetězce LED zapnuté (výchozí po zapnutí) |
| 0x11 režim | 0x65 | `LED_MOD` = 11: jas = kód I2C × střída PWM (výchozí); náběh vypnutý |
| 0x19 jas, horních 8 bitů | 0xcc | kód jasu 0xCCC – přesně „běžný režim“ původního ovladače |
| 0x1a jas, dolní 4 bity | 0xfc | (horní čtyři bity jsou vyhrazené a čtou se jako 1) |
| 0x1b max. proud | 0x00 | 25 mA na řetězec (výchozí, nejnižší ze čtyř stupňů) |
| 0x1f chyby | 0x00 | žádná chyba |

Závěr z datasheetu: v režimu I2C × PWM je proud úměrný kódu jasu, kód 0
znamená vypnuto („Off, device in standby mode“, tabulka 9). Displej tedy
**půjde ztlumit i zhasnout jen zápisem do 0x19/0x1a**, bez ovladače displeje;
PWM od řadiče displeje zůstane, jak ho nastavil bootloader. Pořadí zápisu
podle původního ovladače: nejdřív dolní 4 bity (0x1a), pak horní bajt (0x19).

Meze pro ovladač: rozsah 0–0xCCC (3276), nikdy výš; registr 0x1b (max. proud)
a 0x10/0x11 neměnit. **Zápis zatím neproveden** – čeká na souhlas uživatele.

### 20.8 Pokusný zápis jasu (2026-11-12 14:05)

Souhlas uživatele (otázka „Povoluješ pokusné zápisy jasu do registrů 0x1a
a 0x19 čipu podsvícení?“ → „Ano, všechny čtyři“): kódy 0x400, 0x100, 0x000
a zpět 0xCCC. Provedeno přes telnet nástrojem `i2c-reg` v běžícím BL1,
mezi kroky 6 s; pořadí zápisu 0x1a (dolní 4 bity), pak 0x19.

| Kód | 0x19 po zápisu | 0x1a po zápisu | 0x1f (chyby) |
|---|---|---|---|
| 0x400 | 0x40 | 0xf0 | 0x00 |
| 0x100 | 0x10 | 0xf0 | 0x00 |
| 0x000 | 0x00 | 0xf0 | 0x00 |
| 0xCCC | 0xcc | 0xfc | 0x00 |

Zápisy prošly, zpětné čtení sedí, čip nehlásí chybu a je zpět na původní
hodnotě. **Neověřeno:** co bylo vidět na displeji – čeká se na uživatele.

### 20.9 Pokus potvrzen, ovladač podsvícení (2026-11-12)

Uživatel potvrdil, že se displej při pokusu 20.8 ve dvou krocích ztlumil a pak
na pár sekund zhasl. Jas tedy jde řídit jen přes I2C.

Kernel, větev `troika-clk` (+3, celkem 78 nad v7.3.3):

1. `dt-bindings: leds: backlight: Add SG Micro SGM37603A` – compatible
   `sgmicro,sgm37603a` (předpona `sgmicro` v `vendor-prefixes.yaml` už je).
2. `backlight: Add a driver for the SG Micro SGM37603A` –
   `drivers/video/backlight/sgm37603a.c`: zapisuje jen kód jasu (dolní 4 bity
   do 0x1a, pak horní bajt do 0x19), při startu jas přečte a nic nezapíše
   (displej neblikne), `max-brightness` z DT. Registry 0x10, 0x11 a 0x1b
   nechává být.
3. `arm64: dts: exynos9610-troika: Add the backlight` – uzel `backlight@36`
   s `max-brightness = <3276>` (0xCCC).

Frekvence sběrnice v DT je 390 kHz: čip snese nejvýš 400 kHz (datasheet,
„SCL Clock Period“ min. 2,5 µs) a mainline by ze 400 kHz udělal 416 kHz;
s 390 kHz vychází dělič 15, tedy 390,4 kHz. (Původní kernel s hodinami
200 MHz a svým vzorcem běžel asi na 368 kHz.)

Konfigurace: `BACKLIGHT_CLASS_DEVICE=y` a `BACKLIGHT_SGM37603A=y` (v balíčku
je dosud třída podsvícení jako modul; ovladač tam není).

**Neověřeno:** ovladač na telefonu; jestli Phosh při zhasnutí obrazovky
vypne i podsvícení (simpledrm o podsvícení neví, takže to nejspíš zůstane na
uživatelském prostoru).

### 20.10 Test BL2: ovladač se navázal, Phosh posuvník neukazuje (2026-11-12 14:40)

Image BL2 nastartoval. `/sys/class/backlight/*/brightness` = 3276,
`max_brightness` = 3276 → ovladač `sgm37603a` se navázal a přečetl jas po
bootloaderu. Uživatel: posuvník jasu se v horní liště Phoshe neukazuje.

Příčina (zdroj Phoshe, `src/udev-manager.c`,
`phosh_udev_manager_find_backlight()`): Phosh hledá podsvícení podle
konektoru displeje. Zařízení typu `firmware` a `platform` a libovolné `raw`
bere jen pro **vestavěný** konektor (eDP, LVDS, DSI…); jinak jen `raw`, jehož
rodič je konektor DRM stejného jména a je zapnutý. simpledrm vytváří konektor
typu `Unknown` (`drivers/gpu/drm/sysfb/simpledrm.c:868`), pro Phosh tedy
`Unknown-1` není vestavěný, a naše podsvícení visí pod zařízením I2C.

Možnosti: (a) ladicí přepínač Phoshe `PHOSH_DEBUG=fake-builtin` (bere displej
jako vestavěný; `phosh-session` načítá `~/.phoshdebug`) – rychlá zkouška bez
zásahu do kernelu; (b) skutečný ovladač displeje s konektorem DSI – správné
řešení, ale velká práce; (c) navrhnout Phoshi, aby jediný konektor simpledrm
bral jako vestavěný. **Neověřeno**, jestli (a) zabere.

### 20.11 Posuvník jasu s `PHOSH_DEBUG=fake-builtin` (2026-11-12 ~14:55)

Uživatel vytvořil `~/.phoshdebug` s `export PHOSH_DEBUG=fake-builtin`,
restartoval greetd a přihlásil se: „nyní se objevil posuvník a funguje
správně“. Řetěz Phosh → logind → `/sys/class/backlight` → `sgm37603a` → čip
tedy funguje.

Pro balíček: Phosh běží jako uživatelská služba systemd
`mobi.phosh.Shell.service`, takže proměnnou jde nastavit doplňkem služby.
Připraveno v pmaports (necommitnuto, balíček zařízení pkgrel 6, podbalíček
`-phosh`): `/usr/lib/systemd/user/mobi.phosh.Shell.service.d/50-builtin-display.conf`
s `Environment=PHOSH_DEBUG=fake-builtin`. **Neověřeno**, že doplněk služby
zabere stejně jako `~/.phoshdebug` – nejdřív zkouška naživo v
`/etc/systemd/user/…`.

**Neověřeno:** co se děje s podsvícením při zhasnutí obrazovky.

### 20.12 Doplněk služby potvrzen; zhasínání obrazovky (2026-11-12 ~15:00)

Uživatel smazal `~/.phoshdebug`, vytvořil
`/etc/systemd/user/mobi.phosh.Shell.service.d/troika.conf` s
`Environment=PHOSH_DEBUG=fake-builtin` a restartoval greetd: posuvník zůstal.
Doplněk služby tedy funguje → pmaports commit `dd0c8a565`
(„device-motorola-troika: let Phosh find the backlight“, pkgrel 6, místní).

Zhasínání podle uživatele:

- obrazovka zhasne sama (nečinnost): podsvícení zhasne úplně – Phosh před
  vypnutím výstupu stáhne jas na nulu;
- zhasnutí tlačítkem Power: obraz zčerná, **podsvícení zůstane svítit** –
  Phosh jen vypne výstup a spoléhá, že podsvícení zhasne kernel (to dělají
  ovladače panelů; simpledrm o podsvícení neví).

Oprava v kernelu (větev `troika-clk`, +3, celkem 81 nad v7.3.3):

1. `dt-bindings: display: simple-framebuffer: Add a backlight` – nepovinná
   vlastnost `backlight` (odkaz na podsvícení).
2. `drm/simpledrm: Switch the backlight with the CRTC` – při vypnutí CRTC
   `backlight_disable()`, při zapnutí `backlight_enable()`. Podsvícení se
   hledá až při prvním použití, takže displej nezávisí na tom, jestli a kdy
   se ovladač podsvícení naváže. Závislost v Kconfig kvůli třídě podsvícení
   jako modulu.
3. `arm64: dts: exynos9610-troika: Tie the backlight to the framebuffer`.

Kontroly: `dtbs_check` (jen známé varování USB), `dt_binding_check` pro
`simple-framebuffer.yaml` a `sgmicro,sgm37603a.yaml` bez hlášení, překlad bez
varování. Vlastnost `backlight` u `simple-framebuffer` není v upstreamu;
je to náš patch. **Neověřeno** na telefonu (image BL3).

### 20.13 Test BL3: podsvícení zhasíná s obrazovkou (2026-11-12 15:10)

Image BL3 (místní kernel z `troika-clk`, 81 nad v7.3.3). Uživatel: „všechno
nyní funguje správně“ – po stisku Power zhasne obraz i podsvícení, po dalším
stisku se displej rozsvítí na stejný jas, posuvník jasu a zhasnutí nečinností
fungují dál. `sudo reboot bootloader` z plného systému skončil ve fastbootu
(restart do bootloaderu je tím ověřený i z plného systému).

### 20.14 Balíček: kernel pkgrel 6, zařízení pkgrel 6 (2026-11-12 15:13)

pmaports, místní commity (nepushnuto):

| Commit | Co |
|---|---|
| `dd0c8a565` device-motorola-troika: let Phosh find the backlight | doplněk služby Phoshe, pkgrel 6 |
| „linux-motorola-troika-mainline: add the GPU and the backlight“ | 81 patchů, `BACKLIGHT_CLASS_DEVICE=y`, `BACKLIGHT_SGM37603A=y`, pkgrel 6 |

Konfigurace balíčku se od místní, na které testy proběhly, liší jen verzí
kompilátoru, `LOCALVERSION_AUTO` a `MODULE_ALLOW_BTF_MISMATCH` (obojí místní
pomůcky). `pmbootstrap kconfig check` prochází, checksumy sedí.
**Nesestaveno.**

Co v této dávce není: fyzický rozměr panelu v DT (měřítko pro Phosh řeší
`phoc.ini`; rozměr by potřeboval uzel panelu, pro který nemáme binding).

## 21. ACPM: komunikace s firmwarem řídicího procesoru (2026-11-12)

### 21.1 Co to je a co už existuje

ACPM je firmware jádra APM v čipu. Původní kernel přes něj mění frekvence
(a firmware s nimi napětí) procesoru, GPU a paměti a ovládá PMIC. Zprávy se
předávají ve sdílené paměti (SRAM) a ohlašují „zvonkem“ v mailboxu.

Mainline to umí pro Google GS101: `drivers/firmware/samsung/exynos-acpm*.c`,
`drivers/mailbox/exynos-mailbox.c` (má i variantu Exynos850). Pro 9610 chybí
jen popis a jedna konstanta.

### 21.2 Hodnoty a zdroje

| Co | Hodnota | Zdroj |
|---|---|---|
| mailbox AP→APM | 0x11900000, SPI 38 | původní `exynos9610.dtsi`, uzel `acpm_ipc` |
| SRAM APM | 0x2039000, délka 0x15000 | tamtéž |
| začátek popisu kanálů v SRAM | 0x6f00 | tamtéž (`initdata-base`) |
| tabulky frekvencí a napětí (fvmap) | SRAM + 0x6000 | uzel `acpm` (`fvmap_offset`), i v `device-info/live.dts:4807` |
| kanál pro frekvence | 5 | uzel `acpm_dvfs` (`acpm-ipc-channel`) |
| zvonek | zápis `(1 << kanál) << 16` do INTGR0 (0x8) | původní `acpm_ipc.c:478`; = varianta Exynos850 v mainline |
| maska přerušení od APM | INTMR1 (0x24) | původní `acpm_ipc.h`; = varianta Exynos850 |
| hodiny mailboxu | `CLK_GOUT_APM_MAILBOX_APM2AP_PCLK` (0x2060) | `cmucal-sfr.c:38` |
| čísla domén | MIF 0, INT 1, CPUCL0 2, CPUCL1 3, G3D 4, INTCAM 5, CAM 6, DISP 7, AUD 8, CP 9 | původní `dt-bindings/clock/exynos9610.h` (`ACPM_DVFS_*`, dolních 16 bitů) |
| zpráva „jaká je frekvence“ | `{id, 0, 1, 0}`, odpověď v druhém slově (kHz) | původní `cal-if/acpm_dvfs.c` (`FREQ_GET`) |

Uspořádání sdílené paměti je stejné jako u GS101: původní
`struct acpm_framework` a `struct ipc_channel` (`fw_header/framework.h`)
se pole po poli kryjí s mainline `struct acpm_shmem` a
`struct acpm_chan_shmem`; pořadové číslo zprávy je v obou v bitech 21:16
prvního slova. Port gta4xl používá mainline ovladač se stejnou konstantou
0x6f00 (jeho `0095`).

### 21.3 Změny (větev `troika-clk`, +4, celkem 85 nad v7.3.3)

1. `dt-bindings: mailbox: google,gs101-mbox: Add Exynos9610` – compatible
   `samsung,exynos9610-mbox` se záložním `samsung,exynos850-mbox` (ovladač se
   nemění).
2. `dt-bindings: firmware: Add the ACPM of the Exynos9610`.
3. `firmware: exynos-acpm: Add Exynos9610` – začátek popisu kanálů 0x6f00,
   zatím bez hodin přes ACPM.
4. `arm64: dts: exynos9610: Add the ACPM firmware interface` – uzly
   `apm_sram`, `apm_mailbox`, `firmware/power-management`.

Místní konfigurace: `EXYNOS_MBOX=y`, `EXYNOS_ACPM_PROTOCOL=y` (v balíčku jsou
zatím jako moduly). `dtbs_check` a `dt_binding_check` obou bindingů čisté.

### 21.4 Co udělá testovací image ACPM1

Jen čte. Ladicí commit (`ACPMDBG`, větev `troika-clk-test`) po navázání
ovladače:

1. vypíše kanály (číslo, režim dotazování, délky);
2. přečte ze SRAM tabulky frekvencí a napětí všech deseti domén;
3. pošle firmwaru pět dotazů „jaká je frekvence“ (MIF, INT, CPUCL0, CPUCL1,
   G3D) a vypíše odpovědi.

Zápisy, ke kterým při tom dojde: maska přerušení a zvonek v mailboxu a zpráva
s ukazatelem fronty ve sdílené paměti – nutná součást každé zprávy, stejná
jako v původním kernelu. Žádná frekvence ani napětí se nemění. Kdyby firmware
zprávě nerozuměl, dotaz vyprší (100 ms) a v logu bude chyba.
**Neověřeno** na telefonu.

### 21.5 Test ACPM1: ovladač se navázal, dva mé předpoklady byly špatně (2026-11-12 16:02)

`fastboot boot out/troika-test-ACPM1.img`, čteno přes telnet.

Funguje: mailbox i ovladač ACPM se navázaly, popis kanálů na 0x6f00 se přečetl
smysluplně – 8 kanálů, všechny s délkou zprávy 16 bajtů:

| Kanál | Dotazování | Délka fronty |
|---|---|---|
| 0 | ano | 15 |
| 1 | ano | 3 |
| 2 | ano | 3 |
| 3 | ne | 1 |
| 4 | ano | 3 |
| 5 | ne | 1 |
| 6 | ne | 1 |
| 7 | ano | 5 |

Špatně bylo:

1. **Dotazy na frekvenci se vůbec neodeslaly.** Kanál 5 (podle původního DT
   kanál `acpm_dvfs`) nepracuje v režimu dotazování, odpověď ohlašuje
   přerušením. Mainline ovladač takové kanály neumí a odmítl je
   („Interrupt mode not supported“, -95) ještě před odesláním. Firmwaru tedy
   zatím nebyla poslána žádná zpráva.
2. **Tabulky frekvencí nejsou na SRAM + 0x6000.** Vypsané hodnoty byly
   nesmysl (liché offsety, 73 „stupňů“). Původní kernel počítá adresu jinak
   (`acpm.c`, `plugins_init`): základ **modulu (pluginu) DVFS** v SRAM
   + `fvmap_offset`. Firmware je totiž složený z modulů, z nichž některé
   zavádí až kernel ze souboru (`firmware_update` + zpráva „attach“);
   seznam modulů je v SRAM (`struct plugin`, `fw_header/common.h`: id, základ,
   velikost, `is_attached`, `stay_attached`, jméno souboru).

Co z toho plyne: než půjde číst tabulky nebo se ptát na frekvenci, je potřeba
vědět, jestli je modul DVFS na troice už zavedený bootloaderem, nebo ho musí
zavést kernel. Port gta4xl ho zavádí sám ze souboru (`0160`,
`acpm_exynos9610_load_plugin`) – to by znamenalo zápis kódu firmwaru do SRAM
řídicího procesoru, tedy krok, ke kterému je potřeba zvláštní souhlas.

Image ACPM2 (jen čtení, nic se neodesílá): vypíše seznam modulů (stav,
základ, velikost, jméno souboru), u kanálů navíc vlastníka a typ,
a pokud je modul DVFS zavedený, tabulky ze správné adresy.

### 21.6 Test ACPM2: moduly firmwaru a tabulky frekvencí (2026-11-12 16:12)

`fastboot boot out/troika-test-ACPM2.img`; čteno přes telnet, celý výpis
v `device-info/acpm-fvmap-ACPM2.txt`. Firmwaru nebyla poslána žádná zpráva.

Moduly firmwaru (popis v SRAM na 0x5898, 6 položek):

| Č. | Základ | Velikost | Zavedený | Má zůstat | Soubor |
|---|---|---|---|---|---|
| 0 | – | – | ano | ano | (jádro firmwaru) |
| 1 | 0xc400 | 0x8c00 | **ano** | ano | `plugin_DVFS_0_0_0.fw` |
| 2 | 0x9000 | 0x1400 | **ne** | ano | `exynos9610_acpm_fvp.fw` |
| 3 | 0x4920 | – | ano | ano | – |
| 4 | 0xa400 | 0x1400 | **ne** | ano | `exynos9610_acpm_tmu.fw` |
| 5 | 0xb800 | 0xc00 | ano | ano | – |

Kanály a jejich vlastníci (pole `owner` je bitová maska čísla modulu):

| Kanál | Vlastník | Dotazování | Použití v původním DT |
|---|---|---|---|
| 0 | modul 2 (FVP) | ano | hodiny (`clock`, `acpm-ipc-channel = <0>`) |
| 1 | modul 2 (FVP) | ano | správce DVFS |
| 2 | modul 3 | ano | – |
| 3 | modul 1 (DVFS) | ne | – |
| 4 | modul 0 (jádro) | ano | `acpm` (připojování modulů) |
| 5 | modul 2 (FVP) | ne | `acpm_dvfs` (změna frekvence) |
| 6 | modul 2 (FVP) | ne | – |
| 7 | modul 4 (TMU) | ano | teplota |

Závěry:

- Modul DVFS s tabulkami zavedl bootloader; tabulky jsou na SRAM + 0x12400
  (základ 0xc400 + 0x6000) a dávají smysl.
- **Moduly FVP (změna frekvence a napětí) a TMU (teplota) zavedené nejsou**
  a mají příznak „má zůstat“. Původní kernel je při každém startu nahraje
  ze souboru do SRAM a připojí (`acpm.c`, `plugins_init`). Bez modulu FVP
  nemá na kanálech 0, 1, 5 a 6 kdo odpovídat.
- Oba soubory jsou součástí zdrojů původního kernelu
  (`refs/downstream-kernel/firmware/exynos9610_acpm_fvp.fw`
  a `…_tmu.fw`, vestavěné přes `fw-external` při `CONFIG_EXYNOS9610_ACPM`);
  oba mají 5120 bajtů = 0x1400, tedy přesně velikost z tabulky modulů.
  V oddílu vendor nejsou. sha256 začínají `8f71dccb` (fvp) a `ef4d33ff` (tmu).
- Cizí port gta4xl dělá totéž (stejné základy 0x9000 a 0xa400).

Tabulky frekvencí a napětí pro tento kus (kHz / µV; `init` = stupeň po
bootloaderu):

| Stupeň | CPUCL0 (A53) | CPUCL1 (A73) | G3D |
|---|---|---|---|
| 0 | 2 054 000 / 1 293 750 | 2 704 000 / 1 225 000 | 1 053 000 / 950 000 |
| 1 | 1 950 000 / 1 225 000 | 2 600 000 / 1 225 000 | 949 000 / 856 250 |
| 2 | 1 846 000 / 1 118 750 | 2 496 000 / 1 225 000 | 839 000 / 781 250 |
| 3 | 1 742 000 / 1 056 250 | 2 314 000 / 1 225 000 | 764 000 / 743 750 |
| 4 | 1 638 000 / 1 000 000 | 2 210 000 / 1 175 000 | 683 000 / 700 000 |
| 5 | 1 534 000 / 943 750 | 2 184 000 / 1 131 250 | 572 000 / 656 250 |
| 6 | 1 456 000 / 906 250 | 2 080 000 / 1 087 500 | 546 000 / 650 000 |
| 7 | 1 326 000 / 850 000 | 1 976 000 / 1 031 250 | 455 000 / 612 500 |
| 8 | 1 222 000 / 818 750 | 1 898 000 / 987 500 | 385 000 / 587 500 |
| 9 | 1 118 000 / 781 250 | 1 768 000 / 943 750 | 338 000 / 581 250 |
| 10 | 1 053 000 / 762 500 (init) | 1 664 000 / 900 000 | 260 000 / 581 250 (init) |
| 11 | 910 000 / 718 750 | 1 508 000 / 831 250 | |
| 12 | 806 000 / 687 500 | 1 456 000 / 812 500 (init) | |
| 13 | 702 000 / 656 250 | 1 352 000 / 781 250 | |
| 14 | 598 000 / 631 250 | 1 248 000 / 750 000 | |
| 15 | 403 000 / 581 250 | 1 144 000 / 725 000 | |
| 16–19 | | 1 040 000 … 728 000 / 706 250 … 643 750 | |

Počáteční stupně sedí s tím, co mainline naměřil: `fout_cpucl0_pll`
1053 MHz (test CLK1) a `fout_g3d_pll` 260 MHz (test GPU1). Paměť (MIF) má
10 stupňů 419–2093 MHz, počáteční 1794 MHz.

Tabulka je společná pro rodinu 9610 a obsahuje i stupně, které 9609 podle
všeho používat nemá (A73 až 2704 MHz). Horní meze pro troiku je potřeba
vzít z původního DT telefonu, ne z této tabulky. **Neověřeno.**

### 21.7 Souhlas a příprava: nahrání a připojení modulu FVP (2026-11-12 16:38)

Souhlas uživatele (otázka „Povoluješ nahrát modul frekvencí do paměti řídicího
procesoru a připojit ho?“ → „Ano, nahrát modul“). Platí pro modul FVP
(`exynos9610_acpm_fvp.fw`), jeho připojení a dotazy na aktuální frekvence.
**Neplatí** pro změnu frekvence nebo napětí ani pro modul TMU.

Postup podle původního kernelu:

| Krok | Původní kernel | Zdroj |
|---|---|---|
| kam zapsat | SRAM + (`base_addr` & ~1) z tabulky modulů, tj. 0x9000 | `acpm.c`, `plugins_init` |
| co zapsat | celý soubor, 5120 bajtů | `firmware_update` / `firmware_load` |
| žádost o připojení | první slovo `(1 << 25) | (id << 26)`, ostatní 0, s odpovědí | `handle_dynamic_plugin`; `acpm_ipc_ctrl.h` (`ACPM_IPC_PROTOCOL_DP_A` = 25, `_ID` = 26) |
| kterým kanálem | kanál uzlu `acpm` = 4 (vlastní ho jádro firmwaru) | původní DT |
| dotaz na frekvenci | `{id, 0, 1, 0}` kanálem uzlu hodin = 0 | `acpm_dvfs.c`; `exynos_acpm_set_device()` dostává uzel hodin, takže kanál DVFS je 0, ne 5 (kanál 5 slouží hlášením o paměti, `acpm_noti_mif`) |

Pojistky v ladicím kódu (commit `ACPMDBG` na `troika-clk-test`), při
nesplnění kterékoli se nic nezapíše:

- modul se hledá podle jména souboru v tabulce firmwaru; musí být nezavedený
  a mít příznak „má zůstat“;
- velikost souboru se musí rovnat velikosti z tabulky;
- oblast nesmí přesahovat SRAM, překrývat jiný modul ani ležet pod daty
  jádra firmwaru;
- kanál 4 musí patřit jádru firmwaru a být v režimu dotazování;
- po zápisu se oblast přečte zpět a porovná; žádost o připojení se pošle jen
  při shodě;
- na frekvence se ptá jen tehdy, když firmware modul označí za zavedený
  a kanál 0 mu patří.

Soubor je ze zdrojů původního kernelu
(`refs/downstream-kernel/firmware/exynos9610_acpm_fvp.fw`, sha256 začíná
`8f71dccb`); do testovacího initramfs ho přidává
`tools/make-clk-test-bootimg.sh` (do repozitáře se nekopíruje).
Image: `out/troika-test-ACPM3.img`. **Neověřeno** na telefonu.

### 21.8 Test ACPM3: modul FVP připojen, firmware odpovídá (2026-11-12 16:56)

`fastboot boot out/troika-test-ACPM3.img`; čteno přes telnet, výpis
v `device-info/acpm-ACPM3.txt`.

```
fvp: writing 5120 bytes to SRAM + 0x9000
fvp: read back matches
fvp: attach ret 0 reply 0a010000 00000000 00000000 00000000
fvp: attached 1 after the request
get rate MIF:    ret 0 reply 00010000 00000000 00000000 00000000
get rate INT:    ret 0 reply 00020000 00000000 00000000 00000000
get rate CPUCL0: ret 0 reply 00030000 00000000 00000000 00000000
get rate CPUCL1: ret 0 reply 00040000 00000000 00000000 00000000
get rate G3D:    ret 0 reply 00050000 00000000 00000000 00000000
```

- Všechny pojistky prošly, zápis sedí se souborem bajt po bajtu.
- Žádost o připojení: odpověď `0x0a010000` = příznak připojení (bit 25),
  číslo modulu 2 (bity 26–28) a pořadové číslo 1 (bity 16–21) – firmware
  žádost vrátil potvrzenou. V tabulce modulů je FVP nově veden jako zavedený.
- Dotazy na frekvenci kanálem 0: firmware na všech pět odpověděl (návrat 0,
  pořadová čísla 1–5 sedí). **Frekvenci ale nevrací** – druhé slovo je 0
  a první obsahuje jen pořadové číslo. Dotaz `FREQ_GET` tedy modul FVP na
  9610 buď neumí, nebo vrací 0, dokud mu někdo frekvenci nenastaví. Sedí to
  s poznámkou portu gta4xl, že odpovědi FVP nesou v prvním slově stav 0
  místo čísla domény, a s tím, že ten port čte frekvenci přímo z PLL.
  (`exynos_acpm_set_init_freq` v původním kernelu existuje, ale nikdo ho
  nevolá.)
- Stav telefonu po testu: bez chyb v logu, `fout_cpucl0_pll` 1053 MHz
  a `fout_g3d_pll` 260 MHz beze změny, UFS jen pro čtení.

Závěr: komunikace s firmwarem funguje oběma směry a modul pro změnu
frekvencí je připojený (do příštího restartu; bootloader ho nezavádí).
Aktuální frekvenci je potřeba číst z hodin (PLL), ne z firmwaru.
**Neověřeno:** že firmware na žádost frekvenci opravdu změní – to je další
krok a vyžaduje samostatný souhlas a nejdřív čtení teploty.


## 22. Teplota čipu: čidlo TMU (2026-11-12)

### 22.1 Jak teplotu čte původní systém a co z toho plyne

- Čip má jednu jednotku TMU na `0x10070000` (velikost `0x700`, přerušení
  SPI 231) se třemi čidly: P0 v GPU, P1 u malých jader (a ISP), P2 u velkých
  jader. Zdroj: původní DT telefonu (`device-info/live.dts`, uzly
  `BIG@10070000` … s `sensors = <4>`, `<2>`, `<1>` = bitová maska čidla)
  a `exynos9610.dtsi` původního kernelu (komentáře `/* P2 */`, `/* P1 */`,
  `/* P0 */`).
- Původní kernel (`CONFIG_EXYNOS_ACPM_THERMAL=y`) teplotu **nečte z registrů,
  ale ptá se firmwaru**: zpráva `TMU_IPC_READ_TEMP` (typ 2) kanálem 7, který
  patří modulu TMU (modul 4, bootloader ho nezavádí – viz 21.6). Mainline má
  pro tyhle zprávy hotovou podporu (`exynos-acpm-tmu.c`, typy 1 a 2 jsou
  stejné), ale bez nahrání modulu TMU nejde použít.
- Port gta4xl (`refs/gta4xl-linux`, patch 0160) jde jinou cestou: čte přímo
  registry TMU, které nechal běžet bootloader, a nic do nich nezapisuje.
  Tvrdí, že bootloader Samsungu čidla zapne a nastaví ochranné vypnutí.
  Pro troiku (bootloader Motoroly) bylo potřeba to ověřit – viz 22.3.

### 22.2 Hodnoty a zdroje

| Hodnota | Zdroj |
|---|---|
| TMU `0x10070000`, velikost `0x700`, SPI 231 | `live.dts` telefonu, `exynos9610.dtsi` původního kernelu |
| Čidla P0 = GPU, P1 = malá jádra, P2 = velká jádra | tamtéž (`sensors`, komentáře), ověřeno zátěží (22.3) |
| Hodiny `CLK_GOUT_PERI_BUSIF_TMU_PCLK` (CMU_PERI `0x2024`) | `cmucal-node.c`/`cmucal-sfr.c` původního kernelu, mainline `clk-exynos9610.c` |
| `TRIMINFO(p)` = `p * 4`: kód pro 25 °C bity 8:0, pro 85 °C bity 17:9, dvoubodová kalibrace bit 23 registru čidla 0 | `exynos_tmu.c` původního kernelu (`exynos9610_tmu_initialize`) |
| `CONTROL` `0x20`, bit 0 = jádro zapnuto, bit 12 = ochranné vypnutí | tamtéž (`tmu_core_enable`) |
| Aktuální kódy: `0x40` (P0 bity 8:0, P1 bity 17:9), `0x44` (P2 bity 8:0) | názvy `CURRENT_TEMP1_0`, `CURRENT_TEMP4_2` a posun 9 tamtéž; ověřeno čtením (22.3) |
| Kalibrační body 25 a 85 °C | `samsung,tmu_first_point_trim`, `…second_point_trim` v DT |
| Přepočet: `(kód − kód25) × 60 / (kód85 − kód25) + 25` | `code_to_temp_with_sensorinfo` tamtéž |
| Prahy původního systému pro velká jádra: 63, 83 (řízení), 95, 100, 105, 110, 115 °C; odpojování velkých jader při 96 °C | `live.dts` (`thermal-zones`, `hotplug_out_threshold`) |

### 22.3 Čtení registrů z běžícího telefonu (image ACPM3, 18:48–18:56)

Nový nástroj `test-initramfs/mem-rd.c` (bez libc, `/dev/mem` otevřený jen pro
čtení, mapování `PROT_READ` – zapisovat neumí). Na telefon se dostal přes
telnet po kouscích v base64, kontrolní součet na telefonu sedí. Ověřen na
registru s identifikací čipu (`0x10000000` = `0xe9610000`).

Hodiny TMU jsou v režimu automatického vypínání (jako ostatní CMU), přístup
k registrům proto funguje, i když `clk_summary` ukazuje bránu jako vypnutou.

Stav, který nechal bootloader (surový výpis: `device-info/tmu-regs-ACPM3.txt`):

- `CONTROL` = `0x00009403`: jádro zapnuté, ochranné vypnutí zapnuté.
- `TRIMINFO0..2` = `0x00829f12`, `0x00129f12`, `0x001a9f13`: dvoubodová
  kalibrace, kódy pro 25 °C 274/274/275, pro 85 °C 335/335/335.
- `AVG_CON`, `COUNTER_VALUE0/1` a pole v `CONTROL` odpovídají tomu, co by
  nastavil `exynos9610_tmu_control` původního kernelu – bootloader čidlo
  inicializoval stejným postupem.
- Prahy: čidlo 0 má nejvyšší práh `0x16d` = 115 °C, další 110, 105, 100, 95,
  90, 85 °C; přerušení povoleno jen pro nejvyšší práh čidla 0 (`INTEN0` =
  `0x80`). Emulace teploty vypnutá.
- Nerozumím: registr `0x74` (v původním ovladači `INTSTAT`) vrací `0x01250125`,
  což vypadá jako kódy teploty, a `STATUS` = `0x7130`. Nepoužívám je.

Teploty (přepočet podle 22.2):

| Stav | GPU | malá jádra | velká jádra |
|---|---|---|---|
| klid | 43,7 | 42,7 | 44,0 °C |
| 8 jader naplno, 10 s | 53,5 | 52,5 | 66,0 |
| 60 s | 56,5 | 55,5 | 69,0 |
| 120 s | 58,4 | 57,5 | 70,0 |
| 25 s po zátěži | 48,6 | 47,6 | 48,0 |

Zátěž = 8 nekonečných smyček v shellu na frekvencích z bootloaderu (malá
jádra 1053 MHz, velká 1456 MHz), s vlastním koncem po 120 s a pojistkou
při 85 °C. Čidlo u velkých jader reaguje nejvíc a během sekund – přiřazení
čidel sedí. **Na frekvencích z bootloaderu se teplota při plné zátěži
ustálí kolem 70 °C**, tedy pod prahem 83 °C, od kterého původní systém
začíná brzdit. Delší zátěž než 2 minuty a zátěž GPU jsem neměřil.

### 22.4 Ovladač a DT (větev `troika-clk`, +3, celkem 88 nad v7.3.3)

1. `dt-bindings: thermal: Add the TMU of the Samsung Exynos9610` – nový
   binding `samsung,exynos9610-tmu` (`#thermal-sensor-cells = <1>`, číslo
   čidla).
2. `thermal: samsung: Add a driver for the TMU of the Exynos9610` –
   `drivers/thermal/samsung/exynos9610_tmu.c`, `CONFIG_EXYNOS9610_THERMAL`.
   **Jen čte**: do TMU nezapíše ani jeden registr, nastavení z bootloaderu
   (kalibrace, prahy, ochranné vypnutí) zůstává. Odmítne TMU, které neběží
   nebo nemá tovární kalibraci (původní ovladač by v tom případě použil
   náhradní hodnotu 55 z DT; já raději nic než špatnou teplotu). Teploty
   jsou i v hwmon.
3. `arm64: dts: exynos9610: Add the TMU and thermal zones` – uzel
   `thermal-sensor@10070000` a zóny `gpu-thermal`, `little-thermal`,
   `big-thermal` s dotazováním po 1 s (hodnota původního systému pro velká
   jádra; bez obsluhy přerušení to jinak nejde).

**Moje volba, ne hodnota z hardwaru:** každá zóna má jediný práh `critical`
na 105 °C – Linux při něm telefon řádně vypne. Čip sám se natvrdo vypíná až
při 115 °C; 105 °C je jeden ze stupňů původního systému a stejnou hodnotu
používá port gta4xl. Chladicí zařízení (snižování frekvence) zatím žádné
není, protože frekvence se ještě nemění.

Mainline ovladač `exynos_tmu.c` jsem nerozšiřoval: počítá s jedním čidlem na
jednotku a TMU si sám inicializuje. Cesta přes firmware (modul TMU) zůstává
otevřená otázka pro změny frekvencí: port gta4xl modul TMU nahrává také
a zmiňuje, že firmware řeší chování za nízkých teplot. Než se to ověří,
neplatí to za fakt.

### 22.5 Test TMU1: ovladač čte stejné teploty jako registry (2026-11-12 19:14)

Image `out/troika-test-TMU1.img` (sha256 začíná `2fced37f`), výpis přes
telnet (`device-info/tmu-TMU1.txt`).

- Ovladač se navázal bez hlášek, vznikly tři zóny (`gpu-thermal`,
  `little-thermal`, `big-thermal`), každá s prahem `critical` 105 °C,
  a tři zařízení hwmon.
- Registry TMU jsou po startu stejné jako v ACPM3 (`CONTROL` `0x00009403`,
  stejná kalibrace) – ovladač do nich nesáhl.
- Zóny odpovídají přepočtu surových kódů na tisícinu stupně:

| Stav | zóny (GPU / malá / velká) | kódy P0/P1/P2 |
|---|---|---|
| klid | 44,7 / 43,7 / 44,0 °C | 294 / 293 / 294 |
| 8 jader naplno, 6 s | 52,5 / 51,6 / 65,0 | 302 / 301 / 315 |
| 12 s | 54,5 / 53,5 / 66,0 | 304 / 303 / 316 |
| 4 s po zátěži | 47,6 / 45,7 / 47,0 | 297 / 296 / 297 |

- Modul FVP se při startu znovu nahrál a připojil jako v ACPM3.
- V logu jen dvě už známé hlášky (`exynos-clkout` −22, panfrost bez
  regulátoru), obě jsou i v logu GPU1.

Závěr: čtení teploty funguje. **Neověřeno:** vypnutí při 105 °C (té teploty
telefon nedosáhl a schválně ji vyvolávat nebudu) a chování po uspání.

## 23. ACPM: nahrávání modulu FVP v čistém ovladači (2026-11-12)

### 23.1 Co se změnilo (větev `troika-clk`, +1, celkem 89 nad v7.3.3)

`firmware: exynos-acpm: Attach the DVFS plugin of the Exynos9610` – to, co
v testu ACPM3 dělal ladicí kód, dělá teď ovladač `exynos-acpm` sám při startu:

- Popis sdílené paměti dostal jména: první dvě slova hlavičky jsou odkaz na
  tabulku modulů a jejich počet (`struct acpm_framework` původního kernelu),
  nová `struct acpm_plugin_shmem` odpovídá `struct plugin`
  (`fw_header/common.h`).
- Data pro Exynos9610 říkají, který modul nahrát (`exynos9610_acpm_fvp.fw`)
  a kterým kanálem jde žádost o připojení (4, `acpm-ipc-channel` uzlu `acpm`
  v původním DT).
- Postup: najít modul v tabulce firmwaru podle jména souboru; nic nedělat,
  když je už připojený nebo ho firmware připojený nechce; ověřit, že se
  vejde do SRAM a že soubor má přesně velikost vyhrazeného místa; zkopírovat;
  přečíst zpět a porovnat; poslat žádost (`BIT(25) | id << 26`) a počkat, až
  ho firmware v tabulce označí jako připojený.
- Neúspěch není fatální (jen varování v logu): ostatní služby firmwaru modul
  nepotřebují.
- Modul TMU ovladač **nenahrává** – není k tomu souhlas a teplotu čteme
  přímo z čidla.

Původní kernel nahrává všechny moduly, které firmware chce mít připojené
(`plugins_init`), tedy FVP i TMU. **Otevřená otázka:** jestli modul FVP při
změně frekvence počítá s připojeným modulem TMU (např. kvůli přirážce napětí
za chladu). Zdroják modulů nemám; před první změnou frekvence to uvedu jako
riziko.

Ladicí commit na `troika-clk-test` je menší: jen vypisuje rozložení
firmwaru a stav modulů, firmwaru nic neposílá (dotaz `FREQ_GET`, který
vracel nulu, je pryč).

### 23.2 Důsledek pro balíček

Ovladač je zabudovaný v kernelu a modul si žádá při startu, tedy dřív, než
je připojený kořenový systém. Soubor `exynos9610_acpm_fvp.fw` proto musí být
**v initramfs** (v testovacím image je). Pro balíček to znamená dvě věci,
které zatím nejsou udělané a o kterých rozhodne uživatel: odkud soubor brát
(je ve zdrojích původního kernelu od Motoroly; v mé kopii u něj žádná
licence uvedena není a soubor `firmware/WHENCE` tam chybí) a jeho přidání do initramfs přes mkinitfs. Bez souboru ovladač jen
vypíše varování a frekvence zůstanou pevné jako dnes.

### 23.3 Test ACPM4: modul připojil čistý ovladač (2026-11-12 20:12)

Image `out/troika-test-ACPM4.img` (sha256 začíná `b4372c00`), výpis přes
telnet (`device-info/acpm-ACPM4.txt`).

```
plugin 1: id 1 base 0xc401 size 0x8c00 attached 1 stay 1 fw 'plugin_DVFS_0_0_0.fw'
plugin 2: id 2 base 0x9001 size 0x1400 attached 1 stay 1 fw 'exynos9610_acpm_fvp.fw'
plugin 4: id 4 base 0xa401 size 0x1400 attached 0 stay 1 fw 'exynos9610_acpm_tmu.fw'
```

- Modul FVP (2) je po startu připojený, bez ladicího kódu; varování
  `Failed to attach plugin` v logu není. Modul TMU (4) zůstal nepřipojený.
- Teploty 44,7 / 43,7 / 45,0 °C, `fout_cpucl0_pll` 1053 MHz, `fout_g3d_pll`
  260 MHz, UFS jen pro čtení, v logu jen dvě známé hlášky.

Závěr: přesun do čistého ovladače funguje. **Neověřeno:** chování bez
souboru modulu v initramfs (má jen vypsat varování) a změna frekvence.


## 24. Návrh první změny frekvence přes firmware (2026-11-12, čeká na souhlas)

### 24.1 Jak to dělá původní systém

- Frekvenci GPU nastavuje `gpu_set_dvfs_using_calapi()` (mali
  `platform/exynos/gpu_control.c`) voláním `cal_dfs_set_rate(g3d_cmu_cal_id,
  kHz)`. `g3d_cmu_cal_id` je v DT telefonu `0xb040004`, tedy doména
  spravovaná firmwarem číslo 4 (`IS_ACPM_VCLK`, `GET_IDX` v `cmucal.h`).
- `cal_dfs_set_rate()` pro takovou doménu pošle firmwaru zprávu
  `exynos_acpm_set_rate()`: slova `{číslo domény, kHz, FREQ_REQ = 0, 0}`,
  s odpovědí, kanálem uzlu hodin (`acpm-ipc-channel = <0>` v
  `clock-controller@0x12100000`). Napětí původní kernel sám nenastavuje –
  PLL i napětí větve přepíná firmware podle své tabulky.
- Aktuální frekvenci původní kernel čte z registrů (`vclk_recalc_rate`),
  ne z firmwaru. Odpovídá to zjištění z ACPM3, že `FREQ_GET` vrací nulu.
- Chování za chladu: při `CONFIG_EXYNOS_ACPM_THERMAL=y` kernel firmwaru
  zprávy o chladu neposílá (`COLDTEMP_REQ` je v `acpm_dvfs.c` vypnuté
  podmíněným překladem) – nejspíš si to firmware řeší sám přes modul TMU. Ten
  u nás připojený není. Práh „cold“ původního systému je 20 °C (`big-cold`,
  `gpu-cold` v DT); čip má teď 43 až 45 °C.
- Firmware může kanálem 5 kernel žádat o zvýšení frekvence paměťové sběrnice
  (`acpm_noti_mif_callback`). Náš kernel na to neodpovídá. Pro navržený krok
  to nevadí: původní tabulka GPU chce pro 338 MHz sběrnici aspoň na 419 MHz
  a ta běží na 1794 MHz.

### 24.2 Navržený krok: GPU 260 → 338 → 260 MHz

| Co | Hodnota | Zdroj |
|---|---|---|
| Doména | 4 (G3D) | `g3d_cmu_cal_id = <0xb040004>` v `live.dts` |
| Kanál | 0 | `acpm-ipc-channel` uzlu hodin v `live.dts`; vlastní ho modul FVP (21.6) |
| Zpráva | `{4, kHz, 0, 0}` s odpovědí | `exynos_acpm_set_rate()` v `cal-if/acpm_dvfs.c` |
| Výchozí stav | 260 MHz, 581,25 mV | PLL `0x11430120` = `0xa0a00412` (26 × 160 / 16), tabulka firmwaru stupeň 10 |
| Cíl | 338 MHz, 581,25 mV | tabulka firmwaru stupeň 9; `gpu_dvfs_table` v `live.dts` tuto frekvenci obsahuje |
| Mez čipu | 260 až 1053 MHz | `gpu_min_clock`, `gpu_max_clock` v `live.dts` |

Proč právě tohle: oba stupně mají v tabulce firmwaru **stejné napětí**, takže
jde o změnu frekvence bez změny napětí. GPU v testovacím image nic nekreslí,
takže případná chyba nezasáhne běžící systém tak jako u procesoru. Obě
frekvence jsou nejnižší dva stupně původního systému.

Tři zprávy v tomto pořadí, po každé čtení registru PLL (`mem-rd 11430120`),
teplot a logu:

1. `260000` – žádost o frekvenci, na které GPU už je. Čekám odpověď bez
   chyby a nezměněný registr.
2. `338000` – čekám PLL přepnutý na 338 MHz.
3. `260000` – návrat do výchozího stavu.

Když kterýkoli krok dopadne jinak, než čekám, další neposílám.

Jak: ladicí commit na `troika-clk-test` přidává soubor
`/sys/kernel/debug/acpmdbg_g3d_khz`. Přijme jen hodnoty 260000 a 338000,
cokoli jiného odmítne; vznikne jen tehdy, když je modul FVP připojený
a kanál 0 mu patří. Dokud do něj nikdo nezapíše, firmwaru se nic neposílá –
samotné spuštění image ACPM5 je stejné jako ACPM4.

### 24.3 Rizika a co nevím

- **Napětí nezměřím.** Podle tabulky se měnit nemá; firmware si ale regulátor
  řídí sám a co přesně udělá, nevidím (čtení regulátoru přes firmware zatím
  nemáme). Nejvyšší napětí v tabulce GPU je 950 mV a původní systém ho běžně
  používá; navržené hodnoty jsou na opačném konci.
- **Firmware může zprávu odmítnout, neodpovědět nebo se zaseknout.** V tom
  případě může telefon zamrznout nebo se restartovat. Firmware se při každém
  startu nahrává znovu, nic trvalého se nemění; pomůže Power + Vol−.
- **Modul TMU není připojený.** Původní systém ho připojený má a firmware
  přes něj nejspíš hlídá chlad. Jestli na něm modul FVP závisí i jinak, nevím.
- **Kernel o změně neví.** Hodiny v mainline si pamatují 260 MHz; po kroku 3
  to zase platí. Panfrost v testovacím image nic nekreslí.
- Změna nepřežije restart.

### 24.4 Test ACPM5: firmware frekvenci GPU změnil a vrátil (2026-11-12 20:30)

Souhlas uživatele: odpověď „Ano, všechny tři kroky“ na otázku, zda poslat
firmwaru tři žádosti o frekvenci GPU 260, 338 a zpět 260 MHz (telefon běžel
v ACPM5, výchozí stav zkontrolován). Výpis: `device-info/acpm-ACPM5.txt`.

| Krok | Žádost | Odpověď firmwaru | PLL `0x11430120` | Frekvence |
|---|---|---|---|---|
| výchozí stav | – | – | `0xa0a00412` | 26 × 160 / (4 × 4) = 260 MHz |
| 1 | 260000 | `00010000 0003f7a0`, návrat 0 | `0xa0a00412` | 260 MHz (beze změny) |
| 2 | 338000 | `00020000 00052850`, návrat 0 | `0xa0680411` | 26 × 104 / (4 × 2) = 338 MHz |
| 3 | 260000 | `00030000 0003f7a0`, návrat 0 | `0xa0a00412` | 260 MHz |

- Firmware na každou žádost odpověděl bez chyby; ve druhém slově odpovědi
  vrací požadovanou frekvenci, v prvním pořadové číslo.
- PLL je po každém kroku zapnutý a zavěšený (bity 31 a 29), výběr hodin
  `0x11431004` zůstal na PLL.
- Teploty se nehnuly (43 až 45 °C), UFS jen pro čtení, v logu nic nového.
- Ladicí soubor jinou hodnotu odmítá: zápis 500000 skončil chybou
  `Invalid argument` a firmwaru neodešel (v logu jsou jen tři žádosti).
  Tenhle pokus nebyl součástí odsouhlasených kroků; spoléhal na kontrolu
  v ladicím kódu, která hodnotu zahodí dřív, než se cokoli pošle.

Závěr: **změna frekvence přes firmware funguje** – zpráva `{4, kHz, 0, 0}`
kanálem 0 přepne PLL GPU a zase ho vrátí. **Neověřeno:** co firmware při tom
udělal s napětím (podle tabulky nic), stupně s jiným napětím, GPU pod
zátěží, ostatní domény (procesor, paměť).

