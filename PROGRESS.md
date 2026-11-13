# PROGRESS – port Nura na Motorola One Action (troika)

## Stav: Zvuk, cesta A – překlad adres ABOXu hotový (vlastní doména, mapování firmwaru a vyrovnávací paměti, obsluha chyby) a záporná kontrola ukázala, že kanál RDMA bez firmwaru z paměti nečte (FINDINGS.md 33.10). Další: etapa A2 – nahrát firmware a zapnout procesor ABOXu (3 registry PMU, nový souhlas) nebo zvuk odložit. Balíčky PKG8 (GPU + WiFi) fungují (32.1). Telefon běží v ABOX7, registry ABOXu ve výchozím stavu. Uživatel 2026-11-13 povolil commit a push bez ptaní (viz paměť). `troika-clk` 95 commitů nad v7.3.3, `troika-clk-test` +11

USB síť na telefonu funguje z místně sestaveného kernelu (test USB1). Balíček r3 se
ještě nestavěl. Další krok je na uživateli: sestavit balíčky, `install`, `export`,
pak kontrola boot image a netboot (TESTING.md kap. 5f). Nic se neflashovalo.

## Pravidla (platí pořád)

- Žádné `fastboot flash`, `fastboot erase`, `pmbootstrap flasher`.
  Test jen přes `fastboot boot` a spouští ho uživatel.
- Nesahat do `~/troika-backup`.
- Žádná vymyšlená hardwarová hodnota; bez zdroje = TODO + dotaz.
- Regulátory, nabíjení, napájení v DTS nepovolovat.
  (Výjimka 2026-11-13: pro zvuk – kodek a zesilovač – smějí být popsány,
  postup co nejopatrnější, před každým krokem se ptát.)
- Potřebnou otázku uživateli položit rovnou, vždy. Neptat se předem, jestli
  smím otázku položit (zadání uživatele 2026-11-13).

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
  `patches-src/`, `kernel-patches/` (export větve `troika-clk`, 67 patchů: DT,
  hodiny, USB, UFS, dotyk, restart; po změně kernelu smazat a znovu
  `git -C linux format-patch -o ../kernel-patches v7.3.3..troika-clk`),
  `kernel-patches-debug/` (5 ladicích commitů větve `troika-clk-test`, jen pro
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
  (Uživatel ho smazal tentýž večer.)

### 2026-11-11 ~21:20 – push ověřen; dohoda o sestavování balíčku

Push ověřen: `origin/main` = d950823, `letress/motorola-troika-mainline` =
cfcf09c10 (oba repozitáře soukromé).

Dohoda s uživatelem: balíček kernelu (sestavení 1,5 h) se sestaví až na konci
kola změn. Do té doby se testuje místními testovacími image
(`tools/make-clk-test-bootimg.sh`), které stojí uživatele asi minutu. UFS tedy
zatím do pmaports nejde; počká na další změny kernelu.

### 2026-11-11 ~21:25–21:45 – dotyková obrazovka: průzkum, implementace, image TS1

Uživatel: „Dotyková obrazovka“. Průzkum a implementace ve FINDINGS.md kap. 15.
Telefon má Himax HX83112A (TDDI) na I2C4, adresa 0x48, reset gpa1-3,
přerušení gpa2-4, žádný regulátor (ověřeno v původním DT, ovladači i ve
výpisu GPIO z běžícího Androidu). Mainline má ovladač pro HX83112B; přidáno
ID pro A, uzel I2C4 a uzel čipu (4 commity, `troika-clk` = 64 nad v7.3.3).

`troika-clk-test` = +4 DEBUG commity (nově TSDBG). V místní konfiguraci
`build/linux/.config` je ovladač dotyku `=y` (v balíčku je `=m`; při přenosu
do pmaports zvážit `=y` nebo přidání modulu do initramfs). Kernel sestaven,
`dtbs_check` beze změny, binding prochází. Testovací init: nový oddíl
`--- DOTYK ---` s výpisem souřadnic; hlášky UFS jen v testech UFS.

`out/troika-test-TS1.img` připraven a zkontrolován; **na telefonu nezkoušeno**.
Tyto změny zatím nejsou commitnuté v hlavním repozitáři ani exportované do
`kernel-patches/`.

### 2026-11-11 ~22:25–22:40 – test TS1, oprava ladicího výpisu, image TS2

Uživatel spustil TS1. Log stažen (`device-info/dmesg-mainline-TS1.txt`).
I2C4 běží, čip 1 ms po resetu vrátil ID `83112a` (sedí). Ovladač se ale
nenavázal: můj ladicí kód četl registry firmwaru před kontrolou ID a čip
o pár ms později vracel nuly (`Unknown product id: 0`). Chyba je v DEBUG
commitu, ne ve čtyřech commitech na `troika-clk` (ty se neměnily).

DEBUG commit přepsán (`troika-clk-test` HEAD `055fc49`): čtení až po
registraci přerušení a po 300 ms. `out/troika-test-TS2.img` připraven a
zkontrolován; **na telefonu nezkoušeno**. Nevím, jestli při TS1 zůstal obraz
(uživatel se ještě nevyjádřil). Vše stále necommitnuté v hlavním repozitáři.

### 2026-11-11 ~22:35–22:40 – test TS2

Uživatel spustil TS2 (z TS1 přes Power + Vol−). Log:
`device-info/dmesg-mainline-TS2.txt`. Ovladač se navázal (ID `83112a`),
`Himax Touchscreen` = event0. Přerušení gpa2-4 (úroveň 0): v klidu 0, při
dotyku 1337 za ~25 s, bez chyb kontrolního součtu. Ladicí čtení registrů
firmwaru vrací nuly i po 300 ms (FINDINGS.md 15.5) – pro funkci není potřeba.

Výpis souřadnic v testovacím initu nefungoval (`hexdump` nad evdev → EINVAL);
opraveno v `test-initramfs/init` (`cat | hexdump`), do image se dostane až
při příštím sestavení. V běžícím TS2 jsem výpis spustil přes telnet (na
displej a do `/touch.txt` v RAM telefonu). **Čeká se na dotyk uživatele:
ověřit rozsah a orientaci souřadnic.** Otevřená otázka: zůstal při resetu
dotyku obraz?

### 2026-11-11 ~22:42 – TS2: souřadnice ověřeny

Uživatel přejel prstem z levého horního rohu do pravého dolního; záznam
z `/touch.txt` v RAM telefonu: 326 hlášení, x 30 → 1016, y 27 → 2513, plynule,
bez chyb. Orientace i rozsah sedí s DTS (1080 × 2520), žádné převracení.
**Dotyková obrazovka funguje.**

Zbývá (až uživatel řekne): commit + export `kernel-patches/` (64) a
`kernel-patches-debug/` (4) + push; na konci kola přenést UFS a dotyk do
pmaports (pkgrel 5, ovladač dotyku `=y` nebo modul do initramfs) a jedno
sestavení balíčku. Pozor: plný systém z balíčku uvidí UFS bez ochrany
„jen pro čtení“.

### 2026-11-11 ~22:45 – commit a push dotykové obrazovky

Uživatel: „Commit a push“. `kernel-patches/` znovu exportováno z `troika-clk`
(64; prvních 60 se liší jen číslováním `[PATCH n/64]`), `kernel-patches-debug/`
z `troika-clk..troika-clk-test` (4). Commitnuto spolu s FINDINGS kap. 15,
TESTING 5i, PROGRESS a opraveným `test-initramfs/init` a pushnuto do
`origin/main`. pmaports beze změny (UFS a dotyk tam stále nejsou).

### 2026-11-11 ~22:45–22:56 – restart a vypnutí z kernelu: průzkum, souhlas, image RB1

Uživatel: „vrhni se na Restart a vypnutí z kernelu“. Zdroje: původní
`exynos-reboot.c`, uzel `reboot` v DT, pmucal (FINDINGS.md kap. 16).

**Souhlas se zápisem do PMU** (otázka s přesnými registry, odpověď „Ano,
všechny tři“): `0x11860400` SWRESET = 1; `0x1186330c` PS_HOLD_CONTROL bit 8
= 0; `0x1186081c` SYSIP_DAT3 = důvod restartu (0x77665501 / …00 / …02).
Na SYSIP_DAT0 (`0x810`) ani RESET_SEQUENCER_CONFIGURATION (`0x500`) souhlas
není.

**Moje chyba při průzkumu:** čtení registrů PMU přes debugfs regmapu na
běžícím TS2 shodilo proces chybou kernelu (external abort na offsetu 0x8;
jen čtení). Zámek regmapu zůstal držený; telefon je nutné restartovat
tlačítky. Podrobnosti FINDINGS.md 16.3.

Kernel: `troika-clk` = 67 commitů nad v7.3.3 (nové 3: exynos-pmu compatible,
syscon-reboot + syscon-poweroff v dtsi, reboot-mode v troika.dts).
`troika-clk-test` = +5 DEBUG (nově PMUDBG: čtení 0x500, 0x330c, 0x81c, 0x810
z pracovní fronty 3 s po startu). Sestaveno, `dtbs_check` beze změny.

Testovací init: oddíl `--- RESTART A VYPNUTI ---` a pro značku `rb*` akce na
tlačítkách (Vol+ bootloader, Power restart, Vol− vypnutí, po puštění).
Nový `test-initramfs/reboot-arg.S` (reboot(2) s argumentem; busybox to
neumí), skript ho překládá do image.

`out/troika-test-RB1.img` připraven a zkontrolován; **na telefonu
nezkoušeno**. Necommitnuto v hlavním repozitáři, `kernel-patches/` není
znovu exportováno.

### 2026-11-11 ~22:58 – RB1 spuštěn, stav před akcemi

Uživatel spustil RB1. Log stažen před stiskem tlačítek
(`device-info/dmesg-mainline-RB1.txt`): RESET_SEQUENCER_CONFIGURATION = 0,
PS_HOLD_CONTROL = 0x80005300, SYSIP_DAT3 = 0, SYSIP_DAT0 = 0; všechny tři
ovladače navázané (FINDINGS.md 16.4). **Výsledky tří akcí (Vol+, Power,
Vol−) čekají na uživatele.**

### 2026-11-11 ~23:05 – RB1 prošel

Uživatel po třech bězích RB1: „restart do bootloaderu funguje, vypnutí
funguje, klasický restart taky funguje“. Zapsáno do FINDINGS.md 16.5.
Bootloaderu stačí SYSIP_DAT3. Od teď jde z testovacího image do fastbootu
přes telnet (`tools/phone-sh.py 'reboot-arg bootloader'`) – spouštět jen
když to uživatel chce, restart telefonu je jeho rozhodnutí.

Zbývá (až uživatel řekne): commit hlavního repozitáře + export
`kernel-patches/` (67) a `kernel-patches-debug/` (5) + push; pak přenést
UFS, dotyk a restart do pmaports (pkgrel 5) a jedno sestavení balíčku.

### 2026-11-11 ~23:08 – commit a push restartu a vypnutí

Uživatel: „Commit a push“. `kernel-patches/` znovu exportováno z `troika-clk`
(67; prvních 64 se liší jen číslováním), `kernel-patches-debug/` (5).
Commitnuto spolu s FINDINGS kap. 16, TESTING 5j, PROGRESS, testovacím initem,
`test-initramfs/reboot-arg.S` a úpravou `tools/make-clk-test-bootimg.sh`;
pushnuto do `origin/main`. pmaports beze změny.

### 2026-11-11 ~23:10–23:17 – balíček pmaports připraven (kernel r5, zařízení r3)

Uživatel se ptal na SD kartu, WiFi, zvuk, grafiku a kameru: z původního DT je
to dw-mshc + PMIC S2MPU09 přes SPEEDY, Samsung SCSC, ABOX + CS35L41,
exynos9-decon/dsim + Mali, FIMC-IS. Nic z toho nejde hned; doporučil jsem
nejdřív balíček. Uživatel: „Připrav balíček“.

pmaports (FINDINGS.md kap. 17): `tools/update-kernel-pkg.sh` přenesl 67
patchů, dotyk `=y`, kernel pkgrel 5 (commit `2b727fc1f`); balíček zařízení
pkgrel 3 s pravidlem udev, které drží disky UFS jen pro čtení (commit
`03767089d`; moje pojistka navíc). `kconfig check` prochází. **Nepushnuto,
nesestaveno.**

Další krok je na uživateli: sestavit, `install`, `export`, pak mi napsat;
zkontroluju boot image a uložím jako `out/troika-test-NET3.img`.
V hlavním repozitáři jsou necommitnuté FINDINGS kap. 17, TESTING 5k a
PROGRESS.


### 2026-11-12 – úklid místa na disku

Sestavení balíčku kernelu r5 selhalo: došlo místo na disku (26 MB volných).
Se souhlasem uživatele smazány jen slepé uličky (mimo git, 1,4 GB):
`build/verify`, `build/uniloader-pkgtest`, `build/prev-fdt-test`,
`out/troika-test-{A,B,C,D,E1,E2,E3,F,G,H,K,P,P2}.img`,
`out/uniLoader-troika-{diag,diagboot,embedded}`. `build/linux` a
`refs/linux-next` zůstaly. Největší položky jsou mimo projekt: zbytek
nepovedeného sestavení v chrootu pmbootstrapu (9,2 GB, smaže `pmbootstrap
zap`), ccache (2,8 GB), obraz pro netboot (1,4 GB).

### 2026-11-12 00:52–01:40 – kernel r5 sestaven, první export zadržen

Kernel r5 sestaven (00:52), `install` (01:16) a `export` (01:21) doběhly.
Boot image z exportu odpovídá balíčkům (uniLoader = r5, „second“ = původní
DTB, žádné ladicí výpisy; FINDINGS.md 17.1).

Při kontrole initramfs jsem ale našel, že má disky UFS zapisovatelné a že
jeho skripty (`mount_subpartitions` + `resize_root_partition`) mohou při
souběhu spustit `parted … resizepart` nad oddílem UFS (FINDINGS.md 17.2).
Na 43 obrazech ze zálohy podmínka nenastává, systém a uživatelská data
v záloze nejsou. Image jsem proto **neuložil jako NET3 a k testu nepustil**.

Oprava: balíček zařízení pkgrel 4 (pmaports commit `f5083510b`, místní):
pravidlo udev a `blockdev` i v initramfs, háček `00-ufs-readonly.sh`, který
před `netboot.sh` zkontroluje `ro` = 1 a jinak start zastaví. Háček vyzkoušen
nasucho, na telefonu neověřen.

Další krok je na uživateli: `install` a `export` znovu (kernel se
nesestavuje), pak mi napsat. Zkontroluju, že initramfs obsahuje pravidlo,
háček a `blockdev`, a uložím `out/troika-test-NET3.img`.
V hlavním repozitáři jsou necommitnuté FINDINGS kap. 17, TESTING 5k a
PROGRESS; v pmaports tři místní commity (`2b727fc1f`, `03767089d`,
`f5083510b`), nepushnuto.

### 2026-11-12 01:31–01:40 – druhý `install` selhal (vada pmbootstrapu)

Balíček zařízení r4 se sestavil (01:32) a initramfs z druhého exportu (01:33)
už obsahuje pravidlo `60-ufs-readonly.rules`, háček `hooks/00-ufs-readonly.sh`
(oba shodné s pmaports) a `/usr/bin/blockdev` z util-linux (potřebuje jen
musl, ten tam je).

`install` ale skončil chybou v kroku 3/4: `losetup --json --list` – po sestavení
balíčku pmbootstrap smazal a znovu vytvořil nativní chroot a util-linux do něj
podruhé nenainstaloval (busybox `losetup` neumí `--json`). Obraz systému
`motorola-troika.img` je proto prázdný (bez tabulky oddílů) a export k ničemu.
Balíček r4 už je sestavený, takže další `install` nic nesestavuje a chroot
nemaže. Postup: `pmbootstrap shutdown`, `install`, `export`.

### 2026-11-12 01:41–01:50 – image NET3 připraven

Po `pmbootstrap shutdown` třetí `install` (01:45) a `export` (01:46) doběhly.
Kontrola (FINDINGS.md 17.3): boot image odpovídá balíčkům, initramfs má
pravidlo, háček i `blockdev`, UUID oddílů v obrazu systému souhlasí s boot
image. Uloženo jako `out/troika-test-NET3.img` (sha256 začíná `d200dd47`).

Další krok je na uživateli: TESTING.md 5k kroky 3–5 (`netboot serve
--replace`, `fastboot boot`, ssh, `lsblk`). Necommitnuto: FINDINGS kap. 17,
TESTING 5k, PROGRESS; pmaports tři místní commity, nepushnuto.

### 2026-11-12 01:49–01:56 – test NET3: start a úložiště jen pro čtení

Uživatel spustil server, `fastboot boot out/troika-test-NET3.img`, přihlásil
se přes SSH. `lsblk`: všechny disky a oddíly UFS mají RO = 1 a nejsou
připojené; `dmesg` neobsahuje „was not read-only“ (FINDINGS.md 17.4).
Uživatel: „vše se zobrazilo správně“.

Zbývá: dotyk (`/proc/interrupts`, řádek `hx83112a`), `systemctl
is-system-running`, `sudo reboot`, `sudo poweroff`.

### 2026-11-12 ~02:00 – test NET3 prošel

`systemctl is-system-running` = `running`; `hx83112a` v `/proc/interrupts`
má 286 přerušení; `sudo reboot` restartoval telefon. Uživatel: „vše proběhlo
správně“ (FINDINGS.md 17.5). `sudo poweroff` z plného systému se nezkoušel.

Stav repozitářů: hlavní – necommitnuté FINDINGS kap. 17, TESTING 5k,
PROGRESS; pmaports – místní commity `2b727fc1f`, `03767089d`, `f5083510b`
(nepushnuto). Možné další kroky (rozhodne uživatel): commit a push; průzkum
SD karty (jen čtení zdrojů, regulátory jsou zakázané); nahlášení souběhu
v `mount_subpartitions` do postmarketos-initramfs.

### 2026-11-12 ~02:05 – commit a push balíčku a testu NET3

Na pokyn uživatele: hlavní repozitář (FINDINGS kap. 17, TESTING 5k, PROGRESS)
commitnut a pushnut do `origin/main`; pmaports větev
`motorola-troika-mainline` pushnuta do `letress/pmaports-troika` (commity
`2b727fc1f`, `03767089d`, `f5083510b`).

### 2026-11-12 ~02:07–02:15 – příprava testu GUI1 (Phosh přes netboot)

Uživatel: „a kdy budeme nasazovat gui?“ → kernel má `DRM_SIMPLEDRM=y`
a dotyk funguje, takže Phosh se softwarovým vykreslováním jde zkusit hned,
jen přes netboot. Uživatel: „ano, chci“.

- `pmbootstrap config ui phosh` (spustil jsem sám, bez sudo); `extra_packages`
  zůstává háček netboot.
- Před tím jsem ve zdrojích kernelu ověřil, co příznak jen pro čtení u disků
  UFS skutečně brání (FINDINGS.md 17.2, tabulka): `write(2)`, `mmap` se
  zápisem a připojení pro zápis selžou; příkazy SCSI napřímo (`SG_IO`, bsg)
  by ho obešly. Opraveno moje dřívější nepřesné tvrzení o `losetup`.
- Phosh závisí na `udiskie` (automatické připojování); kontrola `lsblk`
  po startu zůstává prvním krokem.
- Postup: TESTING.md 5l. FINDINGS/TESTING/PROGRESS necommitnuto.

### 2026-11-12 02:15–02:35 – image GUI1 připraven

Uživatel spustil `install --zap` (DONE 02:28, obraz 3037 M) a `export`
(02:31). Kontrola (FINDINGS.md 18.1): boot image odpovídá balíčkům, initramfs
je stejný jako u NET3 včetně ochrany UFS, UUID sedí, v systému je Phosh 0.58.
Uloženo jako `out/troika-test-GUI1.img`. Další krok je na uživateli:
TESTING.md 5l kroky 3–5. FINDINGS/TESTING/PROGRESS necommitnuto.

### 2026-11-12 02:32–02:46 – test GUI1: Phosh běží

Uživatel spustil server a `fastboot boot out/troika-test-GUI1.img`. Phosh
nastartoval: načítací kolečko, přihlašovací klávesnice (miniaturní), po
přihlášení plocha; vše malé, uživatel si nastavil zvětšení 200 %
(FINDINGS.md 18.2). Příčina: framebuffer nenese fyzický rozměr panelu.
Zdroj rozměru nalezen v původním kernelu: 64 × 148 mm.

Zbývá: `lsblk` v tomto běhu, jméno výstupu (`ls /sys/class/drm/`), pak
`phoc.ini` s měřítkem do balíčku zařízení (pkgrel 5, bez sestavení kernelu).
FINDINGS/TESTING/PROGRESS necommitnuto.

### 2026-11-12 ~02:46–02:55 – GUI1 prošel, měřítko do balíčku zařízení

`lsblk` v běžícím Phoshi: UFS všude RO = 1, nic připojeného; výstup se jmenuje
`Unknown-1`. Uživatel: „funguje to dobře, ale grafické prostředí je trhané,
dá se v něm pohybovat“ (FINDINGS.md 18.3).

pmaports: balíček zařízení pkgrel 5, podbalíček `device-motorola-troika-phosh`
s `/etc/phosh/phoc.ini` (měřítko 3 pro `Unknown-1`, na přání uživatele:
„ideální je pro mě 300 %“); místní commit,
nesestaveno. Postup testu: TESTING.md 5m. Hlavní repozitář necommitnut
(FINDINGS kap. 17.2 oprava + 18, TESTING 5l + 5m, PROGRESS).

### 2026-11-12 ~02:58–03:10 – měřítko 3, vypnutí z plného systému

Uživatel chce měřítko 300 % → `phoc.ini` změněn na `scale = 3`, commit
v pmaports přepsán (`f1514a044`, místní). První `build` selhal, protože ještě
běžel netboot server (umount chrootu, kód 32). Uživatel telefon vypnul
`sudo poweroff` z plného systému – **vypnul se** (FINDINGS.md 18.4) – ukončil
server, `build` balíčku zařízení r5 prošel (03:04) a běží `install`.

### 2026-11-12 03:10–03:13 – image GUI2 připraven

`install` (03:08) a `export` (03:10) doběhly. Kontrola (FINDINGS.md 18.5):
vše odpovídá balíčkům, v systému je `/etc/phosh/phoc.ini` s měřítkem 3.
Uloženo jako `out/troika-test-GUI2.img`. Další krok je na uživateli:
`netboot serve --replace`, `fastboot boot`, podívat se na velikost
přihlašovací obrazovky, přes SSH `lsblk`.

### 2026-11-12 03:13–03:22 – test GUI2 prošel

Uživatel: „velikost je nyní už skvělá“; `lsblk` v pořádku (FINDINGS.md 18.6).
Na otázku „na co se vrhnout teď“ navrženo: nejdřív commit a push, potom
průzkum podsvícení (I2C 0x36 v původním DT), případně rozměr panelu do DT
s příštím sestavením kernelu. GPU, displej (DECON), WiFi, zvuk a SD karta
jsou velké práce a většinou narážejí na pravidlo o regulátorech.

### 2026-11-12 ~03:25 – commit a push GUI

Na pokyn uživatele („Jen commit a push“): hlavní repozitář (FINDINGS 17.2
oprava + kap. 18, TESTING 5l a 5m, PROGRESS) commitnut a pushnut do
`origin/main`; pmaports větev `motorola-troika-mainline` pushnuta do
`letress/pmaports-troika` (nově `f1514a044`, balíček zařízení r5 s měřítkem
pro Phosh). Podsvícení se zatím nezačalo.

### 2026-11-12 ~03:30–04:00 – GPU: průzkum a image GPU1

Uživatel: „vytvoř ovladač GPU, ať vím, jestli má projekt smysl“, s využitím
práce ostatních.

- Ovladač existuje (panfrost + Mesa), chybí popis v DT. Hodnoty z původního
  DT a cmucal (FINDINGS.md 19.2).
- Nalezen `github.com/arhammahajan/gta4xl-linux`: mainline 7.3 na Exynosu 9611
  s funkčním panfrostem, vlastním ovladačem displeje, WiFi/BT, SD, DVFS,
  baterií (FINDINGS.md 19.3). Staženo do `refs/gta4xl-linux` (gitignore).
- Kernel: 3 commity na `troika-clk` (binding, `cmu_g3d` + `gpu` v dtsi,
  zapnutí na troice); bez regulátoru, bez OPP, bez napájecí domény.
  `troika-clk-test` přeskládán (5 DEBUG nad tím). dtbs_check čistý až na
  známé varování USB.
- `test-initramfs/init`: oddíl `--- GPU ---` pro značku `gpu*`.
- `out/troika-test-GPU1.img` (sha256 začíná `87de57c9`).

Další krok: uživatel spustí GPU1; já přečtu stav přes telnet. Když se panfrost
naváže, druhý krok je plný systém (Phosh z netbootu) s místním kernelem –
bez sestavování balíčku – a zjistit, jestli phoc vykresluje na GPU.
Necommitnuto: FINDINGS kap. 19, TESTING 5n, PROGRESS, `test-initramfs/init`;
`kernel-patches/` ještě není přeexportováno (67 → 70).

### 2026-11-12 03:42–04:10 – test GPU1 prošel, příprava GPU2

GPU1: panfrost se navázal (Mali-G72 r0p1, 3 jádra, 260 MHz, `renderD128`,
bez chyb; FINDINGS.md 19.6). Log uložen do `device-info/dmesg-mainline-GPU1.txt`.

Pro druhý krok: `build/linux` přestavěn z čisté větve `troika-clk`
s konfigurací balíčku (původní místní konfigurace je v
`build/linux/.config.local-test`), `LOCALVERSION_AUTO` vypnuto, sestavení
s `LOCALVERSION=`, aby se kernel jmenoval `7.3.3-troika` jako v balíčku.
Nový skript `tools/make-local-pmos-bootimg.sh`.

### 2026-11-12 04:04–04:10 – image GPU2 připraven

Telefon poslán přes telnet do fastbootu (`reboot-arg bootloader`, na pokyn
uživatele). Kernel s konfigurací balíčku sestaven (18 min, plné sestavení),
jmenuje se `7.3.3-troika`, vermagic shodný s moduly v systému, bez ladicích
výpisů. `tools/make-local-pmos-bootimg.sh GPU2 out/troika-test-GUI2.img` →
`out/troika-test-GPU2.img` (sha256 začíná `d09db2d9`): ramdisk, „second“
a cmdline shodné s GUI2, liší se jen kernel (obsahuje uzel GPU).
Další krok je na uživateli: TESTING.md 5o.

### 2026-11-12 04:07–04:15 – GPU2 první pokus: ladicí shell initramfs

`fastboot boot out/troika-test-GPU2.img` skončil v ladicím shellu initramfs:
`nbd.ko` z balíčku se do místního kernelu nenačetl kvůli neshodě BTF
(FINDINGS.md 19.8). UFS zůstalo jen pro čtení (pravidlo zabírá už
v initramfs). Místní kernel se přestavuje s `MODULE_ALLOW_BTF_MISMATCH=y`;
pak nový image GPU2.

### 2026-11-12 04:12 – opravený image GPU2

Kernel přestavěn s `MODULE_ALLOW_BTF_MISMATCH=y` (2 min), image GPU2 přepsán
(sha256 začíná `8a4d0449`); ramdisk a „second“ shodné s GUI2. Uživatel mezitím
dvakrát spustil starý image (ladicí shell, žádná škoda).

### 2026-11-12 13:32–13:40 – test GPU2 prošel: Phosh vykresluje na GPU

Uživatel spustil server a opravený image GPU2. Systém nastartoval,
`panfrost-job` má 15 638 přerušení → phoc vykresluje přes panfrost bez
nastavování (FINDINGS.md 19.9). UFS RO = 1.

Zbývá: dojem uživatele z plynulosti; přenést 3 commity do `kernel-patches/`
a do balíčku (kernel pkgrel 6, sestavení 1,5 h – jedno kolo, případně
i s rozměrem panelu); commit a push. Další možné kroky: řízení frekvence GPU
přes ACPM (podklad: gta4xl `0170-exynos9611-gpu-dvfs.patch`), ovladač
displeje (gta4xl `0130`, jiný panel).

### 2026-11-12 ~13:40 – commit a push GPU

Uživatel: Phosh je „mnohem více plynulejší“. Na jeho pokyn commit a push
hlavního repozitáře: FINDINGS kap. 19, TESTING 5n a 5o, PROGRESS,
`test-initramfs/init` (oddíl GPU), `tools/make-local-pmos-bootimg.sh`,
`kernel-patches/` (70) a `kernel-patches-debug/` (5). pmaports beze změny
(`f1514a044` je pushnutý).

Další přání uživatele: podsvícení displeje a změna frekvence GPU přes
firmware, s co nejméně sestaveními kernelu → zkoušet místními image,
balíček sestavit jednou na konci.

### 2026-11-12 ~13:40–13:48 – průzkum podsvícení, ACPM a WiFi

Uživatel: Phosh je s GPU mnohem plynulejší; chce zkusit ztmavování displeje,
změnu frekvence GPU přes firmware a WiFi, s co nejméně sestaveními kernelu.

Průzkum (FINDINGS.md kap. 20): podsvícení je SGM37603A na I2C 0x36
(`hsi2c_3`, 0x11D10000); frekvence GPU potřebuje celý řetěz ACPM a souhlas
se změnou napětí přes firmware; WiFi je SCSC Maxwell s ovladačem mimo strom.
Doporučené pořadí: podsvícení → ACPM/GPU → WiFi. Vše zkoušet místními image,
balíček sestavit jednou po dávce.

Rozpracováno: nic v kernelu (jen průzkum).

### 2026-11-12 13:48–13:58 – podsvícení, krok 1: sběrnice a image BL1

Uživatel: „Ano, podsvícení“.

- Kernel, větev `troika-clk` (+5, celkem 75 nad v7.3.3): tři bindingy
  (`samsung,exynos9610-cmgp-sysreg`, `-usi`, `-hsi2c`), v dtsi `sysreg_cmgp`,
  `usi_cmgp0_i2c` a `hsi2c_3`, na troice zapnutí sběrnice na 400 kHz.
  Bez uzlu čipu – ovladač zatím není. dtbs_check: jen známé varování USB.
- `test-initramfs/i2c-reg.c`: malý nástroj pro čtení a zápis registru I2C
  (busybox žádný nemá); `tools/make-clk-test-bootimg.sh` ho přibaluje.
  Na PC se nedal vyzkoušet (qemu z chrootu pmbootstrapu padá), ověří se na
  telefonu.
- `test-initramfs/init`: oddíl `--- PODSVICENI ---` pro značku `bl*`, jen čte
  registry 0x10, 0x11, 0x19, 0x1a, 0x1b, 0x1f.
- `out/troika-test-BL1.img` (sha256 začíná `233a5769`), kernel z větve
  `troika-clk-test` s konfigurací balíčku.

Další krok: uživatel spustí BL1; já přes telnet přečtu registry. Zápis jasu
až po jeho souhlasu s konkrétními hodnotami.

### 2026-11-12 13:59–14:05 – test BL1 prošel

Sběrnice `i2c-1` (11d10000.i2c) funguje, `i2c-reg` funguje, registry čipu:
0x10 = 0x1f, 0x11 = 0x65, 0x19 = 0xcc, 0x1a = 0xfc, 0x1b = 0x00, 0x1f = 0x00
(FINDINGS.md 20.7). Stažen datasheet SGM37603A (mimo repozitář): režim
I2C × PWM, kód 0 = vypnuto → stmívání půjde přes I2C.

Jiná session upozornila, že zkouška `i2c-reg` pod qemu na PC shodila qemu
a že by se skutečnými argumenty sahala na sběrnici PC; poučení uloženo do
paměti. Nic se nestalo.

Další krok: se souhlasem uživatele zapsat přes telnet kódy 0x400, 0x100,
0x000 a zpět 0xCCC a sledovat displej; pak napsat ovladač podsvícení.

### 2026-11-12 14:03–14:06 – pokusný zápis jasu

Uživatel odsouhlasil čtyři zápisy (AskUserQuestion: „Ano, všechny čtyři“).
Provedeno přes telnet; zpětné čtení sedí, bez chyb, čip vrácen na 0xCCC
(FINDINGS.md 20.8). Co bylo vidět na displeji, zatím nevím.

### 2026-11-12 14:06–14:30 – ovladač podsvícení

Uživatel potvrdil ztlumení a zhasnutí displeje při pokusu. Napsán ovladač
`sgm37603a` + binding + uzel v DT (FINDINGS.md 20.9); frekvence sběrnice
snížena na 390 kHz (commit sběrnice přepsán přes autosquash).
`troika-clk-test` znovu postaven nad `troika-clk` (`rebase --onto`).

Místní konfigurace: `BACKLIGHT_CLASS_DEVICE=y`, `BACKLIGHT_SGM37603A=y`.
Pozor: sestavení jsem pustil s `W=1`, což vyvolalo plné přeložení (asi
18 min); po něm je potřeba buď `W=1` držet, nebo přeložit znovu bez něj.

### 2026-11-12 14:32 – image BL2 připraven

Kernel z `troika-clk` (s `W=1`, plné sestavení 23 min, ovladač bez varování)
→ `tools/make-local-pmos-bootimg.sh BL2 out/troika-test-GUI2.img` →
`out/troika-test-BL2.img` (sha256 začíná `dbaee858`); ramdisk a „second“
shodné s GUI2. Na pozadí běží přeložení bez `W=1`, aby další sestavení byla
zase přírůstková. Další krok je na uživateli: TESTING.md 5q.

### 2026-11-12 14:39–14:50 – test BL2: ovladač ano, posuvník ne

BL2 běží, ovladač podsvícení navázán (3276/3276). Posuvník v Phoshi chybí:
Phosh páruje podsvícení s vestavěným konektorem, simpledrm má `Unknown`
(FINDINGS.md 20.10). Uživateli navržena zkouška s `~/.phoshdebug`
(`PHOSH_DEBUG=fake-builtin`) a restartem greetd.

### 2026-11-12 ~14:55–15:00 – posuvník jasu funguje

S `~/.phoshdebug` (`PHOSH_DEBUG=fake-builtin`) se posuvník objevil a jas
mění (FINDINGS.md 20.11). Do balíčku zařízení připraven doplněk služby
`mobi.phosh.Shell.service` (pkgrel 6, soubor `phosh-builtin-display.conf`;
zatím necommitnuto, čeká na zkoušku naživo). Přeložení kernelu bez `W=1`
doběhlo, `build/linux` je z čisté větve `troika-clk`.

### 2026-11-12 ~15:00–15:08 – doplněk služby potvrzen, simpledrm a image BL3

Uživatel: s doplňkem služby posuvník zůstal; při nečinnosti displej zhasne
úplně, po stisku Power zůstává podsvícení svítit (FINDINGS.md 20.12).

- pmaports: commit `dd0c8a565` (zařízení pkgrel 6, doplněk služby Phoshe).
- Kernel: 3 commity (binding `backlight` u simple-framebuffer, simpledrm
  vypíná podsvícení s CRTC, propojení v DT troiky); kontroly čisté.
- `out/troika-test-BL3.img` (sha256 začíná `400815f8`).

### 2026-11-12 15:10–15:16 – BL3 prošel, balíčky připraveny

Uživatel: s BL3 „všechno funguje správně“ (Power zhasíná i podsvícení,
`sudo reboot bootloader` funguje; FINDINGS.md 20.13).

- pmaports: kernel pkgrel 6 (81 patchů přes `tools/update-kernel-pkg.sh`,
  podsvícení vestavěné), `kconfig check` prochází; commit místně.
- Hlavní repozitář: `kernel-patches/` přeexportováno na 81, `-debug` 5.
- Postup pro sestavení a test balíčku: TESTING.md 5s.

Necommitnuto v hlavním repozitáři: FINDINGS 20.6–20.14, TESTING 5p–5s,
PROGRESS, `test-initramfs/init`, `test-initramfs/i2c-reg.c`,
`tools/make-clk-test-bootimg.sh`, `kernel-patches*/`.

### 2026-11-12 ~15:20 – commit a push podsvícení a balíčků

Na pokyn uživatele: hlavní repozitář commitnut a pushnut (FINDINGS 20.6–20.14,
TESTING 5p–5s, PROGRESS, `test-initramfs/init`, `test-initramfs/i2c-reg.c`,
`tools/make-clk-test-bootimg.sh`, `kernel-patches/` 81, `-debug/` 5);
pmaports větev pushnuta do `letress/pmaports-troika` (`dd0c8a565` zařízení
pkgrel 6, `6a3faa4df` kernel pkgrel 6).

### 2026-11-12 15:16–15:28 – ACPM: průzkum a image ACPM1

Uživatel se ptal, jestli už sestavovat balíček; doporučil jsem počkat
a přidat nejdřív základ ACPM (frekvence CPU a GPU, SD karta). Uživatel:
„pusťme se do komunikace s firmwarem“ (krok jen čtení; změna frekvence až
po dalším výslovném souhlasu).

- Průzkum: mainline ovladač ACPM pro GS101 sedí i na 9610 – stejné uspořádání
  sdílené paměti, mailbox jako Exynos850 (FINDINGS.md 21.1–21.2).
- Kernel: 4 commity na `troika-clk` (85 nad v7.3.3); na `troika-clk-test`
  nový ladicí commit `ACPMDBG` (6 DEBUG celkem).
- `test-initramfs/init`: oddíl `--- ACPM ---` pro značku `acpm*`.
- `out/troika-test-ACPM1.img` (sha256 začíná `dc265bc4`).

### 2026-11-12 16:02–16:10 – test ACPM1 a image ACPM2

ACPM1: ovladač a mailbox se navázaly, kanály přečteny (8). Dotazy na
frekvenci mainline odmítl odeslat (kanál 5 bez dotazování), tabulky fvmap
byly ze špatné adresy – leží v modulu DVFS (FINDINGS.md 21.5). Firmwaru
nebyla poslána žádná zpráva.

Ladicí commit přepsán: vypisuje seznam modulů firmwaru, vlastníky kanálů
a tabulky z adresy modulu DVFS; nic neodesílá. `out/troika-test-ACPM2.img`
(sha256 začíná `28e5be23`).

Pozn.: při čtení logu jsem vypsal i začátek řádku s příkazovou řádkou
kernelu (zkrácený na 170 znaků, sériové číslo v něm nebylo); filtr doplněn
do `test-initramfs/init`, v dalších příkazech ho hlídat.

### 2026-11-12 16:10–16:20 – test ACPM2: moduly a tabulky

Telefon poslán do fastbootu přes telnet (na pokyn uživatele; první pokus
`reboot-arg` nezabral, druhý s plnou cestou `/bin/reboot-arg` ano).
ACPM2: 6 modulů firmwaru, DVFS zavedený, FVP a TMU ne; tabulky frekvencí
a napětí přečteny ze SRAM + 0x12400; kanály 0, 1, 5, 6 patří modulu FVP,
kanál 7 modulu TMU (FINDINGS.md 21.6). Soubory obou modulů nalezeny ve
zdrojích původního kernelu (`firmware/`, po 5120 bajtech).

Výpis uložen do `device-info/acpm-fvmap-ACPM2.txt` (gitignore).
Další krok: požádat uživatele o souhlas s nahráním modulu FVP (zápis kódu
firmwaru do SRAM APM + zpráva „attach“ kanálem 4), případně i TMU.

### 2026-11-12 16:20–16:40 – souhlas s modulem FVP, image ACPM3

Uživatel: „Ano, nahrát modul“ (souhlas jen s modulem FVP a dotazy na
frekvenci). Dohledán přesný postup původního kernelu: žádost o připojení
kanálem 4, dotazy na frekvenci kanálem 0 (ne 5). Ladicí commit rozšířen
o zápis modulu se zpětným čtením, žádost o připojení a pět dotazů; vše
s pojistkami (FINDINGS.md 21.7). `tools/make-clk-test-bootimg.sh` přidává
soubor modulu do initramfs. `out/troika-test-ACPM3.img` (sha256 začíná
`8f74232a`).

### 2026-11-12 16:55–17:05 – test ACPM3 prošel

Telefon poslán do fastbootu přes telnet (na pokyn uživatele). ACPM3: modul
FVP zapsán na SRAM + 0x9000, zpětné čtení sedí, připojení potvrzeno, firmware
odpovídá na zprávy kanálem 0; `FREQ_GET` vrací 0 (FINDINGS.md 21.8). Telefon
po testu v pořádku, frekvence beze změny.

Další kroky: (1) čtení teploty – průzkum čidla TMU; (2) přesunout nahrávání
modulu z ladicího commitu do čistého ovladače; (3) až potom souhlas uživatele
se změnou frekvence (zpráva `FREQ_REQ`).

### 2026-11-12 18:45 – commit a push (ACPM)

Na pokyn uživatele: `kernel-patches/` znovu vyexportováno (85),
`kernel-patches-debug/` (6), hlavní repozitář commitnut (`849beb9`) a pushnut.
pmaports beze změny (balíček kernelu pkgrel 6 stále nesestaven a bez commitů
ACPM). Telefon běží v testovacím image ACPM3 s připojeným modulem FVP (do
restartu). Další krok: průzkum čidla teploty (jen čtení).


### 2026-11-12 18:45–19:00 – teplota čipu: průzkum, čtení registrů, ovladač

- Průzkum (jen čtení): původní kernel čte teplotu přes firmware (modul TMU,
  kanál 7), port gta4xl přímo z registrů TMU, které nechal běžet bootloader.
- Nový nástroj `test-initramfs/mem-rd.c` (čte registry přes `/dev/mem`,
  zapisovat neumí); na telefon v image ACPM3 přenesen přes telnet.
- Na telefonu přečteny registry TMU `0x10070000`: bootloader čidlo spustil,
  zkalibroval a zapnul ochranné vypnutí při 115 °C. Teploty v klidu 43–44 °C,
  při plné zátěži 8 jader se velká jádra do 2 minut ustálí na 70 °C
  (frekvence z bootloaderu). Surová data: `device-info/tmu-regs-ACPM3.txt`.
- `troika-clk` +3 (binding, ovladač `exynos9610_tmu.c`, uzel TMU a tři
  teplotní zóny v dtsi), `troika-clk-test` přeskládána (6 ladicích commitů).
  `build/linux/.config`: `EXYNOS9610_THERMAL=y`. Kontroly: `dt_binding_check`
  bez nálezu, `CHECK_DTBS` jen známé varování USB.
- Sestaven `out/troika-test-TMU1.img` (sha256 začíná `2fced37f`), postup
  TESTING.md 5w. Testovací init má nový oddíl `--- TMU ---`.
- Necommitnuto v hlavním repozitáři: FINDINGS kap. 22, TESTING 5w, PROGRESS,
  `test-initramfs/init`, `test-initramfs/mem-rd.c`,
  `tools/make-clk-test-bootimg.sh`; `kernel-patches/` čeká na export (88).
- Pro balíček kernelu (až bude dávka hotová): přidat `EXYNOS9610_THERMAL=y`
  vedle `EXYNOS_MBOX=y` a `EXYNOS_ACPM_PROTOCOL=y`.

Další kroky: (1) test TMU1; (2) přesunout nahrávání modulu FVP do čistého
ovladače; (3) souhlas uživatele se změnou frekvence a první `FREQ_REQ`.

### 2026-11-12 19:14 – test TMU1 prošel

Telefon restartován přes telnet do fastbootu (na pokyn uživatele), uživatel
spustil TMU1. Tři teplotní zóny a hwmon ukazují stejné hodnoty jako přepočet
registrů čtených `/bin/mem-rd`, při zátěži reagují; registry TMU beze změny.
Telefon běží dál v TMU1 (modul FVP připojen do restartu). Necommitnuto.

Další kroky: (1) přesunout nahrávání modulu FVP z ladicího commitu do
čistého ovladače; (2) souhlas uživatele se změnou frekvence a první
`FREQ_REQ`; teplota se při tom dá sledovat ze zón.

### 2026-11-12 19:20 – commit a push (teplota)

Na pokyn uživatele: `kernel-patches/` znovu vyexportováno (88),
`kernel-patches-debug/` (6), hlavní repozitář commitnut a pushnut. pmaports
beze změny: balíček kernelu pkgrel 6 je stále nesestavený a chybí mu commity
ACPM a TMU a volby `EXYNOS_MBOX=y`, `EXYNOS_ACPM_PROTOCOL=y`,
`EXYNOS9610_THERMAL=y`. Telefon běží v TMU1.

### 2026-11-12 19:25–19:52 – nahrávání modulu FVP v čistém ovladači

- `troika-clk` +1: `firmware: exynos-acpm: Attach the DVFS plugin of the
  Exynos9610` (hledání modulu v tabulce firmwaru, kontroly, kopie, zpětné
  čtení, žádost o připojení kanálem 4). Modul TMU se nenahrává.
- `troika-clk-test`: starý ladicí commit ACPM zahozen a nahrazen menším,
  který jen vypisuje rozložení firmwaru a stav modulů (nic neposílá).
- Sestaveno bez varování, `out/troika-test-ACPM4.img` (sha256 začíná
  `b4372c00`), postup TESTING.md 5x.
- Zjištění: původní kernel nahrává FVP i TMU; jestli FVP potřebuje TMU,
  nevím (FINDINGS.md 23.1, otevřená otázka před změnou frekvence).
- Pro balíček: soubor `exynos9610_acpm_fvp.fw` musí být v initramfs; odkud
  ho brát, rozhodne uživatel (FINDINGS.md 23.2).
- Necommitnuto: FINDINGS kap. 23, TESTING 5x, PROGRESS,
  `test-initramfs/init`; `kernel-patches/` čeká na export (89, ladicí 6).

Další kroky: (1) test ACPM4; (2) souhlas uživatele se změnou frekvence
s konkrétními hodnotami a první `FREQ_REQ`.

### 2026-11-12 20:12 – test ACPM4 prošel

Telefon restartován přes telnet do fastbootu (na pokyn uživatele), uživatel
spustil ACPM4. Modul FVP je připojený čistým ovladačem, bez varování; modul
TMU nepřipojený; teploty, hodiny a UFS beze změny. Telefon běží v ACPM4.
Necommitnuto (viz předchozí záznam).

Další krok: připravit pro uživatele návrh první změny frekvence (která
doména, jaké hodnoty, meze Exynosu 9609 z původního DT, rizika) a požádat
o výslovný souhlas. Bez něj žádnou zprávu `FREQ_REQ` neposílat.

### 2026-11-12 20:13–20:19 – návrh první změny frekvence, image ACPM5

- Průzkum (jen čtení): původní kernel mění frekvenci GPU zprávou
  `{4, kHz, 0, 0}` kanálem 0, napětí řídí firmware; meze GPU v DT telefonu
  260 až 1053 MHz; stupně 260 a 338 MHz mají v tabulce firmwaru stejné
  napětí 581,25 mV. PLL GPU `0x11430120` = `0xa0a00412` (260 MHz).
- `troika-clk-test` +1 (celkem 7 ladicích): soubor debugfs
  `acpmdbg_g3d_khz`, přijme jen 260000 a 338000, sám nic neposílá.
- Sestaven `out/troika-test-ACPM5.img` (sha256 začíná `5c25f0d9`), postup TESTING.md 5y.
- Návrh a rizika: FINDINGS.md 24. Souhlas zatím není.
- Necommitnuto: FINDINGS kap. 23 a 24, TESTING 5x a 5y, PROGRESS,
  `test-initramfs/init`; `kernel-patches/` čeká na export (89, ladicí 7).

### 2026-11-12 20:30 – test ACPM5 prošel: první změna frekvence

Telefon restartován do fastbootu (na pokyn uživatele), uživatel spustil
ACPM5. Výchozí stav zkontrolován, souhlas získán přes AskUserQuestion
(„Ano, všechny tři kroky“). Odeslány tři žádosti: 260000, 338000, 260000;
PLL GPU `0xa0a00412` → `0xa0680411` → `0xa0a00412`. Telefon v pořádku, běží
dál v ACPM5 na 260 MHz. Necommitnuto (FINDINGS 23 a 24, TESTING 5x a 5y,
PROGRESS, init; export patchů 89 + 7).

Další možné kroky (každý se souhlasem): stupeň GPU s jiným napětím,
frekvence procesoru; pak čistý ovladač hodin přes ACPM a škálování GPU
v panfrostu. Před balíčkem vyřešit soubor modulu v initramfs (23.2).

### 2026-11-12 20:31 – commit a push (ACPM4, ACPM5)

Na pokyn uživatele: `kernel-patches/` znovu vyexportováno (89),
`kernel-patches-debug/` (7), hlavní repozitář commitnut a pushnut. pmaports
beze změny: balíček kernelu pkgrel 6 je stále nesestavený a chybí mu commity
ACPM a TMU, volby `EXYNOS_MBOX=y`, `EXYNOS_ACPM_PROTOCOL=y`,
`EXYNOS9610_THERMAL=y` a soubor modulu FVP v initramfs (FINDINGS.md 23.2).
Telefon běží v ACPM5, GPU na 260 MHz.

### 2026-11-12 20:37 – návrh vyšších stupňů GPU, image ACPM6

- Průzkum (jen čtení): větev GPU je `BUCK5` (`vdd_g3d`, DT 0,5 až 1,2 V),
  napětí v registru `0x21` PMIC (adresa 1), `300000 + kód × 6250` µV;
  původní kernel ho čte přes firmware kanálem 2. Tabulka GPU v DT telefonu
  chce pro stupně nad 764 MHz sběrnici paměti na 2093 MHz, proto návrh
  končí na 764 MHz.
- `troika-clk-test` +1 (celkem 8 ladicích): `acpmdbg_g3d_khz` přijme sedm
  frekvencí (260 až 764 MHz), nový `acpmdbg_buck5_uv` jen čte.
- Sestaven `out/troika-test-ACPM6.img` (sha256 začíná `598aaa49`), postup
  TESTING.md 5z. Souhlas zatím není.
- Necommitnuto: FINDINGS kap. 25, TESTING 5z, PROGRESS;
  `kernel-patches-debug/` čeká na export (8).

### 2026-11-12 20:40 – test ACPM6 prošel: stupně GPU do 764 MHz, napětí

Telefon restartován do fastbootu (na pokyn uživatele), uživatel spustil
ACPM6. Souhlas přes AskUserQuestion („Ano, celý postup“). Čtení napětí
funguje; 12 žádostí o frekvenci nahoru a zpět, napětí na každém stupni
přesně podle tabulky firmwaru (581,25 až 743,75 mV). Postup se jednou
zastavil na mé příliš přísné kontrole PLL (384,8 vs. 385 MHz), pak
pokračoval s tolerancí 1 MHz. Telefon běží v ACPM6 na 260 MHz.
Necommitnuto: FINDINGS kap. 25, TESTING 5z, PROGRESS; export ladicích
patchů (8).

Další možné kroky: (1) Phosh s GPU na vyšší pevné frekvenci – skutečná
zátěž (místní kernel + soubor modulu v initramfs pmOS; nový souhlas);
(2) čistý ovladač hodin přes ACPM a škálování GPU v panfrostu (OPP tabulka);
(3) procesor. Před balíčkem vyřešit soubor modulu v initramfs (23.2).

### 2026-11-12 20:41 – commit a push (ACPM6)

Na pokyn uživatele: patche znovu vyexportovány (89 čistých, 8 ladicích),
hlavní repozitář commitnut a pushnut. pmaports beze změny. Telefon běží
v ACPM6, GPU na 260 MHz.

### 2026-11-12 20:44 – příprava zkoušky GPU3 (Phosh, vyšší frekvence GPU)

- `tools/make-local-pmos-bootimg.sh` připojuje soubor modulu FVP za
  initramfs jako druhý archiv cpio.
- Sestaven `out/troika-test-GPU3.img` (sha256 začíná `d229ec68`) z BL3
  a místního kernelu (`troika-clk-test`). Obsah ověřen jen na PC.
- Postup TESTING.md 5aa: uživatel přepíná frekvenci sám přes `sudo tee`.
- Necommitnuto: FINDINGS kap. 26, TESTING 5aa, PROGRESS, skript.

### 2026-11-12 21:05 – GPU3: první pokus bez modulu, oprava image

První GPU3 nabootoval do Phoshe, ale modul FVP se nenahrál (chyba -2):
připojený archiv cpio nebyl zarovnaný na 4 bajty a kernel ho odmítl
(FINDINGS.md 26.1). Skript opraven, GPU3 sestaven znovu (sha256 začíná
`62c99ea8`). Běží starý netboot server z 16:55 (drží port 9999; nový
`netboot serve` proto končí hláškou „nbd-server quit“) – telefon obsluhuje
ten starý. Čeká na nové spuštění GPU3 uživatelem. Necommitnuto.

### 2026-11-12 21:19 – GPU3 podruhé: přepínání v Phoshi prošlo

Opravený GPU3 nahrál modul FVP (kontrola 0 + dva soubory). Uživatel přepnul
455000, 764000, 260000 přes `sudo tee`, bez chyby; teploty 43 až 47 °C
(jednotlivá čtení). Čeká se na jeho hodnocení plynulosti. Netboot server
běží (spuštěn 21:10). Necommitnuto: FINDINGS kap. 26, TESTING 5aa, PROGRESS,
`tools/make-local-pmos-bootimg.sh`.

### 2026-11-12 21:20 – GPU3: uživatel potvrdil plynulejší prostředí na 764 MHz

Další krok (čeká na rozhodnutí uživatele): automatické škálování GPU –
čistý poskytovatel hodin GPU přes ACPM, tabulka OPP v DT (stupně a napětí
z tabulky firmwaru, do 764 MHz) a devfreq v panfrostu; chlazení přes
teplotní zóny. Necommitnuto od `7a10399`.

### 2026-11-12 21:21 – commit a push (GPU3)

Na pokyn uživatele hlavní repozitář commitnut a pushnut. Patche kernelu se
od `7a10399` neměnily (89 + 8). pmaports beze změny.

### 2026-11-12 21:30 – automatické škálování GPU: kód a image ACPM7

`troika-clk` +4 (binding hodin ACPM, žádost DVFS pro 9610, hodiny GPU
v `clk-acpm`, DT: GPU z hodin ACPM + tabulka OPP). `troika-clk-test`
přeskládána. `build/linux/.config`: `EXYNOS_ACPM_CLK=y`. Sestaveno bez
varování; `out/troika-test-ACPM7.img`. Telefon běží v GPU3 (Phosh).
Necommitnuto: FINDINGS kap. 27, TESTING 5ab, PROGRESS; export patchů 93 + 8.

### 2026-11-12 21:40 – test ACPM7 prošel, připraven GPU4

Souhlas přes AskUserQuestion („Ano, souhlasím“, jen pro ACPM7). Devfreq
navázán, sedm stupňů přes `min_freq` nahoru i dolů, PLL i napětí sedí.
Sestaven `out/troika-test-GPU4.img` (plný systém, stejný kernel). Telefon
běží v ACPM7 na 260 MHz. Necommitnuto: FINDINGS kap. 27, TESTING 5ab a 5ac,
PROGRESS; export patchů 93 + 8.

### 2026-11-12 21:55 – zkouška GPU4 v Phoshi

Uživatel spustil GPU4 a výslovně souhlasil s automatickým škálováním.
Hodnocení: plynulost dobrá, teploty do 49 °C. Necommitnuto: FINDINGS
kap. 27, TESTING 5ab a 5ac, PROGRESS; export patchů 93 + 8.

### 2026-11-12 21:56 – commit a push (ACPM7, GPU4)

Na pokyn uživatele: patche znovu vyexportovány (93 čistých, 8 ladicích),
hlavní repozitář commitnut a pushnut. pmaports beze změny. Telefon běží
v GPU4 (Phosh, netboot).

### 2026-11-12 21:58 – teplotní brzdění GPU: DT a image ACPM8

`troika-clk` +1 (práh `passive` 88 °C / 5 °C a mapa chlazení v zóně GPU),
`troika-clk-test` přeskládána, `CHECK_DTBS` bez nových nálezů, sestaven
`out/troika-test-ACPM8.img`. Telefon běží v GPU4 (Phosh). Necommitnuto:
FINDINGS kap. 28, TESTING 5ad, PROGRESS; export patchů 94 + 8.

### 2026-11-12 22:03 – test ACPM8 prošel

Zóna GPU s prahem 88 °C navázaná na chlazení; ruční stav chlazení 3 a 6
snížil GPU ze 764 na 455 a 260 MHz (napětí podle tabulky), po uvolnění
návrat. Telefon běží v ACPM8 na 260 MHz. Necommitnuto: FINDINGS kap. 28,
TESTING 5ad, PROGRESS; export patchů 94 + 8.

### 2026-11-12 22:07 – commit a push (ACPM8)

Na pokyn uživatele: patche znovu vyexportovány (94 čistých, 8 ladicích),
hlavní repozitář commitnut a pushnut. pmaports beze změny. Telefon běží
v ACPM8.

### 2026-11-12 22:10 – balíčky: firmware, kernel pkgrel 7, zařízení pkgrel 7

Ověřen původ a licence souboru modulu FVP (repozitář Motoroly, shodný
součet, žádná licence → `proprietary`). pmaports: tři místní commity
(`e70a787a6`, `18f3e8f2c`, `b651290cc`), `kconfig check` prošel. Postup
sestavení TESTING.md 5ae. Telefon běží v ACPM8. Necommitnuto v hlavním
repozitáři: FINDINGS kap. 29, TESTING 5ae, PROGRESS.

### 2026-11-12 22:12 – WiFi: zahájení, plán

Uživatel chce pokračovat WiFi. Sepsán výchozí stav a plán (FINDINGS.md 30).
Další krok: inventura jen pro čtení (krok 1). Souhlas se zápisy pro rádio
zatím není. Dávka balíčků (5ae) čeká na sestavení uživatelem; v hlavním
repozitáři necommitnuto od `51e0e72` (FINDINGS 29 a 30, TESTING 5ae,
PROGRESS), pmaports tři místní commity nepushnuté.

### 2026-11-12 22:19 – WiFi krok 1: inventura hotová

FINDINGS.md 30.3: uzly a adresy z původního DT, seznam zápisů při startu
rádia (PMU, SMC, BAAW, boot_cfg – k souhlasu později), firmware
`mx140.bin` (sestavení 6906, řada `leman_s620 S620_E9610_R3-1`, stejná jako
u gta4xl). Dvě cesty pro krok 2: A náš ovladač 10.17 ručně, B jejich 11.19
s hotovým patchem (potřeba sehnat odpovídající zdrojáky). Kopie firmwaru
pro rozbor jen ve scratchpadu, záloha nedotčena. Necommitnuto od `51e0e72`.

### 2026-11-12 22:30 – WiFi: zkratka B schůdná

Zdrojáky ovladače 11.19 staženy (`refs/gta4xl-kernel`, commit `8d1f7a68b0d0`),
272/272 součtů sedí, patche portu gta4xl aplikovány (`build/scsc`), překlad
proti našemu kernelu bez chyb (4 moduly). Hlavní číslo verze rozhraní
firmwaru se shoduje (14), vedlejší ne (14.5 vs 14.9). FINDINGS.md 30.4.
Další kroky: hlavička pro troiku, uzly DT + paměť, firmware do systému,
pak souhlas a první start rádia. Necommitnuto od `51e0e72`.

### 2026-11-12 22:42 – WiFi: příprava pro troiku, image WLAN1

`troika-clk-test` +1 (WIP uzly rádia v DT), `wifi/troika.h` + `wifi/Makefile`,
`tools/prepare-wifi.sh`, `tools/extract-wifi-fw.sh`, úprava
`tools/make-clk-test-bootimg.sh` (moduly a firmware do initramfs).
`build/linux/.config`: `CFG80211=y`, `RFKILL=y` (kvůli tomu už místní kernel
nesedí s moduly balíčku pro bezdrát; pro GPU zkoušky v Phoshi to nevadí).
Sestaven `out/troika-test-WLAN1.img`. Souhlas se startem rádia zatím není.
Necommitnuto: FINDINGS kap. 29 a 30, TESTING 5ae a 5af, PROGRESS, `wifi/`,
tři skripty; export ladicích patchů (9).

### 2026-11-12 22:59 – WLAN1 poprvé: chyba v DT (alignment), opraveno

Paměť rádia se nepřidělila kvůli špatnému počtu buněk `alignment`
(FINDINGS.md 30.7). Moduly nenačteny, souhlas nežádán. DT opraven, WLAN1
sestaven znovu (`f1c2e530`). Telefon běží ve starém WLAN1, čeká na nové
spuštění.

### 2026-11-12 23:23 – test WLAN1: rádio nastartovalo

Souhlasy přes AskUserQuestion („Ano, celý start“, „Ano, zapni wlan0“).
Moduly načteny, po dvou opravách cest (firmware tabletu natvrdo, konfigurace
naplocho) firmware rádia naběhl a služba WLAN se spustila; `wlan0` zapnuto
a zase vypnuto, bez hledání sítí. Zdrojové opravy v `tools/prepare-wifi.sh`
a `tools/extract-wifi-fw.sh`; image WLAN1 je potřeba pro další test sestavit
znovu (na telefonu jsou jen dočasné odkazy). Telefon běží ve WLAN1.
Necommitnuto: FINDINGS kap. 29 a 30, TESTING 5ae a 5af, PROGRESS, `wifi/`,
skripty; export ladicích patchů (9).

### 2026-11-12 23:25 – commit a push (balíčky, WiFi)

Na pokyn uživatele: patche znovu vyexportovány (94 čistých, 9 ladicích
včetně pracovního commitu WiFi), hlavní repozitář commitnut a pushnut;
pmaports pushnut (`e70a787a6`, `18f3e8f2c`, `b651290cc`), balíčky stále
nesestavené. Telefon běží ve WLAN1.

### 2026-11-12 23:31 – WiFi: image WLAN2 pro hledání sítí

Nový `tools/fetch-wifi-tools.sh` (wpa_supplicant + knihovny z chrootu, jen
čtení), `tools/make-clk-test-bootimg.sh` je přidává do initramfs. Sestaven
`out/troika-test-WLAN2.img` (sha256 začíná `e942ad2b`). Souhlas s vysíláním
zatím není. Necommitnuto: FINDINGS 30.9, TESTING 5ag, PROGRESS, dva skripty.

### 2026-11-12 23:34 – test WLAN2: hledání sítí prošlo

Souhlas přes AskUserQuestion („Ano, vyhledej sítě“). Moduly, `wlan0`,
`wpa_supplicant`, `wpa_cli scan`: 7 sítí (6× 2,4 GHz, 1× 5 GHz). Telefon
běží ve WLAN2 s rádiem zapnutým a `wpa_supplicant` spuštěným, připravený na
připojení uživatelem. Necommitnuto: FINDINGS 30.9 a 30.10, TESTING 5ag,
PROGRESS, `tools/fetch-wifi-tools.sh`, úprava `make-clk-test-bootimg.sh`.

### 2026-11-12 23:40 – test WLAN2: připojení a internet přes WiFi

Uživatel se připojil sám (telnet, `wpa_cli`). DHCP, ping na bránu
a `1.1.1.1`, stažení 10 a 50 MB – vše prošlo, bez varování ovladače.
Telefon běží ve WLAN2 připojený k síti uživatele. Necommitnuto: FINDINGS
30.9 až 30.11, TESTING 5ag, PROGRESS, `tools/fetch-wifi-tools.sh`, úprava
`make-clk-test-bootimg.sh`.

### 2026-11-12 23:42 – commit a push (WLAN2)

Na pokyn uživatele hlavní repozitář commitnut a pushnut. Patche kernelu
beze změny (94 + 9). pmaports beze změny (`b651290cc`). Telefon běží ve
WLAN2 připojený k síti uživatele.

### 2026-11-12 23:45 – WiFi: adresa MAC, původ firmwaru, návrh zařazení

FINDINGS.md 30.12. MAC přichází od bootloaderu na příkazové řádce kernelu
(hodnota nečtena); firmware rádia je veřejně v TheMuppets a shoduje se se
zálohou (6/6). Návrh čtyř kroků zařazení; čeká na rozhodnutí uživatele,
odkud brát zdrojáky ovladače pro balíček. Necommitnuto od `f447a14`.

### 2026-11-12 23:55 – WiFi do balíčků (příprava)

Uživatel rozhodl uložit ovladač do repozitáře. `wifi/scsc` (275 souborů),
`wifi/README.md`, `tools/update-wifi-pkg.sh`; archiv ovladače jako místní
zdroj balíčku kernelu (repozitáře nejsou veřejně stažitelné). `troika-clk`
+1 (uzly rádia), `troika-clk-test` zpět na 8 ladicích. pmaports: tři nové
místní commity (firmware rádia, kernel pkgrel 8, zařízení pkgrel 8).
Necommitnuto v hlavním repozitáři: FINDINGS 30.12 a 30.13, TESTING 5ae,
PROGRESS, `wifi/`, skripty; export patchů (95 + 8).

### 2026-11-13 00:15 – commit a push (WiFi v balíčcích)

Na pokyn uživatele: patche znovu vyexportovány (95 čistých, 8 ladicích),
hlavní repozitář commitnut a pushnut (včetně `wifi/scsc`), pmaports pushnut
(`8055cfd51`, `0d3ed3a57`, `426d6ea28`). Uživatel v 00:12 spustil sestavení
balíčku kernelu (asi 1,5 h); netboot server z 21:10 ještě běží – před
`install` ukončit. Telefon běží ve WLAN2.

### 2026-11-13 00:26 – zvuk: průzkum ABOXu (jen čtení)

Na podnět z vedlejšího vlákna (přání uživatele). FINDINGS.md kap. 31:
kodek CS47L35 a zesilovač CS35L41 na SPI, zvuk vede jen přes ABOX, původní
ovladač řídí přehrávání zprávami firmwaru Calliope; mainline pro ABOX nic
nemá, port gta4xl také ne. Překážka: napájecí větve (pravidlo o
regulátorech). Doporučené další kroky jen čtením. pmaports neměněn
(běží sestavení kernelu). Necommitnuto od `a251575`.

### 2026-11-13 00:36 – zvuk: rozbor firmwaru ABOXu (jen čtení)

FINDINGS.md 31.5: RDMA je hardwarový přenos z paměti, jeho registry
nastavuje firmware (kernel je jen čte); cesta bez firmwaru má šanci, ale
jistá není. Navržené tři kroky ověření na telefonu, první jen čtením.
Necommitnuto od `a251575` (FINDINGS kap. 31, PROGRESS).

### 2026-11-13 00:37 – zvuk: cesta B zvolena, krok 1 (čtení) hotov

Uživatel: „první metodou B a testováním“. ABOX po startu: doména zapnutá,
hodiny běží, registry čitelné a ve výchozím stavu (FINDINGS.md 31.6).
Upřesněno, co pravidlo o regulátorech znamená pro zvuk. Další: krok 2
(zápisy do registrů jednoho kanálu RDMA, sledovat `STATUS`) – potřebuje
návrh s konkrétními hodnotami a souhlas. Necommitnuto od `a251575`.

### 2026-11-13 00:56 – zvuk: návrh kroku 2 (nejdřív výpis z LineageOS)

Pro zápisy do RDMA chybí doložené hodnoty bitů; navržen krok 2a jen čtením:
výpis registrů ABOXu z LineageOS v klidu a při přehrávání (TESTING.md 5ah,
FINDINGS.md 31.7). Uživateli vysvětlen původ a důvod pravidla o
regulátorech. Necommitnuto od `a251575`.

### 2026-11-13 01:01 – sestavení kernelu pkgrel 8 selhalo při instalaci modulů, opraveno

Překlad kernelu i tří modulů WiFi v chrootu prošel; chyba až v `package()`:
moduly ovladače se instalovaly před moduly kernelu a `depmod` nenašel
`modules.builtin`. Pořadí v APKBUILD otočeno (pmaports `fef101563`, místní
commit). Sestavení je potřeba spustit znovu. Uživatel udělil pro zvuk
výjimku z pravidla o regulátorech (s důrazem na minimální riziko)
a souhlasí s výpisem registrů ABOXu z LineageOS.

### 2026-11-13 01:29 – PKG8 sestaveno, image zkontrolován

Uživatel sestavil kernel (podruhé, po opravě), firmware, zařízení, `install`
a `export`. Kontrola prošla (FINDINGS.md 32). Image uložen jako
`out/troika-test-PKG8.img`. Telefon běží ve WLAN2. Necommitnuto: FINDINGS
kap. 31 a 32, TESTING 5ah, PROGRESS; pmaports `fef101563` nepushnut.

### 2026-11-13 01:47 – test PKG8 prošel

Systém z balíčků nabootoval přes netboot; kontroly (modul FVP, GPU, teploty,
RO) v pořádku, WiFi v Phoshi funguje včetně stahování. Necommitnuto:
FINDINGS kap. 31 a 32, TESTING 5ae/5ah, PROGRESS; pmaports `fef101563`
nepushnut.

### 2026-11-13 01:51 – commit a push (PKG8, zvuk)

Na pokyn uživatele hlavní repozitář commitnut a pushnut (patche kernelu
beze změny, 95 + 8) a pmaports pushnut včetně opravy `fef101563`. Telefon
běží v systému z balíčků (PKG8, netboot).

### 2026-11-13 02:13 – zvuk: výpis registrů ABOXu z LineageOS

FINDINGS.md 31.9. Regmap pokrývá jen směrování/UAIF (77 registrů), RDMA ne,
kromě RDMA5 (komprimovaný kanál, při běžném přehrávání nulový). Rozdíl
klid/hraje: `QCHANNEL_DISABLE`, `ROUTE_CTRL0`, `SPUS_CTRL0`, `UAIF0_CTRL0/1`;
reproduktor jde přes UAIF0. Telefon běží v LineageOS, adb root. Necommitnuto
od `4c8f004`.

### 2026-11-13 02:20 – zvuk: komprimovaný kanál nepoužitelný, sonda do firmwaru

FINDINGS.md 31.10. LineageOS komprimované přehrávání nenabízí (RDMA5
zůstal nulový). Firmware Calliope obsahuje fyzické adresy registrů RDMA
a UAIF, rozbor je schůdný, ale na PC chybí disassembler pro ARM – čeká na
svolení uživatele k instalaci (`capstone` přes pip pro uživatele).
Telefon běží v LineageOS. Necommitnuto od `4c8f004`.

### 2026-11-13 02:24 – zvuk: rozbor firmwaru Calliope, pole registrů RDMA

FINDINGS.md 31.11. Uživatel nainstaloval `capstone`. Z firmwaru vyčtena
pole `RDMA_CTRL0`, pořadí nastavení kanálu a kód formátu (shodný
s `abox_get_format`); RDMA je čistě hardwarový přenos. Nový
`tools/fw-disasm.py`. Otevřené: adresa paměti bez IOMMU, bity 4/5,
hlasitost. Další: návrh pokusu na našem kernelu (zápisy do jednoho kanálu,
sledovat `STATUS`) a souhlas. Necommitnuto od `4c8f004`.

### 2026-11-13 02:27 – zvuk: návrh pokusu ABOX1

Nový `test-initramfs/abox-wr.c` (zápis jen do 14 vyjmenovaných registrů
ABOXu), přidán do testovacího image; sestaven `out/troika-test-ABOX1.img`
(sha256 začíná `2c48d29e`). Návrh kroků a hodnot v FINDINGS.md 31.12.
Souhlas zatím není. Necommitnuto od `4c8f004`: FINDINGS 31.9–31.12,
TESTING 5ai, PROGRESS, `tools/fw-disasm.py`, `abox-wr.c`, úprava
`make-clk-test-bootimg.sh`.

### 2026-11-13 02:31 – pokus ABOX1 proveden

Souhlas přes AskUserQuestion („Ano, proveď pokus“). Zápisy do RDMA1
a `SYSPOWER_CTRL` prošly, `STATUS` po zapnutí `0x80000000`, pozice 0;
registry vráceny. Řízení překladu adres ABOXu čte 4. Necommitnuto od
`4c8f004`.

### 2026-11-13 02:38 – zvuk: návrh pokusu ABOX2 (směrování + UAIF0)

FINDINGS.md 31.14. Piny UAIF0 (`gpb0-0..4`) jsou na telefonu vstupy, takže
ven nic nepůjde. Všechny potřebné registry už má `abox-wr` povolené, pokus
jde provést v běžícím ABOX1. Čeká na souhlas uživatele. Necommitnuto od
`4c8f004`.

### 2026-11-13 02:40 – pokus ABOX2 proveden

Souhlas přes AskUserQuestion („Ano, proveď pokus“). Zápisy prošly, piny
beze změny, pozice 0, vše vráceno. Necommitnuto od `4c8f004`.

### 2026-11-13 02:47 – zvuk: příprava ABOX3 (hodiny UAIF0)

`troika-clk-test` +1 (ladicí zapnutí bran UAIF0 jen s `pmos_test=abox3`),
sestaven `out/troika-test-ABOX3.img` (`6acfc0e5`). Čeká na restart telefonu
do fastbootu, spuštění a souhlas s opakováním pokusu ABOX2. Necommitnuto od
`4c8f004`.

### 2026-11-13 02:51 – zvuk: ABOX3 (hodiny běží, pozice stojí), příprava ABOX4

FINDINGS.md 31.17. Pokus s hodinami UAIF0 prošel, vše vráceno; pozice 0.
Porovnání s LineageOS: chybí `SPUS_CTRL1` `0x00370009` a časovač 0
(`0x604` = 1). `abox-wr` rozšířen o `0x204` a `0x604`, kopie na telefonu.
Moje chyba: do zkoušky nového nástroje jsem zařadil zápis (nic se
nezapsalo). Čeká na souhlas s ABOX4. Necommitnuto od `4c8f004`.

### 2026-11-13 02:52 – pokus ABOX4 (jen zápisy bez firmwaru) bez pohybu pozice

Souhlas přes AskUserQuestion. Vše vráceno. Cesta B pravděpodobně slepá;
rozhodnutí o dalším postupu je na uživateli. Necommitnuto od `4c8f004`.

### 2026-11-13 02:56 – zvuk: B2 (jen čtení) bez rozhodnutí

Řadič přerušení ABOXu i překlad adres jsou vypnuté; dvě čtení sysmmu skončila
chybou sběrnice (bez následků). Další zápisy by byly odhady. Necommitnuto od
`4c8f004`.

### 2026-11-13 03:06 – commit a push (zvuk)

Na pokyn uživatele hlavní repozitář commitnut a pushnut: ladicí patche
znovu vyexportovány (9), patche kernelu beze změny (95). pmaports beze
změny. Telefon běží v ABOX3, registry zvukové jednotky ve výchozím stavu.

### 2026-11-13 03:07 – zvuk, krok A: rozbor a plán (jen čtení)

FINDINGS.md kap. 33. Postup startu firmwaru z původního ovladače; hlavní
překážka je překlad adres `sysmmu_abox` (v7, mainline neumí; obejití je
nebezpečné, adresa firmwaru kolidovala s pamětí kernelu). Etapy A0–A3.
Commit `f1e3ca9` pushnut. Necommitnuto: FINDINGS kap. 33, PROGRESS.

### 2026-11-13 03:11 – zvuk, A0 hotovo; telefon se zasekl moje chyba

FINDINGS.md 33.4: mainline `exynos-iommu` v7 **umí** tuto jednotku (oprava
mého dřívějšího tvrzení). Při ověřování verze jsem četl `0x14920034` s
vypnutou bránou `aclk`; telefon se zasekl a restartoval do fastbootu (bez
následků). Telefon je teď ve fastbootu. Další: A1 (kód: `EXYNOS_IOMMU`,
uzel `sysmmu_abox`, zařízení abox, ladicí mapování). Necommitnuto: FINDINGS
kap. 33, PROGRESS.

### 2026-11-13 03:17 – zvuk, A1: příprava ABOX5

`troika-clk-test` +1 (DEBUG: stand-in master + uzel `sysmmu_abox` +
vyhrazené paměti), `EXYNOS_IOMMU=y` jen v místní konfiguraci, sestaven
`out/troika-test-ABOX5.img` (`cdc51d9d`). Telefon je ve fastbootu. Čeká na
souhlas se startem (zápisy ovladače do jednotky překladu). Necommitnuto:
FINDINGS 33.4–33.5, TESTING 5ak, PROGRESS, `test-initramfs/init`.

### 2026-11-13 04:12 – ABOX5: první start, chyba v DT opravena

Ovladač `exynos-sysmmu` odmítl uzel (`Failed to get device clock(s)`), do
jednotky se nezapsalo. Hodiny přejmenovány na `sysmmu`, ABOX5 sestaven znovu
(`6afc3a5b`). Telefon běží ve starém ABOX5. Necommitnuto od `f1e3ca9`.

### 2026-11-13 04:18 – zvuk A1: překlad adres běží, příprava ABOX6

Opravený ABOX5 běží (uživatel spustil sám): `sysmmu_abox` navázán, mapování
firmware a vyrovnávací paměti prošla. Zjištěno, že mainline při chybě
překladu paniká – přidána obsluha chyby; sestaven `out/troika-test-ABOX6.img`
(`d0d4736d`). Čeká na restart a souhlas s kontrolami (33.8). Necommitnuto od
`f1e3ca9`.

### 2026-11-13 04:29 – ABOX6 odhalil chybu v obsluze chyb, ABOX7 opraven

Obsluha chyby překladu se v ABOX6 nenastavila (výchozí doména to nedovolí) –
ověřeno z výstupu, kontrola nezačala. ABOX7 (`b2e4f857`) používá vlastní
doménu. Telefon běží v ABOX6, v registrech ABOXu nic. Necommitnuto od
`f1e3ca9`.

### 2026-11-13 04:32 – ABOX7: záporná kontrola, commit a push

Záporná kontrola bez chyby překladu; vše vráceno. Kladná nespuštěna.
Uživatel povolil automatický commit a push; tento commit je první takový.

### 2026-11-13 04:33 – troika-clk-test přeskládána na 96 commitů, DTB přeložen

Na žádost druhé relace: `troika-clk-test` přeskládána (`git rebase troika-clk`,
bez konfliktů, 12 ladicích commitů navíc), DTB přeložen (panel 64 × 148 mm je
v něm), `kernel-patches` (96) i `kernel-patches-debug` (12) znovu
vyexportovány. Image `ABOX*` v `out/` jsou ze starší báze a panel nemají;
Image kernelu v `build/linux` není přeložen znovu (jen DTB). Do pmaports
přibude 0096 při příští dávce balíčků.

### 2026-11-13 04:39 – zvuk A2: příprava ABOX8

PMU ukázalo, že procesor ABOXu je napájen a stačí bit 15 v `0x11862528`.
`troika-clk-test` +1 (debugfs `abox-fw/{status,load,start,stop}`),
`tools/extract-abox-fw.sh`, firmware v image, `out/troika-test-ABOX8.img`
(`3975f433`). Čeká na restart a souhlas po krocích (33.11). Uloženo
automaticky podle oprávnění uživatele.


- 2026-11-13: ABOX8 zůstal na „Booting kernel“: uzel `panel` z commitu
  cbeffdf (velikost displeje) nemá ovladač a fw_devlink držel framebuffer
  v odloženém probe (`wait for supplier /panel`). Oprava: `status =
  "disabled"` na uzlu panel (troika-clk 07be2ea, patch 0097); simpledrm
  velikost čte dál. ABOX9 (`1c2be410`) potvrzen: displej běží, simpledrm
  a /dev/fb0 jsou. Balíček jádra musí obsahovat 0096 i 0097. Zvuk A2:
  `load`/`start`/`stop` stále bez souhlasu.

- 2026-11-13: zvuk A2: `load` a `start` proběhly (souhlas), firmware
  běží, zapisuje na nenamapované IOVA 0xa0670000, záplava faultů zastavila
  síť, vypnuto dlouhým stiskem. ABOX10 (`613ac76e`) mapuje vlastní scratch
  a omezuje výpisy. `load`, `start`, `stop` pro ABOX10 potřebují nový souhlas.
