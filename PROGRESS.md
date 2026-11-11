# PROGRESS – port Nura na Motorola One Action (troika)

## Stav: testy UFS1 a UFS2 prošly – kernel vidí vnitřní úložiště (jen pro čtení); UFS zatím není v balíčku pmaports

USB síť na telefonu funguje z místně sestaveného kernelu (test USB1). Balíček r3 se
ještě nestavěl. Další krok je na uživateli: sestavit balíčky, `install`, `export`,
pak kontrola boot image a netboot (TESTING.md kap. 5f). Nic se neflashovalo.

## Pravidla (platí pořád)

- Žádné `fastboot flash`, `fastboot erase`, `pmbootstrap flasher`.
  Test jen přes `fastboot boot` a spouští ho uživatel.
- Nesahat do `~/troika-backup`.
- Žádná vymyšlená hardwarová hodnota; bez zdroje = TODO + dotaz.
- Regulátory, nabíjení, napájení v DTS nepovolovat.

## Hotovo ve fázi 1

- [x] Stav mainline (linux-next next-20261109): chipid + pinctrl přijaty, zbytek ne.
- [x] Lore: clk v3 (2026-07-22), dts v1 (2025-09-14), pinctrl „Require clocks“
      – přečteno včetně review, výtahy v `refs/lore/`.
- [x] Wiki Nura (obě stránky), pmaports (balíček neexistuje, jen uzavřený MR !6458).
- [x] Downstream kernel + LineageOS device tree: tlačítka, fb, USB, boot image.
- [x] `FINDINGS.md`.
- [x] `pmbootstrap bootimg_analyze` – proběhl 2026-11-10 15:20 na
      `~/troika-backup/boot_a.img` (LineageOS 22): header v1, pagesize 2048,
      base 0x10000000, kernel 0x8000, `dtb_second=true`. Výstup ve FINDINGS.md kap. 4.

## Záloha firmwaru (odpověď uživatele 2026-11-10)

Uživatel **zálohu nemá**. Možnosti, jak získat boot.img/dtbo.img (čeká se na volbu):

- A) Vyčíst oddíly přímo z telefonu (`dd` z `/dev/block/bootdevice/by-name/boot_a`
  apod., jen čtení; potřebuje root nebo recovery s `adb root`). Jediná cesta ke
  skutečné záloze tohoto kusu.
- B) Oficiální stock firmware přes Motorola Rescue and Smart Assistant
  (stahuje uživatel sám).
- C) `boot.img`, `dtbo.img`, `vbmeta.img` z download.lineageos.org
  (build `lineage-24.0-20261109-nightly-troika`). Není to stock, ale formát
  boot image je stejný a DTB ve „second“ je ze zdrojáků v `refs/downstream-kernel`.
  Stačí pro `bootimg_analyze` a pro uniLoader; nestačí pro návrat ke stocku.

Na telefonu běží **LineageOS** (odpověď 2026-11-10) → zvolena cesta A.
Bootloader je tedy odemčený; `fastboot boot` zatím neověřen.

Připraven `tools/dump-from-phone.sh` (spouští uživatel; na telefonu jen čte):
- oddíly `boot`, `dtbo`, `vbmeta` obou slotů → `~/troika-backup/` + `SHA256SUMS`
  (pozor: je to LineageOS, ne stock – záloha současného stavu),
- `device-info/live.dtb` (`/sys/firmware/fdt` = DTB od bootloaderu včetně uzlů
  `memory` a `reserved-memory`), `iomem.txt`, `cmdline.txt`, `dmesg.txt`,
  `soc0.txt`, `gpio.txt`, `props.txt`.
**Skript doběhl 2026-11-10 13:27 bez chyby.** Záloha je v `~/troika-backup/`
(6 image + `SHA256SUMS`), data v `device-info/`. Rozbor hotový, výsledky ve
FINDINGS.md kap. 8:

- RAM: 4 oblasti z `live.dts:13-31` (celkem 4027 MiB) – vyřešeno.
- `bootloader_fb` `0xec000000`/`0x4000000` potvrzeno z live DTB.
- Tlačítka active-low, chip ID `EXYNOS9610`, panel Himax HX83112A.
- boot image: header v1, DTB ve „second“ (rozebráno `unpack_bootimg`).
- Telefon běží LineageOS 22 (kernel 4.14.113), aktivní slot `_a`, bootloader odemčený.

Zbývá: formát pixelů (až test), chování `fastboot boot` na MBM bootloaderu,
název deviceinfo klíče `dtb_second` vs. `bootimg_dtb_second` (ověřit v boot-deploy).

## Nainstalované nástroje (2026-11-10, bez roota)

- `pmbootstrap` 3.12.1 – klon oficiálního repozitáře v `tools/pmbootstrap`
  (7d23010a125c), symlink `~/.local/bin/pmbootstrap`. `pmbootstrap init`
  proběhl 2026-11-10 15:02 neinteraktivně s výchozími odpověďmi
  (`yes '' | pmbootstrap init`): work `~/.local/var/pmbootstrap`, kanál
  systemd-edge, zařízení `qemu-amd64`, UI console. Pro fázi 2 se přenastaví
  na troiku. Nativní chroot existuje (`pmbootstrap shutdown` ho odpojí).
- `dtc` 1.8.1 (+ `fdtdump`, `fdtget`, `fdtput`, `fdtoverlay`, libfdt) –
  sestaveno z git.kernel.org/pub/scm/utils/dtc (8f48565e5cfe),
  `PREFIX=~/.local`.
- `sudo` vyžaduje heslo, které asistent nezadává → příkazy pmbootstrapu
  pracující s chrootem (`init`, `bootimg_analyze`, build) spouští uživatel.
- Pro fázi 2 stále chybí: aarch64 cross-compiler, `b4`, `dtschema`, `yamllint`.

## Otevřené otázky na uživatele (detail ve FINDINGS.md kap. 6)

1. ~~Kde je záloha firmwaru~~ – záloha není, viz výše; vybrat cestu A/B/C.
2. ~~pmbootstrap, dtc~~ – nainstalováno. Zbývá svolení pro b4, cross-compiler,
   dtschema (až pro fázi 2; 28 GB volných).
3. ~~Rozložení RAM z telefonu~~ – vyřešeno (FINDINGS.md kap. 8.1).
4. Bootloader je odemčený. Funguje na něm `fastboot boot`? (`fastboot getvar all`)
4a. Rezervovat v DTS všechny pevné oblasti z kap. 8.2 jako `no-map` (doporučeno),
    nebo jen framebuffer?
5. Clock driver v3: vynechat z prvního bootu (doporučeno), nebo opravovat hned?
6. uniLoader jako mezičlánek kvůli dtbo overlayi – ano/ne?
7. Compatible: zavést `samsung,exynos9609`?
8. USB síť: odložit (v mainline chybí PHY)?

## Fáze 2 – hotovo, netestováno na hardwaru (potvrzeno uživatelem 2026-11-10: bez clock driveru, uniLoader, bez USB, všech 7 rezervací)

Hotovo:
- [x] Nástroje bez roota: cross-gcc 16.2.0 z kernel.org (`toolchain/`),
      dtschema 2026.9 + yamllint ve venv s Pythonem 3.13 (`toolchain/dtschema-venv`,
      přes `uv`), prostředí `. tools/env.sh`. `b4` nenainstalováno (lore vrací 403 i jemu).
- [x] `linux/` – kernel 7.3.3 z tarballu (sha256 ověřen), větev `troika-mainline`:
      3 Chimacovy patche (dts v1, sestavené z `refs/lore/` do `patches-src/`,
      autorství zachováno) + 12 oprav + binding a DTS troiky = 17 commitů.
- [x] `arch/arm64/boot/dts/exynos/exynos9610-troika.dts`.
- [x] `dtbs_check` W=1: 0 řádků (před opravami 299); `dt_binding_check` OK;
      checkpatch: jen chybějící Signed-off-by / tvar Co-Authored-By.
- [x] `uniLoader-src/` větev `troika`: board `motorola/board-troika.c`, `troika_defconfig`.
- [x] `test-initramfs/init` + statický busybox (Alpine 3.22, `dl/`),
      `tools/make-test-bootimg.sh` (varianty A/B/C do `out/`).
- [x] `pmaports/` větev `motorola-troika-mainline`: `device-motorola-troika`,
      `linux-motorola-troika-mainline` (APKBUILD + 17 patchů).
- [x] `TESTING.md`, FINDINGS.md kap. 9.

- [x] Build kernelu: `build/linux/arch/arm64/boot/Image` (34,5 MB, 7.3.3-troika-gd6ace9a1fea0),
      18 min 39 s, bez varování.
- [x] `tools/update-kernel-pkg.sh` + `pmbootstrap -p ~/nura-troika/pmaports kconfig check
      linux-motorola-troika-mainline` → succeeded (1 INFO: INPUT_EVDEV y/m).
      Pozn.: pmbootstrap vyžaduje, aby `origin` v pmaports mířil na oficiální URL – nastaveno.
- [x] `tools/make-test-bootimg.sh` → `out/troika-test-{A,B,C}.img` + `SHA256SUMS`;
      ověřeno v `build/verify/` (hlavičky = záloha, obsah, rozložení v paměti).
- [x] pmaports commit bb2a0e8 na větvi `motorola-troika-mainline`.

**Fáze 2 je na straně počítače hotová. Čeká se na test uživatele podle TESTING.md
(`fastboot boot out/troika-test-A.img`).**

Neověřeno:
- Nic neběželo na telefonu ani v emulátoru (QEMU není nainstalované).
- Build balíčků přes pmbootstrap (sudo) – příkazy v TESTING.md kap. 6.

Vědomě neuděláno:
- Clock driver v3 + opravy (samostatná větev) – odloženo, první boot ho nepotřebuje.
- Opravy CMGP přerušení (P1/P2) – zdroje si odporují, viz FINDINGS.md 9.2.
- uniLoader není integrovaný do pmaports balíčku – čeká na výsledek testů A–C.
- USB, UFS, regulátory, clocky – mimo rozsah.

Další krok po této fázi: uživatel provede test podle TESTING.md (jen `fastboot boot`).

## Kde co je

- `FINDINGS.md` – výsledky průzkumu se zdroji.
- `TESTING.md` – postup testu přes `fastboot boot`.
- `linux/` (větev `troika-mainline`), `uniLoader-src/` (větev `troika`), `pmaports/` (větev `motorola-troika-mainline`).
- `build/` – výstupy buildu a logy; `out/` – testovací image; `dl/` – stažené zdroje; `toolchain/` – kompilátor a dtschema.
- `refs/` – zdrojové stromy (commity ve FINDINGS.md kap. 7).
- `refs/lore/` – výtahy z lore; `_decode.py` převádí uložený výstup prohlížeče na text.

## Poznámky k prostředí

- lore.kernel.org a wiki.nura.eco jsou za Anubis: `curl`/WebFetch neprojde,
  vestavěný prohlížeč ano.
- `~/Firmware/boot.img` je MediaTek MT6735M (pixi45), ne troika – nepoužívat.
- postmarketOS se přejmenoval na Nura: wiki.nura.eco, gitlab.nura.eco/nura/pmaports
  (staré URL přesměrovávají).

## Testy na telefonu

### 2026-11-10 – test A (`out/troika-test-A.img`), první pokus

- `adb reboot bootloader` OK.
- `fastboot boot` **přijat**: `Sending 'boot.img' (35176 KB) OKAY [0.820s]`,
  `Booting OKAY [0.170s]`. MBM-2.2 tedy `fastboot boot` podporuje a image
  s hlavičkou v1 a uniLoaderem jako kernelem neodmítl.
- Na displeji **zůstala obrazovka bootloaderu beze změny**, žádný text uniLoaderu
  ani kernelu. Telefon se sám nerestartoval, zůstal viset; ukončeno ručně.
- Výklad: buď se uniLoader vůbec nespustil / spadl před kreslením, nebo běží
  (možná i kernel), ale kreslí na `0xec000000`, zatímco displej zobrazuje jiný
  buffer. Z jednoho testu to rozlišit nejde → diagnostický test D.

#### Rozbor a další postup (2026-11-10)

Možné příčiny výsledku testu A, od nejpravděpodobnější neumím seřadit:

1. Bootloader po `Booting OKAY` do image vůbec neskočil (odmítl ho nebo se
   zasekl). Rozdíly proti záloze: hlavička kernelu má `text_offset 0` místo
   `0x80000`, ramdisk je malý neandroidí, image nemá AVB patičku.
2. uniLoader se spustil, ale spadl před kreslením (např. bootloader předává
   řízení se zapnutou MMU/cache, s čímž uniLoader nepočítá).
3. uniLoader (a možná i kernel) běží, ale kreslí na `0xec000000`, zatímco
   displej v režimu fastboot zobrazuje jiný buffer.

Krok 1 – **kalibrace známým dobrým image** (bez nového kódu, bez zápisu):
`fastboot boot ~/troika-backup/boot_a.img` a popsat, co displej dělá mezi
`Booting OKAY` a naběhnutím systému (varování o odemčení? logo? jak dlouho?).
Tím se zjistí, jak vypadá „bootloader pokračuje“, a jestli se to u testu A stalo.

Krok 2 – test C (`out/troika-test-C.img`, holý mainline kernel): jiná cesta
bez uniLoaderu, zadarmo.

Krok 3 – podle výsledků diagnostický uniLoader (test D). Připravené podklady:
adresu zobrazovaného bufferu lze přečíst z DMA registrů DPU (jen čtení):
báze G0 `0x14881000`, G1 `0x14882000`, VG0 `0x14883000`, GF `0x14884000`
(`device-info/live.dts:7306-7358`, komentáře v `exynos9610.dtsi:2514-2562`,
první `reg` je DMA: `dpp_drv.c:806-814`), registr `IDMA_IN_BASE_ADDR_Y` = +0x40,
`IDMA_SRC_SIZE` = +0x10, `IDMA_IMG_SIZE` = +0x18 (`cal_9610/regs-dpp.h:182-222`);
výchozí kanál bootloaderu je `default_idma = 0` (`live.dts:7410`).
Signál restartem přes PMU (`SWRESET`, `exynos-reboot.c:69`) jsem zavrhl –
zápis do PMU nechci dělat bez výslovného souhlasu.

### 2026-11-10 – kalibrace (`~/troika-backup/boot_a.img`) a upřesnění testu A

- Po testu A telefon bez problémů naběhl do LineageOS (`adb reboot bootloader` pak fungoval).
- Kalibrace: `fastboot boot ~/troika-backup/boot_a.img` → `OKAY`; displej se
  **změnil na logo/varování** a naběhl Android. Tak tedy vypadá, když bootloader
  image přijme a pokračuje.
- Upřesnění k testu A od uživatele: fastboot menu zůstalo **úplně stejné**
  (žádné logo/varování) a telefon **nebylo nutné ukončovat – naběhl do LineageOS**.
  (Dřívější odpověď „zůstal viset“ tím neplatí.)
- **Nový výklad:** bootloader image A nejspíš **odmítl ještě před skokem**
  (jinak by ukázal logo/varování jako u kalibrace). uniLoader ani kernel se
  tedy pravděpodobně vůbec nespustily; výklady 2 a 3 výše jsou málo pravděpodobné.
  Není jasné, jestli LineageOS naběhl sám, nebo po stisku tlačítka v menu,
  a jestli bootloader vypsal v menu chybovou hlášku.
- Čím se A liší od přijatého image: kernel (uniLoader, `text_offset 0`),
  ramdisk (malý, neandroidí), chybí AVB patička (image vytvořený `mkbootimg`,
  ne výpis oddílu). → bisekce image E1–E3.

- **Pozor, pozorování z testu A a kalibrace jsou nejistá.** Uživatel následně
  napsal „Booting OKAY se mi vůbec neukázalo“. Terminál přitom `Booting OKAY`
  vypsal u obou běhů (záložka 3). Není jasné, jestli myslel displej telefonu,
  nebo jestli jeho odpovědi ve výběru z možností nepopisují to, co viděl.
  Před dalšími závěry je potřeba volný popis, co telefon ukazoval krok za krokem.
- Připraveny bisekční image `out/troika-test-E{1,2,3}.img` (všechny s původním
  DTB ve „second“): E1 = původní kernel + původní ramdisk (jen přebalené, bez AVB
  patičky), E2 = původní kernel + testovací initramfs, E3 = uniLoader + původní
  ramdisk. Použijí se, pokud se potvrdí, že bootloader image A odmítl.

- Upřesněno: uživatel myslel, že se `Booting OKAY` neukázalo **na displeji telefonu**
  (tam se ani ukazovat nemá). Pozorování tedy platí: kalibrace → logo/varování →
  Android; test A → menu beze změny → LineageOS. Stále nevím, jestli LineageOS
  po testu A naběhl sám, nebo po stisku tlačítka, a jestli menu ukázalo chybu.
- Další krok: bisekce E1 → E3 → E2 (TESTING.md kap. 4a). `avbtool` na počítači
  je (`/usr/bin/avbtool`); kdyby E1 neprošel, další pokus je přidat k image hash footer.

### 2026-11-10 – bisekce E1

- `fastboot boot out/troika-test-E1.img` → `OKAY`; **logo/varování, pak Android**.
  Původní kernel + původní ramdisk přebalené `mkbootimg` bez AVB patičky bootloader
  **přijímá**. Způsob zabalení ani chybějící AVB patička tedy nevadí; image A
  odmítl kvůli kernelu (uniLoader) nebo ramdisku (testovací initramfs).
- Další: E3 (uniLoader + původní ramdisk), pak E2.

### 2026-11-10 – bisekce E3

- `fastboot boot out/troika-test-E3.img` → `OKAY`; menu nahradilo **logo/varování
  a na něm to zamrzlo** (žádný text uniLoaderu, žádná konzole).
- Závěry:
  - Bootloader **uniLoader jako kernel přijímá** (hlavička s `text_offset 0` nevadí).
  - Image A byl odmítnut kvůli **testovacímu ramdisku** (malý, neandroidí). E2 by to
    potvrdil; pro další testy se do boot.img dává původní ramdisk – uniLoader stejně
    použije svůj vložený.
  - Po skoku se na displeji nic nezměnilo: buď uniLoader spadl dřív, než kreslil,
    nebo kreslí na `0xec000000` a displej zobrazuje jiný buffer. uniLoader hned po
    startu maže framebuffer na černo, takže při správné adrese by logo zmizelo.
- Další: diagnostický uniLoader (test D) – obarví celou oblast `bootloader_fb`,
  přečte adresy bufferů z DMA registrů DPU (jen čtení) a zkusí PSCI reset.

### 2026-11-10 – připraven test D

- `uniLoader-src` commit e25f083: `board/motorola/board-troika-diag.c`,
  `configs/troika_diag_defconfig` (popis v commitu a v TESTING.md kap. 4b).
- `tools/make-test-bootimg.sh` teď staví i `out/uniLoader-troika-diag` a
  `out/troika-test-D.img` (diag + původní ramdisk + původní DTB); ověřeno v `build/verify/D`.
- Přeložený kód zkontrolován (`mrs cntvct_el0`, konstanta 26 000 000, `smc #0`).
- Čeká se na výsledek `fastboot boot out/troika-test-D.img`.
- Poznámka pro balíčky: bootloader odmítá boot.img s malým neandroidím ramdiskem
  (test A); příčina neznámá (velikost? obsah?). E2 zatím neproběhl.

### 2026-11-10 – test D (diagnostika displeje): uniLoader běží a kreslí

- `fastboot boot out/troika-test-D.img` → `OKAY`. Popis uživatele: po logu
  **černá obrazovka**, pak **fialová barva přejela postupně od horního okraje**,
  až byl celý displej fialový, potom se **zobrazil text uniLoaderu**. Telefon se
  sám nerestartoval; vypsal výzvu k podržení tlačítek (= `PSCI reset returned`).
- Závěry:
  - **uniLoader se spouští, relokace funguje, displej jde z uniLoaderu ovládat.**
  - Zobrazovaný buffer leží **uvnitř oblasti `bootloader_fb`** (purpurová výplň
    oblasti byla vidět). Jestli přesně na `0xec000000`, ukáže řádek
    `TROIKA DIAG: console at 0x...` na fotce (čeká se).
  - Výplň byla vidět jako postupné přejíždění → zápis do RAM je pomalý, cache jsou
    nejspíš vypnuté. Kopírování uniLoaderu (35 MB) a kernelu (34,5 MB) proto může
    trvat jednotky až desítky sekund – u E3 bylo možná potřeba čekat déle.
  - **PSCI `SYSTEM_RESET` firmware nepodporuje** (volání se vrátilo). Restart
    z mainline půjde jen přes PMU (jako downstream), což zatím neděláme.
- Čeká se na fotku s hodnotami `console at`, `G0/G1/VG0/GF en= src= img= addr=`.

- **Fotka z testu D (17:05):** `TROIKA DIAG: console at 0xec000000`,
  `G0 en=4 src=9d80438 img=9d80438 addr=ec000000`, G1/VG0/GF samé nuly,
  odpočet 30→5 s, `PSCI SYSTEM_RESET...`, `[ERROR] PSCI reset returned`.
  Barvy štítků správně (jantarová, červená).
  → **framebuffer potvrzen: 0xec000000, 1080×2520, stride 4320, a8r8g8b8.**
- Kernel: commit DTS troiky upraven (`git commit --amend`, nyní 2b2aa00) – komentář
  a zpráva už neříkají „formát je odhad“; DTB je bajtově shodný, image netřeba
  přestavovat. pmaports eab4cc4 (kconfig check znovu OK). uniLoader: komentář v boardu.
- FINDINGS.md kap. 10 = souhrn ověřeného na hardwaru.
- **Další krok: zopakovat E3 a čekat aspoň 3 minuty** (kopírování je pomalé).

### 2026-11-10 – E3 podruhé (sledováno přes 3 minuty): zůstalo logo

- `fastboot boot out/troika-test-E3.img` → `OKAY`; **po celé 3 minuty jen logo**,
  žádný text uniLoaderu. Telefon se sám nerestartoval a na USB se neobjevil.
- Časová osa z logu počítače (`journalctl -k`, USB události):
  test A 16:21:40 → Android až 16:34:54 (ruční restart; A tedy taky **visel**, USB
  fastbootu se při `boot` odpojilo, takže bootloader fastboot opustil – jen
  nepřekreslil displej); kalibrace 16:36:29 → Android 16:37:51 (82 s);
  E1 16:45:49 → 16:46:05; E3 16:48:20 → 16:57:53; D 16:58:39 → 17:07:50;
  E3 podruhé 17:12:03 → 17:17:13.
- Rozpor: diagnostický uniLoader (D) kreslí, běžný (E3) ne, přitom cesta kódem
  až po první text je stejná (ověřeno: data boardu v binárce jsou správně –
  `motorola-troika`, simplefb, `0xec000000`; `_reloc_size` je násobek 16;
  zásobníkové rámce jsou malé).
- Hypotézy:
  - T: po skoku ještě něco překreslí displej logem (v D byla před purpurovou
    krátce černá obrazovka, kterou můj kód nekreslí). Běžný uniLoader vypíše text
    během zlomku sekundy a logo ho přepíše; diagnostika kreslila desítky sekund.
  - K: text se mihne, kernel hned po startu způsobí reset a bootloader zůstane
    stát na logu.
- Další: krokovací uniLoader (test F) – po textu čeká 20 s s odpočtem, pak teprve
  kopíruje a spouští kernel.

- Připraven test F: `uniLoader-src` 295077e (`board-troika-step.c`,
  `troika_step_defconfig`), `out/troika-test-F.img` (krokovací uniLoader + původní
  ramdisk + původní DTB), ověřeno v `build/verify/F`. Postup v TESTING.md kap. 4d.

### 2026-11-10 – test F (krokovací): celou dobu jen logo

- `fastboot boot out/troika-test-F.img` → `OKAY`; **3 minuty jen logo**, žádný text,
  ani řádky odpočtu vypisované později. Hypotézy T (jednorázové překreslení) i K
  (text se mihne, kernel resetuje) tím padají: běžná cesta nenakreslí vůbec nic.
- Jediný uniLoader, který kdy na troice kreslil, je diagnostický (D). Rozdíl v
  rozložení binárek (`nm`):

  | varianta | `__bss_end` | `_stack_end` | `image_size` v hlavičce | výsledek |
  |---|---|---|---|---|
  | běžná (E3) | `0x8a210e88` | `0x8a212000` | `0x2211f80` | logo |
  | krokovací (F) | `0x8a210ef8` | `0x8a212000` | `0x2211f80` | logo |
  | diagnostická (D) | `0x8a211038` | `0x8a213000` | `0x2212f80` | **kreslí** |

  U D přeteklo BSS přes hranici stránky, takže linker skript (`.stack ALIGN(4096)`
  + 4096) posunul zásobník o stránku výš: D má mezi koncem BSS a zásobníkem ~4 KiB
  rezervu navíc a o stránku větší `image_size`. Těsně pod koncem BSS leží `fb_info`
  (ukazatel na framebuffer). Jestli jde o přetečení zásobníku, citlivost bootloaderu
  na `image_size`, nebo o `early_init` diagnostiky (výplň + čtení DMA registrů),
  nevím – statická analýza rámců přetečení nepotvrzuje (největší rámec 304 B).
- Další: H = krokovací varianta se stejným rozložením jako D (jen výplň v BSS),
  G = diagnostika, která po odpočtu spustí kernel.

- Připraveny testy H a G: `uniLoader-src` c48944c (`MOTOROLA_TROIKA_STEP_PAD`,
  `MOTOROLA_TROIKA_DIAG_BOOT`, defconfigy `troika_steppad`, `troika_diagboot`).
  Rozložení ověřeno přes `nm`: obě mají `_stack_end 0x8a213000` a `image_size
  0x2212f80` jako D. Image `out/troika-test-{H,G}.img` ověřeny v `build/verify/`.
  Postup v TESTING.md kap. 4e. Pořadí: H, při neúspěchu G, při neúspěchu znovu D.

### 2026-11-10 (večer) – test H: jen logo

- `fastboot boot out/troika-test-H.img` → `OKAY`; 3 minuty jen logo. Samotné
  rozložení paměti (zásobník o stránku výš, větší `image_size`) tedy **nerozhoduje**.
- Zbývá rozdíl v `early_init` diagnostiky (výplň 64 MiB, čtení DMA registrů), nebo
  úspěch D nebyl opakovatelný. Rozhodne test G (diagnostika + kernel), případně
  opakování D.
- Nová pracovní hypotéza (neověřená): displej po skoku z bootloaderu není vždy
  „živý“. DECON může být ve stavu, kdy pošle snímek na panel jen na vyžádání
  (trigger), a panel jinak drží poslední obraz (logo). uniLoader na jiných Exynos
  zařízeních proto v `early_init` zapisuje `HW_SW_TRIG_CONTROL` (DECON + 0x70)
  = 0x1281 s komentářem „Allow framebuffer to be written to“; pro 9610 má stejný
  zápis v `soc/exynos/exynos9610.c`, ale ta funkce se po refaktoringu nevolá.
  V testu D byla před purpurovou krátce černá, což by odpovídalo tomu, že se
  displej „rozběhl“ až chvíli po skoku. Zápis do registru displeje jsem zatím
  nedělal – je to zápis do hardwaru a chci k němu souhlas uživatele.

### 2026-11-10 23:10 – test G a OPRAVA ADRESY FRAMEBUFFERU

- `fastboot boot out/troika-test-G.img` → kreslil jako D; po odpočtu `STEP 2`,
  `trying to open fdt...`, `adding linux,initrd-start...`, `adding linux,initrd-end...`,
  `Booting kernel...` a dál nic, text zůstal stát.
- Ostrá fotka: **`TROIKA DIAG: console at 0xed000000`, `G0 ... addr=ed000000`.**
  V testu D jsem z rozmazané fotky přečetl `ec000000` – **moje chyba**. Skutečný
  zobrazovaný buffer je na `0xed000000` (16 MiB uvnitř `bootloader_fb`).
- Vysvětluje to vše: E3/F/H i kernel v G kreslily na `0xec000000`, kam displej
  nečte. Hypotézy o zásobníku, rozložení, triggeru DECON byly zbytečné.
- Opraveno a přestavěno:
  - kernel: commit DTS troiky upraven (amend → 5adaa33): `framebuffer@ed000000`
    s `reg = <0x0 0xed000000 (1080*2520*4)>`; rezervace `0xec000000`/64 MiB zůstává.
    `dtbs_check` W=1 čistý. `Image` se nepřestavoval (mění se jen DTB).
  - uniLoader 1569de4: `board-troika.c`, `board-troika-step.c` → `0xed000000`.
  - pmaports 8904e95: aktualizovaný patch 0017 a součty.
  - `out/troika-test-K.img` = běžný uniLoader + kernel + opravený DTB (+ původní
    ramdisk a DTB v boot image). Ověřeno: adresa v datech boardu `0xed000000`,
    vložený DTB obsahuje `framebuffer@ed000000`, obsah image v `build/verify/K`.
  - Ostatní image v `out/` se přestavěly taky a mají už opravenou adresu.
- **Další krok: `fastboot boot out/troika-test-K.img`** – očekávání: logo → text
  uniLoaderu → `Booting kernel...` → konzole kernelu → rámeček testu → tlačítka.

### 2026-11-10 ~23:25 – test K: ÚSPĚCH, první boot mainline kernelu

`fastboot boot out/troika-test-K.img` → logo → uniLoader → kernel → testovací
initramfs. Fotka: `device-info/photos/test-K-mainline-boot.png`.

Přečteno z fotky (zvětšený výřez):
- log kernelu: `[drm] Initialized simpledrm 1.0.0 for ed000000.framebuffer`,
  `Console: switching to colour frame buffer device 67x78`,
  PMU `armv8_cortex_a53` i `armv8_cortex_a73` „7 counters available“,
  `input: gpio-keys as /devices/platform/gpio-keys/input/input0`,
  `Run /init as init process`
- `kernel: 7.3.3-troika-gd6ace9a1fea0`
- `model: Motorola One Action`
- `soc_id: EXYNOS9610 rev 2` (chipid driver v mainline čip poznal)
- `cpu: 8 online` (všech 8 jader přes PSCI)
- `MemTotal: 3781496 kB` (třetí číslice na fotce není úplně jistá; řádově sedí:
  4027 MiB − 237 MiB rezervací − kernel a struktury)
- `fb: 1080,2520 32 bpp`
- `cmdline: console=tty0 fbcon=font:TER16x32` (z CONFIG_CMDLINE)
- barevný test: červená, zelená, modrá ve správném pořadí
- vstup: `gpio-keys`, `Handlers=kbd event0`
- tlačítka: `code=115 STISK/UVOLNENI`, `code=114 STISK/UVOLNENI`, `code=116
  STISK/UVOLNENI` – všechna tři, správné kódy, polarita v pořádku
  (před řádkem 114 je `^[[26~` – ozvěna klávesy z VT konzole, neškodné)

Cíl „minimální boot: RAM, framebuffer přes SimpleFB, tlačítka“ je splněn.
USB síť ne (v mainline chybí PHY; mimo rozsah).

Co zbývá / další možné kroky:
1. Zopakovat A, B, C s opravenou adresou (image v `out/` už ji mají) – řeknou,
   jestli bootloader snese malý ramdisk (A), mainline DTB ve „second“ (B) a boot
   bez uniLoaderu (C). Od toho se odvíjí podoba pmaports balíčku.
2. Sestavit balíčky přes pmbootstrap (uživatel, sudo); zapojit uniLoader do balíčku.
3. Restart z kernelu (PSCI reset není; jen přes PMU syscon-reboot – zápis do PMU,
   chce souhlas).
4. Clock driver v3 + opravy na samostatné větvi; pak UART, UFS/SD, USB.
5. Dát vědět upstreamu (Chimac): překlepy `samsung.pin-*`, `#interrupt-cells`,
   adresa fb na troice, mrtvý `soc_init()` v uniLoaderu.

### 2026-11-10 23:30 – příprava testů A/B/C (na přání uživatele před balíčkem)

- `test-initramfs/init` teď po připojení `/dev` přesměruje výstup na `/dev/tty0`,
  aby byl rámeček vidět i s `console=null` od bootloaderu (týká se testu C).
- `troika-test-B.img` má nově původní ramdisk (mění se jen DTB ve „second“),
  aby každý z A/B/C měnil jedinou věc proti K. Obsah všech čtyř ověřen v `build/verify/`.
- Všechny image přestavěny (i K – liší se jen tím jedním řádkem v initu).
- Postup a tabulka důsledků: TESTING.md kap. 5. Čeká se na výsledky A, B, C.

### 2026-11-10 – test A (opravená adresa): prošel

- `fastboot boot out/troika-test-A.img` → logo → uniLoader → kernel → rámeček.
  **Malý neandroidí ramdisk bootloaderu nevadí.** Dřívější závěr „bootloader A
  odmítl kvůli ramdisku“ byl mylný (tehdy se jen kreslilo na špatnou adresu;
  popis „menu zůstalo bez loga“ z prvního pokusu se tentokrát nezopakoval).
- Důsledek: do boot image může jít libovolný initramfs, tedy i pmOS.
- Další: B (mainline DTB ve „second“), pak C (bez uniLoaderu).

### 2026-11-10 – test B: bootloader image odmítl

- `fastboot boot out/troika-test-B.img` → `Sending OKAY`, **`Booting FAILED (remote: '')`**.
  B se od úspěšného K liší jen DTB ve „second“ (mainline místo původního).
- Závěr: bootloader DTB ve „second“ kontroluje a mainline DTB nepřijme – nejspíš
  proto, že na něj nejde aplikovat overlay z oddílu `dtbo` (118 odkazů na labely,
  které mainline DTB nemá). Přesnou příčinu neznám, chybová hláška je prázdná.
  Odmítnutí je okamžité a čisté (telefon zůstává ve fastbootu).
- Důsledky:
  - Test C (stejný DTB ve „second“) má selhat stejně.
  - Bez flashování vlastního `dtbo` (zakázáno) musí být ve „second“ DTB, který
    bootloader přijme, a mainline DTB musí kernelu předat uniLoader. Přesně to dělá
    image K/A.
  - `deviceinfo_bootimg_dtb_second="true"` s `deviceinfo_dtb=exynos9610-troika`
    tedy v balíčku fungovat nebude; balíček je potřeba postavit jinak.

### 2026-11-10 – test C: odmítnut stejně jako B

- `fastboot boot out/troika-test-C.img` → `Booting FAILED (remote: '')`. Potvrzuje,
  že rozhoduje DTB ve „second“, ne to, co je na místě kernelu.

### Shrnutí testů A/B/C (2026-11-10)

| Test | Změna proti K | Výsledek | Závěr |
|---|---|---|---|
| A | malý testovací ramdisk | prošel až k rámečku | ramdisk může být libovolný |
| B | mainline DTB ve „second“ | `FAILED (remote: '')` | bootloader mainline DTB nepřijme |
| C | holý kernel + mainline DTB | `FAILED (remote: '')` | totéž; bez uniLoaderu to bez zásahu do `dtbo` nejde |

Co z toho plyne pro balíček (zatím jen návrh, nic z toho není udělané):
1. „kernel“ v boot image = uniLoader s vloženým `Image` a mainline DTB,
2. „second“ = DTB, který bootloader přijme (původní `exynos9610.dtb`),
3. ramdisk = běžný initramfs pmOS; bootloader ho nahraje na `0x84000000`
   (`device-info/live.dts:3963`), ale jeho velikost a příkazovou řádku zná jen
   bootloader a předává je ve svém DTB (registr x0 při skoku). uniLoader ten DTB
   dnes zahazuje → bude potřeba patch, který z něj převezme `linux,initrd-start/end`
   a `bootargs` (bez `console=null`) do mainline DTB.
4. Otevřené: kde vzít původní DTB pro „second“ v balíčku (sestavit z downstream
   zdrojáků / přibalit binárku / vzít ze zálohy uživatele).

Alternativa obvyklá u pmOS: flashnout prázdný `dtbo` – pak by fungovala cesta C
a stávající balíčky. Je to zápis do telefonu a rozbije LineageOS ve slotu;
podle pravidel se nedělá bez výslovného rozhodnutí uživatele.

## Fáze 3 – balíček s uniLoaderem (zahájeno 2026-11-10 ~23:35 na pokyn uživatele)

Hotovo na počítači:
- **uniLoader** (`uniLoader-src`, větev `troika`, commit 6111dab): start-up uchová
  x0 (DTB od bootloaderu) v `prev_bl_fdt`; nová volba `PREV_BL_FDT_INHERIT`
  (`main/prev-fdt.c`) zkopíruje `linux,initrd-start/end` a připojí `bootargs`
  (bez `console=null`/`console=ram`) do vloženého DTB. `troika_defconfig` teď má
  `RAMDISK_NO_COPY` + `PREV_BL_FDT_INHERIT`; původní chování je `troika_embedded_defconfig`.
- Test na počítači: `build/prev-fdt-test/harness` (stejný zdroják, hostitelský
  libfdt) proti skutečnému `device-info/live.dtb` → initrd `0x84000000–0x84e5562c`
  a bootargs převzaty, `console=null` vypuštěno; bez DTB / s nesmyslem selže čistě.
- Čistá větev `troika-pkg` v `uniLoader-src` (2 commity nad upstream 30442c6);
  z ní jsou patche `uniloader-000{1,2}-*.diff` v balíčku kernelu. Ruční sestavení
  z upstream tarballu + těchto patchů prošlo (`build/uniloader-pkgtest`).
- **pmaports** 853fa47: kernel APKBUILD staví uniLoader a instaluje ho do
  `/usr/share/u-boot/motorola-troika/uniLoader`; device balíček nese původní DTB
  (`/boot/dtbs/vendor/exynos9610-troika-vendor.dtb`) a `deviceinfo` s
  `deviceinfo_uboot_boardname`, `deviceinfo_bootimg_override_payload="uniLoader"`,
  `deviceinfo_bootimg_dtb_second="true"`. Mechanismus ověřen ve zdrojácích
  boot-deploy 0.25.0 a mkinitfs 2.12.2 (`copyUbootFiles`). `kconfig check` OK.
- `tools/make-test-bootimg.sh`: K/A/E3 používají `uniLoader-troika-embedded`;
  nový **`out/troika-test-P.img`** = balíčkový uniLoader (zástupný ramdisk 4 KiB) +
  testovací initramfs jako ramdisk boot image + původní DTB + cmdline
  `console=tty0 fbcon=font:TER16x32 pmos_test=1`.

Čeká se na: `fastboot boot out/troika-test-P.img` (TESTING.md kap. 5a).

Neověřeno: build balíčků přes pmbootstrap; image z `pmbootstrap install`;
jestli MBM opravdu předává DTB v x0 (předpoklad podle arm64 boot protokolu).

Známé slabiny návrhu:
- původní DTB je v balíčku jako binárka ze zálohy (GPL, zdroj znám, ale není
  sestavený ze zdrojáků) – pro upstream pmaports by to chtělo vyřešit,
- převzatá příkazová řádka obsahuje i vendor argumenty z původního DTB
  (`swiotlb=512`, `firmware_class.path=/vendor/firmware`, `androidboot.*` …);
  mainline je ignoruje nebo předá initu, ale čisté to není,
- po aktualizaci LineageOS se může změnit `dtbo` v telefonu a původní DTB z
  balíčku na něj nemusí sedět.

### 2026-11-11 ~00:00 – test P: kernel běží, initramfs se nenačetl

- `fastboot boot out/troika-test-P.img` → `OKAY`; uniLoader i kernel běží, rámeček ne.
  Fotka `device-info/photos/test-P-initramfs-failed.png` (zvětšený výřez):
  `[0.199479] Initramfs unpacking failed: read error`, `Freeing initrd memory: 4K`,
  poslední řádek `input: gpio-keys as …/input0` (0.42 s), dál nic.
- `4K` = zástupná velikost ramdisku z buildu → **převzetí `linux,initrd-*` z DTB
  bootloaderu neproběhlo**. Co uniLoader vypsal, nevím (text zmizel rychle).
- Proč log končí u 0.42 s: v testu K následovaly další řádky až v 0.98 s a hned po
  nich by tady přišla panika „Unable to mount root fs“; simpledrm vykresluje se
  zpožděním, takže poslední řádky před panikou se nejspíš nestihly zobrazit. Domněnka.
- Hypotéza k převzetí: uniLoader se hned po startu kopíruje na `0x88000000–0x8a171000`.
  Pokud bootloader ukládá svůj DTB do tohoto rozsahu (u Exynos LK bývá `0x8a000000` –
  to je vzpomínka, ne doložený údaj), relokace ho přepíše dřív, než ho přečteme.
  Druhá možnost: x0 při skoku na DTB neukazuje.
- Další: vypsat hodnotu x0 a hlavičku DTB na displej s pauzou (krokovací varianta)
  a přesunout uniLoader a kernel na `0x94000000` / `0x98000000`.

- Připraven test P2: `uniLoader-src` e1f452a (výpis x0 a hlavičky v krokovací
  variantě, `previous device tree at …` v `prev_fdt_inherit`, balíčková konfigurace
  na `TEXT_BASE 0x94000000` / `PAYLOAD_ENTRY 0x98000000`, `troika_stepinherit_defconfig`).
  `out/troika-test-P2.img` ověřen v `build/verify/P2`. `out/troika-test-P.img` je
  přestavěný s novými adresami (bez pauzy).
- **Pozor:** balíček v `pmaports/` (853fa47) má ještě staré patche uniLoaderu
  (staré adresy, bez výpisu). Aktualizovat větev `troika-pkg` a balíček až po
  výsledku P2.

### 2026-11-11 00:09 – test P2: DTB bootloaderu nalezen, pak reset

- Fotka `device-info/photos/test-P2-uniloader-x0.png` (zvětšený výřez):
  `x0 from bootloader: 0x8a000000`, `at x0: magic=d00dfeed totalsize=350eb`,
  po odpočtu `previous device tree at 0x8a000000`, `inherited linux,initrd-start`
  a (podle uživatele) začátek dalšího řádku `inherited linux,` – pak **telefon
  sám restartoval** do LineageOS.
- Potvrzeno: bootloader předává svůj DTB v x0 a ukládá ho na **`0x8a000000`**
  (217 323 B). Staré umístění uniLoaderu (`0x88000000–0x8a171000`) ho přepisovalo;
  na `0x94000000` zůstává celý.
- Příčina resetu (velmi pravděpodobná, ne změřená): v `main/prev-fdt.c` jsem pro
  skládání příkazové řádky použil `memcpy`. V uniLoaderu je to asemblerová rutina,
  která dělá nezarovnané přístupy; s vypnutou MMU je taková operace chybou
  (alignment fault). uniLoader proto v libfdt všude používá vlastní
  `__optimized_memcpy/memmove` a asm `memcpy` volá jen se zarovnanými adresami
  (kopie kernelu). Můj test na počítači to nezachytil – běžel s glibc.
  Řádek „nedopsaný po slovo linux“ je zpoždění zobrazení: procesor už byl dál.

- Oprava: `uniLoader-src` 89e01bc – `append_args()` kopíruje bajt po bajtu místo
  `memcpy`. Ověřeno: hostitelský test přeložený s `-Dmemcpy=FORBIDDEN…` (objekt
  neodkazuje na žádnou `mem*` funkci) dává stejný výsledek; v přeloženém
  uniLoaderu volají asm `memcpy` už jen `arch_load_kernel` a `strncpy`.
- Větev `troika-pkg` sjednocena s testovaným kódem (52f3193: nové adresy, výpis
  ukazatele, bajtová kopie); balíček pmaports 10f8d94 má nové patche a součty;
  ruční sestavení z upstream tarballu + patchů znovu prošlo.
- `out/troika-test-P2.img` a `out/troika-test-P.img` přestavěny (00:18). Čeká se
  na opakování P2.

## Repozitáře na GitHubu (2026-11-11, všechny soukromé)

- `letress/nura-troika` – tento adresář: dokumenty, `tools/`, `test-initramfs/`,
  `patches-src/`, `kernel-patches/` (export větve `troika-clk`, 60 patchů: DT,
  hodiny, USB, UFS; po změně kernelu smazat a znovu
  `git -C linux format-patch -o ../kernel-patches v7.3.3..troika-clk`),
  `kernel-patches-debug/` (3 ladicí commity větve `troika-clk-test`, jen pro
  testovací image, ne pro balíček).
  Remote `origin`, větev `main`.
- `letress/uniLoader-troika` – `uniLoader-src/`, větev `troika`, remote `letress`.
- `letress/pmaports-troika` – `pmaports/`, větev `motorola-troika-mainline`, remote `letress`.

Nepushuje se (`.gitignore`): `device-info/` (identifikátory telefonu), `refs/`,
`dl/`, `toolchain/`, `build/`, `out/`, strom `linux/` (jen jako patche).
`~/troika-backup` je mimo projekt a na GitHub nepatří.

### 2026-11-11 ~00:25 – opakovaný test P2: ÚSPĚCH

`fastboot boot out/troika-test-P2.img` (uniLoader bez vloženého ramdisku, na
`0x94000000`, kernel na `0x98000000`, initramfs jako ramdisk boot image, původní
DTB ve „second“) → uniLoader → kernel → rámeček testu, tlačítka reagují.

Fotka přišla jen vložená do chatu (na disku není), takže jsem ji nemohl zvětšit.
Co z ní čtu:
- jistě: rámeček testu je vidět, tedy **initramfs z boot image se načetl** – a
  protože uniLoader v tomhle buildu má jen 4KiB zástupný ramdisk, znamená to, že
  **převzetí `linux,initrd-start/end` z DTB bootloaderu funguje**;
- `clk: Not disabling unused clocks` v logu kernelu → kernel dostal
  `clk_ignore_unused`, tedy **převzetí bootargs funguje**;
- `cmdline:` začíná `swiotlb=512 rootwait ro init=/init clk_ignore_unused …`
  (bez `console=null` – filtr zabral) a v pátém řádku je
  `… revision=0x46 fbcon=font:TER16x32 pmos_test=1 skip_initramfs root=PARTUUID=…`;
- `console=tty0` z cmdline boot image tam **nevidím** – buď ho bootloader
  odstraňuje (stejně jako si sám dosazuje `console=null`), nebo ho v malém obrázku
  přehlížím. Neověřeno; konzole na displeji funguje i bez něj.
- `kernel: 7.3.3-troika-gd6ace9a1fea0`, `soc_id: EXYNOS9610 rev 2`, `cpu: 8 online`,
  `MemTotal` ≈ 3,77 GB, `fb: 1080,2520 32 bpp`, barvy správně, tlačítka 115/114/116.

Stav balíčku: `pmaports` 10f8d94 obsahuje přesně tento kód uniLoaderu (větev
`troika-pkg` = `troika` bez krokovací desky). Varianta bez pauzy je
`out/troika-test-P.img` (přestavěná 00:18, na telefonu neběžela).

Zbývá:
1. Sestavit balíčky přes pmbootstrap (uživatel, sudo) a vyzkoušet image z
   `pmbootstrap install` přes `fastboot boot` – teprve to ověří APKBUILD, deviceinfo
   a mechanismus boot-deploy/mkinitfs.
2. Ověřit, jestli bootloader z cmdline boot image opravdu maže `console=`.
3. Dál: restart z kernelu, clocky, úložiště, USB.

### 2026-11-11 00:35 – cmdline z testu P2 potvrzena ze zvětšené fotky

Uživatel uložil původní fotku do `photos/20261111_002654.heic` (3000×4000; obsahuje
sériové číslo a MAC adresy – nesdílet). Převedeno přes ffmpeg, čteno ze zvětšených
výřezů prvních pěti řádků:

```
cmdline: swiotlb=512 rootwait ro init=/init clk_ignore_unused bcm_setup=0xffffff80f8e00000
androidboot.hardware=exynos9610 androidboot.debug_level=0x4948
firmware_class.path=/vendor/firmware ecd_setup=disable reserve-fimc=0xffffff80f9fe0000
pmic_info=0x3 ccic_info=0x1 epx_activate=true revision=0x46 fbcon=font:TER16x32
pmos_test=1 skip_initramfs root=PARTUUID=… androidboot.…
```

- **`pmos_test=1` tam je** → příkazová řádka z boot image se ke kernelu dostane.
- **`console=tty0` tam není**, ačkoli v boot image bylo hned před `fbcon=…`.
  Bootloader tedy z cmdline boot image argument `console=` odstraňuje. Konzole na
  displeji funguje i tak (tty0 je výchozí).
- Bootloader sám přidává `skip_initramfs root=PARTUUID=…` a řadu `androidboot.*`.

### 2026-11-11 00:36–02:03 – první sestavení `linux-motorola-troika-mainline` přes pmbootstrap

Uživatel spustil `pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64
linux-motorola-troika-mainline`. Balíček **vznikl** (1 h 26 min):
`~/.local/var/pmbootstrap/packages/edge/aarch64/linux-motorola-troika-mainline-7.3.3-r1.apk`
(37,7 MB), index podepsán. Chyba na konci výpisu (`sudo losetup --json --list`)
je jen úklid po sestavení: sudo čekalo 5 minut na heslo a vzdalo to. Na balíček
nemá vliv; další příkaz pmbootstrapu si uklidí sám.

Zkontrolováno v balíčku:

- `usr/share/u-boot/motorola-troika/uniLoader` je tam (27 540 216 B = Image + ~78 kB),
  obsahuje řetězce z patche pro převzetí initrd/bootargs. Krok s uniLoaderem
  v APKBUILD tedy pod pmbootstrapem funguje.
- `boot/dtbs/exynos/exynos9610-troika.dtb` je bajt po bajtu stejný jako ten z ručního
  sestavení (ten, co prošel testy K a P2).
- 932 modulů, `boot/vmlinuz` 27,5 MB.

Nalezené chyby balíčku (moje, opraveno v pmaports commitu 745530f71, pkgrel 2):

1. **V `makedepends` chyběl `pahole`.** Bez něj oldconfig v chrootu potichu vypnul
   `DEBUG_INFO_BTF` a `DEBUG_INFO_BTF_MODULES` (proto je Image o 7 MB menší než
   ručně sestavený). Sestavený r1 tak neodpovídá konfiguraci, která prochází
   `pmbootstrap kconfig check`. Na boot to vliv nemá.
2. **Konfigurace byla vygenerovaná gcc 16.2**, chroot má Alpine gcc 15.2 → oldconfig
   se ptal na volby, které existují jen tam (`GCC_PLUGINS`, `RANDSTRUCT_*`,
   `KSTACK_ERASE`, `DEBUG_INFO_COMPRESSED_ZSTD`) a bral výchozí odpovědi. Tyto
   odpovědi jsou teď přímo v konfiguraci balíčku. `kconfig check` dál prochází.

`tools/update-kernel-pkg.sh` už konfiguraci z `build/linux` nekopíruje sám
(přepsal by ji verzí pro gcc 16.2); bere ji jen jako volitelný argument.

Neověřeno: sestavení r2 (s pahole), `device-motorola-troika`, image z `pmbootstrap install`.

### 2026-11-11 11:00–12:35 – kernel r2 a `device-motorola-troika` sestaveny

- `linux-motorola-troika-mainline-7.3.3-r2.apk` (47,6 MB), 1 h 30 min. Mezipaměť
  ccache nepomohla, můj odhad „výrazně rychleji“ byl špatně.
  - oldconfig se už na nic neptal (0 dotazů v logu druhého běhu),
  - `boot/config` v balíčku se shoduje s konfigurací v pmaports (BTF zapnuté,
    `BTF [M]` u modulů v logu),
  - `exynos9610-troika.dtb` je dál bajt po bajtu shodný s otestovaným,
  - `vmlinuz` 34 478 592 B, uniLoader 34 556 664 B.
- `device-motorola-troika-1-r1.apk` (30 kB), 2 minuty, skončilo `DONE!`.
  Obsahuje `usr/share/deviceinfo/device-motorola-troika`, `etc/machine-info`
  a `boot/dtbs/vendor/exynos9610-troika-vendor.dtb` (shodný s `second` ze zálohy).
- Nastaveno `pmbootstrap config device motorola-troika` (dřív qemu-amd64), aby šel
  spustit `pmbootstrap install`. Nic dalšího v konfiguraci jsem neměnil.

Neověřeno: `pmbootstrap install`, `pmbootstrap export`, boot.img z balíčků na telefonu.


### 2026-11-11 13:05–13:15 – `pmbootstrap install` a `export` prošly, připraven test PM

- `install` (console UI, systemd) skončil `DONE!` za 7 minut; rootfs image
  `motorola-troika.img` 1422 MB. `export` vytvořil odkazy v `/tmp/postmarketOS-export`.
  Hláška „chroot not initialized! This is a bug!“ při `export` je chyba pmbootstrapu,
  export i tak doběhl.
- V logu mkinitfs: `initramfs: replace kernel with file …/uniLoader` a
  `Installing: /boot/exynos9610-troika-vendor.dtb` → `deviceinfo_uboot_boardname`,
  `deviceinfo_bootimg_override_payload` i `deviceinfo_bootimg_dtb_second` fungují
  tak, jak jsem je vyčetl ze zdrojáků.
- Rozbor `boot.img` (47 532 032 B, zkopírováno do `out/troika-test-PM.img`):
  hlavička v1, stránka 2048, kernel @0x10008000 = uniLoader shodný s balíčkem r2
  (sha256 14724dc5…), second = původní DTB (1e5671b9…, shodný s P2 a zálohou),
  ramdisk 12 836 090 B zstd, cmdline
  `console=tty0  pmos_boot_uuid=… pmos_root_uuid=… pmos_rootfsopts=defaults`.
- Rozdíly proti P2: `os_version` = 0 (boot-deploy ho nenastavuje), chybí
  `fbcon=font:TER16x32` (CONFIG_CMDLINE se použije jen při prázdných bootargs),
  větší ramdisk (končí na 0x84c3e000, pod DTB bootloaderu na 0x8a000000).
- Initramfs pmOS (mkinitfs 3.12.5): plymouth (`drm.so`, `frame-buffer.so`, téma
  `pmos-spin`), po 30 s bez kořenového oddílu `root partition not found!` a
  `fail_halt_boot` (smyčka). To je očekávaný konec, dokud není UFS/SD.

### 2026-11-11 ~13:25 – test PM (`out/troika-test-PM.img`): ÚSPĚCH, initramfs pmOS běží

`fastboot boot` → `Booting OKAY`. Bootloaderu tedy `os_version` = 0 v hlavičce nevadí.
Fotka: `device-info/photos/test-PM-pmos-debug-shell.png` (čteno ze zvětšených výřezů;
rozlišení 638×850, časové značky jsou čitelné jen přibližně).

- `input: gpio-keys as /devices/platform/gpio-keys/input/input0`
- `Freeing initrd memory: 12532K` → celý initramfs z boot image (12,8 MB) se načetl
  přes převzetí `linux,initrd-*` v uniLoaderu.
- `[pmOS-rd]: PMOS STAGE 1`, `initramfs version: 3.12.5-r0`, `PMOS STAGE 2`
- `Trying to mount subpartitions for 10 seconds...` → `ERROR: failed to mount subpartitions!`
- kolem 43. sekundy (po 30 s čekání na kořenový oddíl): `Creating logs disk`,
  `Making logs available via mass storage`, `Couldn't write new UDC` (USB řadič
  v mainline není), `Entering debug shell`, spuštěn `buffyboard`
  (`input: buffyboard as /devices/virtual/input/input1`), `telnetd: bind: Address not available`.
- Obrazovka debug shellu: `Device: Motorola One Action (motorola-troika)`,
  `Kernel: 7.3.3-troika`, `OS ver: edge`, `initrd: 3.12.5-r0`, výzva `~ #`
  a klávesnice buffyboard na displeji.

Závěr: `deviceinfo`, balíček kernelu s uniLoaderem, mkinitfs i boot-deploy dávají
dohromady image, který na telefonu bootuje. Konec v debug shellu je očekávaný
(žádné úložiště). Klávesnice na displeji nejde použít – dotyková vrstva nemá
v mainline DTS uzel ani napájení. Jestli se před debug shellem ukázala grafická
obrazovka plymouth, nevím (uživatel popsal až konečný stav).

## Fáze 4 – ovladač hodin (zahájeno 2026-11-11 ~13:30 na pokyn uživatele)

### 2026-11-11 13:30–14:00 – větev `troika-clk`, kontrola a opravy v3

- Patche v3 staženy z lore přes vestavěný prohlížeč do `refs/lore/clk-v3/{1..5}.mbox`.
  `git am` 1–4 na `troika-mainline` bez konfliktu, 5/5 (MAINTAINERS) přeskočen.
- Nový nástroj `tools/check-clk-driver.py` (porovnání s downstream cmucal). Na v3:
  REG 0 skutečných, FIELD 3, DUP 6, PARENT 57, ORPHAN 54, NREGS 4, MISSING 16.
  Po opravách zůstává jen to, co je v FINDINGS 11.3/11.4 vysvětleno.
- 24 commitů oprav + `arm64: dts: exynos9610: Add clock controllers` (9 CMU: top,
  apm, cmgp, core, cpucl0, peri, fsys, usb, dispaud; reg 0x10000). CAM/G2D/G3D
  vynechány kvůli napájecím doménám. Podrobnosti a zdroje: FINDINGS.md kap. 11.
- Kontroly: dtbs_check, dt_binding_check, W=1, checkpatch – viz FINDINGS 11.6.
- Větev `troika-clk-test` = `troika-clk` + 1 ladicí commit (výpis registrů OPTION
  před/po zápisu a stavu napájecích domén; jen čtení). Není pro balíček.
- `build/linux` je teď sestavený z `troika-clk-test` (ne z `troika-mainline`).
  Image v `out/` z dřívějška (K, P, P2, PM) zůstaly beze změny; `tools/make-test-bootimg.sh`
  teď NESPOUŠTĚT bez přepnutí větve a přestavění, přepsal by je kernelem s hodinami.
- `test-initramfs/init` vypisuje navíc sekci „HODINY“ (počet hodin, sirotci,
  13 frekvencí, řádky CLKDBG), pokud kernel hodiny má.
- `tools/make-clk-test-bootimg.sh` → `out/troika-test-CLK1.img` (uniLoader jako
  v balíčku, původní DTB ve second, cmdline s `clk_ignore_unused`).

Poznámka uživatele k testu PM: klávesnice na displeji nereaguje na dotyk. Očekávané –
dotykový řadič nemá v mainline DTS uzel; potřebuje I2C (hodiny) a napájení
(regulátory, které podle pravidel zatím nepovolujeme).

### 2026-11-11 ~14:10 – test CLK1: ÚSPĚCH

`fastboot boot out/troika-test-CLK1.img` → rámeček testu se sekcí HODINY.
Hodnoty a jejich kontrola: FINDINGS.md 11.7. Stručně: 566 hodin, 0 sirotků;
shared0 1599 MHz, shared1 1332,5 MHz; OPTION `0x30000000 -> 0x70000000` u všech
devíti CMU; všech sedm napájecích domén zapnutých (`…f`). Displej, konzole
i tlačítka fungují dál. Fotka: `device-info/photos/test-CLK1-clocks.png`
(obsahuje cmdline se sériovým číslem a MAC – nesdílet).

### 2026-11-11 ~14:30 – průzkum USB (na pokyn uživatele „USB pro SSH“)

Jen čtení zdrojů, nic se nestavělo. Výsledek: FINDINGS.md kap. 12. Mainline má
ovladač pro stejnou rodinu PHY (Exynos 850); pro 9610 chybí varianta se třemi
rozdíly (bity v PMU, bity resetu, bez zápisu SSPPLLCTL), uzel PMU a uzly v DT.
**Čeká na souhlas uživatele se zápisem do registru PMU 0x11860704** (bity 0, 1, 16).

### 2026-11-11 ~14:35 – uživatel povolil zápis do PMU 0x11860704; USB implementováno

- Souhlas: „Ano, povoluji“ na otázku, zda smím při zapínání USB PHY zapisovat do
  registru PMU 0x11860704 (bity 0, 1, 16). Jiné zápisy do PMU povolené nejsou.
- 7 commitů na `troika-clk` (FINDINGS.md 12.6). Kontroly: dtbs_check hlásí jen
  chybějící `vdd33-supply` (záměrně), ostatní čisté.
- `troika-clk-test` = `troika-clk` + 2 ladicí commity (CLKDBG, USBDBG – výpis PMU
  0x704 před/po a registrů PHY před/po inicializaci).
- `test-initramfs/init`: nová sekce USB (gadget NCM přes configfs, 172.16.42.1,
  DHCP a telnet z binárek initramfs pmOS uložených v `dl/pmos-bits/`), cmdline se
  vypisuje zkráceně (bez sériového čísla), podrobnosti hodin jen při `pmos_test=clk…`.
- `tools/make-clk-test-bootimg.sh [JMÉNO]` → `out/troika-test-USB1.img`.

### 2026-11-11 14:57 – test USB1: ÚSPĚCH

Počítač vidí telefon jako síťové zařízení (cdc_ncm, 18d1:d001), DHCP přidělilo
172.16.42.2, ping 3/3, shell přes telnet funguje. Stav gadgetu `configured`,
`high-speed`. Registry před/po: FINDINGS.md 12.7. Log kernelu stažený po síti:
`device-info/dmesg-mainline-USB1.txt` (obsahuje cmdline se sériovým číslem).
Nový nástroj `tools/phone-sh.py '<příkaz>'` – spustí příkaz v shellu telefonu.

### 2026-11-11 ~15:05 – hodiny a USB do balíčku, příprava netbootu (na pokyn „Ano, připrav to“)

- `tools/update-kernel-pkg.sh` bere větev `troika-clk` (jde změnit přes `KBRANCH`)
  a přepisuje i seznam patchů v `source=`. Ladicí commity z `troika-clk-test`
  v balíčku nejsou.
- pmaports commit 224302493: kernel pkgrel 3 (52 patchů), zařízení pkgrel 2
  (`deviceinfo_kernel_cmdline="console=tty0 clk_ignore_unused"`).
- Ověřeno: 52 patchů aplikováno přes `patch -p1` na čistý strom v7.3.3, výsledek
  je bez rozdílu proti větvi `troika-clk`; `pmbootstrap kconfig check` prochází;
  součty v APKBUILD odpovídají souborům. `pmbootstrap checksum --verify` jsem
  spustit nemohl (chce sudo).
- `pmbootstrap config extra_packages postmarketos-mkinitfs-hook-netboot`
  (dřív `none`). Vrácení: `pmbootstrap config extra_packages none`.
- Netboot dohledán: hook `main/postmarketos-mkinitfs-hook-netboot` (NBD na
  172.16.42.2:9999), server `pmbootstrap netboot serve`. Firewall na počítači
  (zóna FedoraWorkstation) porty nad 1024 pouští. Kernel má `BLK_DEV_NBD=m`.
- Kvůli místu na disku (12 GB volných) jsem vyčistil objektové soubory v
  `build/linux` (`make clean`, uvolnilo 4 GB, `.config` zůstal). Před dalším
  místním testem je potřeba kernel znovu sestavit (~20 min). Image v `out/` zůstaly.

### 2026-11-11 15:17–15:30 – kernel r3 sestaven přes pmbootstrap

`linux-motorola-troika-mainline-7.3.3-r3.apk` (47,6 MB), sestavení 12 minut
(tentokrát ccache zabrala). Chyba `sudo losetup` na konci je zase jen úklid
(vypršelo čekání na heslo). Zkontrolováno: konfigurace v balíčku = konfigurace
v pmaports, žádné dotazy oldconfigu, DTB má uzly CMU, PMU, USB PHY a dwc3,
uniLoader obsahuje kernel s ovladači hodin a USB a žádné ladicí výpisy,
`nbd.ko` je v balíčku. Zbývá: `device-motorola-troika` r2, `install`, `export`.

### 2026-11-11 16:15–17:25 – zařízení r2, install, export; připraven NET1

`device-motorola-troika-1-r2.apk` sestaven (16:15), `pmbootstrap install` doběhl
(17:19, `DONE!`), `pmbootstrap export` také (17:23; hláška „chroot not
initialized! This is a bug!“ je vada pmbootstrapu, chroot si připojil sám).

Kontrola `/tmp/postmarketOS-export/boot.img` (uloženo jako
`out/troika-test-NET1.img`, sha256 začíná `31fb237f`):

- hlavička v1, os_version 0, stránka 2048; „kernel“ = uniLoader, bajt po bajtu
  stejný jako v balíčku r3; „second“ = původní DTB z balíčku zařízení;
- cmdline `console=tty0 clk_ignore_unused` + `pmos_boot_uuid`, `pmos_root_uuid`,
  `pmos_rootfsopts`;
- kernel 7.3.3-troika #4 (Alpine gcc 15.2), bez ladicích výpisů CLKDBG/USBDBG;
- initramfs obsahuje `hooks/netboot.sh`, `nbd.ko.zst`, `unudhcpd`, busybox
  (nbd-client je applet busyboxu); konfigurace má `USB_CONFIGFS_NCM=y`,
  `USB_DWC3_EXYNOS=y`, `PHY_EXYNOS5_USBDRD=y`, `BLK_DEV_NBD=m`.

`images_netboot/` je prázdný, takže `netboot serve` si zkopíruje čerstvý obraz
(1,4 GB; volných 16 GB). Zbývá na uživateli: TESTING.md 5f kroky 3–5.

### 2026-11-11 17:40–17:52 – test NET1 prošel (netboot + SSH)

Průběh: `fastboot boot out/troika-test-NET1.img` (OKAY) → telefon v initramfs
čekal na NBD (ping na 172.16.42.1 šel, port 22 odmítal, počítač dostal
172.16.42.2) → `pmbootstrap netboot serve` spuštěn až potom (17:49 „Found
postmarketOS device, serving image...“) → `ssh user@172.16.42.1` se přihlásil,
uvítání „Welcome to Nura!“.

Co to dokládá:

- boot image z `pmbootstrap install/export` (kernel r3 s hodinami a USB,
  zařízení r2) bootuje a USB síť funguje i z balíčkového kernelu (Alpine gcc);
- netboot hook funguje: kořenový systém připojený přes NBD po USB;
- USB síť přežije přechod z initramfs do plného systému se systemd, sshd běží;
- nic se nezapsalo na úložiště telefonu.

Nezjištěno: stav služeb v běžícím systému (`systemctl --failed`), dmesg z
tohoto bootu, chování po vytažení kabelu (kořenový systém na něm visí).
Server se na sudo heslo zeptal jen jednou.

### 2026-11-11 18:0x – kontrola běžícího systému (NET1)

Výpis přes SSH (heslo zadal uživatel) → `device-info/net1-status.txt`;
vyhodnocení ve FINDINGS.md kap. 13. Shrnutí: systém `degraded` jen kvůli
`postmarketos-zram-swap.service` (v konfiguraci kernelu chybí `CONFIG_ZRAM`);
varování kernelu jsou stejná jako v USB1; žádná chyba ovladačů hodin ani USB.
Návrh dalšího kroku: `CONFIG_ZRAM=m` do balíčku (pkgrel 4) – čeká na rozhodnutí.

Poznámka: uživatel omylem spustil druhý `netboot serve`; ten se jen opakovaně
nepřipojí na obsazený port, první server a telefon to neovlivnilo.

### 2026-11-11 – zram do balíčku kernelu (pkgrel 4)

Uživatel: „Ano, přidej zram“. pmaports commit cfcf09c10 (jen místně):
`CONFIG_ZRAM=y`, `ZSMALLOC=y` a šest kompresních backendů podle kategorie
`zram` v `kconfigcheck.toml` (služba pmOS používá zstd). Změny v konfiguraci
jsem spočítal jako rozdíl dvou `olddefconfig` (s volbami a bez) a přenesl do
konfigurace balíčku, aby zůstaly odpovědi pro Alpine gcc 15; vedlejší změny:
`842_COMPRESS/DECOMPRESS=y`, `LZ4HC_COMPRESS=y`, `ZSTD_COMPRESS` m→y.
Ověřeno: `pmbootstrap kconfig check --categories zram` prochází, součty
v APKBUILD sedí. Nesestaveno, na telefonu nezkoušeno.

Zbývá na uživateli: TESTING.md kap. 5g (test NET2).

### 2026-11-11 18:02–19:37 – kernel r4 sestaven, install, export; připraven NET2

`linux-motorola-troika-mainline-7.3.3-r4.apk` (47,8 MB), sestavení 1 h 30 min,
skončilo `DONE!`. `install` (19:35) a `export` (19:36) doběhly.

Kontrola boot image (uloženo jako `out/troika-test-NET2.img`): hlavička v1,
os_version 0; uniLoader = balíček r4 (bajt po bajtu); „second“ = stejný původní
DTB jako u NET1; cmdline `console=tty0 clk_ignore_unused` + nová UUID; kernel
7.3.3-troika #5, bez ladicích výpisů, obsahuje ovladač zram; konfigurace
v balíčku = konfigurace v pmaports (`ZRAM=y`, `ZSMALLOC=y`, `ZRAM_BACKEND_ZSTD=y`);
initramfs má `netboot.sh` a `nbd.ko`.

Zbývá na uživateli: TESTING.md 5g kroky 3–4 (`netboot serve --replace`,
`fastboot boot`, ssh) a pak kontrola `systemctl --failed`.

### 2026-11-11 19:39–19:50 – test NET2 prošel (zram)

`netboot serve --replace` (19:37), `fastboot boot out/troika-test-NET2.img`
(OKAY), server 19:39 „Found postmarketOS device“. Výpis přes SSH (heslo zadal
uživatel) → `device-info/net2-status.txt`:

- kernel `7.3.3-troika #5-postmarketOS`; `systemctl is-system-running` = `running`,
  0 spadlých služeb;
- `/dev/zram0` zstd 5,4 GB jako swap (150 % RAM), priorita 300;
- start 6,8 s kernel + 8,9 s userspace (server tentokrát běžel předem);
- varování kernelu beze změny proti NET1 (FINDINGS.md kap. 13).

pmaports commity 745530f71, 224302493, cfcf09c10 jsou pořád jen místně.

### 2026-11-11 – průzkum úložiště (UFS, SD)

Uživatel: „Úložiště UFS/SD“. Jen průzkum, výsledky ve FINDINGS.md kap. 14.
UFS je proveditelné bez regulátorů (VCC drží bootloader), ale chce zápis do
PMU 0x11860724 (izolace PHY) – **čeká na souhlas uživatele** – a port ovladače
(tabulka PHY + varianta v ufs-exynos.c). SD karta potřebuje regulátory PMIC
S2MPU09 přes SPEEDY, pro které mainline nemá ovladač → odloženo.
V kernelu ani pmaports se nic nezměnilo.

### 2026-11-11 – souhlas se zápisem do PMU pro UFS

Uživatel: „Ano, povoluji“ na otázku „Povoluješ zápis do PMU 0x11860724 (bit 0,
izolace UFS PHY), jak to dělá původní kernel?“. Platí jen pro tento registr a
bit. Začíná port ovladače UFS (větev `troika-clk`), test UFS1 poběží
s testovacím initramfs a diskem jen pro čtení.

### 2026-11-11 večer – port ovladače UFS (větev `troika-clk`, +10 commitů, celkem 62 nad v7.3.3)

Viz FINDINGS.md 14.5 (seznam commitů, mapování na původní kernel, odchylky).
Stručně: nová tabulka PHY `phy-exynos9610-ufs.c`, varianta hosta
`exynos9610_ufs_drvs` v `ufs-exynos.c`, tři menší obecné úpravy (linky před
háčkem, `lanes-per-direction`, volitelné `PA_TxTrailingClocks`), obecná úprava
PHY pro méně aktivních linek, uzly v DT, na desce 1 linka.

- `troika-clk-test` = `troika-clk` + 3 DEBUG commity (CLKDBG, USBDBG, nově
  UFSDBG: stav po bootloaderu, PMU 0x724 před/po, PMA, atributy UniPro).
- checkpatch: jen očekávané (chybí Signed-off-by, Co-Authored-By) a dvě
  zarovnání v přesunutém původním kódu.
- Testovací init: nový oddíl `--- UFS ---`, disky se hned přepínají na jen
  pro čtení; USBDBG se vypisuje už jen v testech USB. `make-clk-test-bootimg.sh`
  přidává `scsi_mod.scan=sync`.
- Otevřené: perioda hodin (3 podle původního kernelu vs. 7/8 podle skutečných
  hodin UniPro) – rozhodne výpis z testu UFS1.
- V balíčku pmaports UFS zatím není (až po úspěšném testu).

### 2026-11-11 20:17–20:30 – kernel s UFS sestaven, image UFS1 připraven

- První sestavení spadlo na mé chybě v ladicím commitu (funkce použité před
  definicí); doplněny dopředné deklarace, druhé sestavení bez chyb a bez
  varování z nových souborů.
- `dtbs_check` pro troiku: jen známé `vdd33-supply` u USB; uzly UFS, PHY
  a sysreg procházejí. `dt_binding_check` obou upravených bindingů prochází.
- `tools/make-clk-test-bootimg.sh UFS1` → `out/troika-test-UFS1.img`
  (35 850 240 B). Zkontrolováno: hlavička v1, původní DTB ve „second“, cmdline
  `… clk_ignore_unused scsi_mod.scan=sync pmos_test=ufs1`, kernel obsahuje
  ovladač UFS pro 9610 a výpisy UFSDBG, init má oddíl UFS s `blockdev --setro`;
  v DTB je `ufs@13520000` (okay, 1 linka), `phy@13524000`, sysreg FSYS.
- Na telefonu nezkoušeno. Zbývá na uživateli: TESTING.md 5h.

### 2026-11-11 20:35–20:50 – test UFS1 prošel, úklid větve, image UFS2

`fastboot boot out/troika-test-UFS1.img` (OKAY). Výsledky ve FINDINGS.md 14.6:
Micron 128 GB, `FAST series_B G_3 L_1`, 42 oddílů, hashe čtyř oddílů shodné se
zálohou, čtení 454 MB/s, vše `ro=1`, nic připojeno. Uživatel poslal i fotku
displeje, souhlasí s logem.

Úklid `troika-clk` (teď 60 commitů nad v7.3.3, UFS = posledních 8):
- zahozeny `phy: … fewer lanes` a `ufs: exynos: Honour lanes-per-direction`
  a tabulka pro nepoužitou linku (řadič má jen jednu linku);
- `lanes-per-direction = <1>` přesunuto do dtsi;
- nová volba `EXYNOS_UFS_OPT_NO_SYNC_PATTERN_MASK` pro 9610 (odstraní tři
  neúspěšné `dme-set`).
`troika-clk-test` přestavěna nad tím (3 DEBUG commity). Kernel sestaven bez
chyb, `dtbs_check` beze změny. `out/troika-test-UFS2.img` připraven –
**na telefonu nezkoušeno**.

UFS zatím není v balíčku pmaports. Zápis na úložiště telefonu (instalace
systému) by byl proti pravidlu „nikdy neflashovat“ a nenavrhuju ho.

### 2026-11-11 ~21:00 – test UFS2 prošel (uklizený kód)

`fastboot boot out/troika-test-UFS2.img`, log v
`device-info/dmesg-mainline-UFS2.txt`. Stejný výsledek jako UFS1: `FAST
series_B G_3 L_1`, `sda` 42 oddílů + `sdb`, `sdc`, vše `ro=1`, nic připojeno,
hashe `boot_a`/`dtbo_a`/`vbmeta_a`/`logo_a` shodné se zálohou, čtení 440 MB/s.
Žádné `dme-set … failed`, žádná chyba v logu. Hláška o záložní GPT trvá
(stav úložiště).

Stav větví kernelu: `troika-clk` 60 commitů nad v7.3.3 (ověřeno na telefonu
přes `troika-clk-test`). Nic není pushnuté. pmaports beze změny (kernel r4).

### 2026-11-11 ~21:15 – commit a push (na pokyn uživatele „Commit a push“)

- `letress/nura-troika` (`origin/main`): dokumentace, nástroje, testovací
  init, `kernel-patches/` nově z větve `troika-clk` (60 patchů),
  `kernel-patches-debug/` (3).
- `letress/pmaports-troika` (`letress/motorola-troika-mainline`): tři místní
  commity (pahole + konfigurace, hodiny + USB r3, zram r4). UFS v balíčku není.
- `letress/uniLoader-troika`: beze změny.
- Strom `linux/` se nepushuje (jen jako patche), `device-info/`, `photos/`,
  `out/`, `build/` také ne.
- V kořeni projektu leží nesledovaný soubor `time` (35 MB, PostScript
  z ImageMagicku, 15:01). Vznikl, když se `tools/phone-sh.py` bez shebangu
  spustil jako shellový skript a řádek `import socket, sys, time` zavolal
  program `import`, který ukládá snímek obrazovky. Necommitnuto; může
  obsahovat obsah obrazovky uživatele, smazání je na uživateli.

### 2026-11-11 ~21:20 – push ověřen; dohoda o sestavování balíčku

Push ověřen: `origin/main` = d950823, `letress/motorola-troika-mainline` =
cfcf09c10 (oba repozitáře soukromé).

Dohoda s uživatelem: balíček kernelu (sestavení 1,5 h) se sestaví až na konci
kola změn. Do té doby se testuje místními testovacími image
(`tools/make-clk-test-bootimg.sh`), které stojí uživatele asi minutu. UFS tedy
zatím do pmaports nejde; počká na další změny kernelu.

