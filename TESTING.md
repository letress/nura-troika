# TESTING – první boot mainline kernelu na troice

> **Stav 2026-11-10 večer: test K (kap. 4f) prošel.** Mainline kernel na troice
> nabootoval do testovacího initramfs: RAM, 8 CPU, konzole na displeji a všechna
> tři tlačítka fungují. Úvodní odstavec níže („nic ještě neběželo“) a kap. 4a–4e
> popisují cestu k tomu a nechávám je pro historii.

Stav k 2026-11-10. **Nic z tohoto návodu ještě neběželo na telefonu.** Image
jsou sestavené a zkontrolované jen na počítači; očekávané chování na displeji
je odvozené ze zdrojáků, ne z pozorování. Kernel ani initramfs jsem nemohl
spustit ani v emulátoru (QEMU tu není).

Co je na počítači ověřené:

- hlavičky všech tří image se shodují s `~/troika-backup/boot_a.img`
  (verze 1, adresy, velikost stránky, cmdline, os version),
- „second“ ve variantě A je bajt po bajtu původní DTB ze zálohy,
- uniLoader má arm64 hlavičku `Image` a obsahuje kernel, mainline DTB i initramfs,
- cílové adresy uniLoaderu (`0x88000000`), kernelu (`0x90000000`) a ramdisku
  (`0x84000000`) leží v první bance RAM, nepřekrývají se a jsou mimo všechny
  rezervované oblasti,
- dekódování událostí tlačítek v initramfs na syntetických datech.

## Zásady

- Test běží jen z RAM přes `fastboot boot`. Do telefonu se **nic nezapisuje**.
- V tomto návodu není žádný `fastboot flash`, `fastboot erase` ani
  `pmbootstrap flasher`. Jediná výjimka je kapitola 7 (obnova ze zálohy),
  kterou použiješ jen tehdy, kdyby telefon přestal bootovat – což by se při
  bootu z RAM stát nemělo.
- Testovací initramfs nepřipojuje žádné úložiště.
- Device tree nepovoluje regulátory, nabíjení, clocky ani USB. Kernel jen
  čte framebuffer, který nechal běžet bootloader, a tři GPIO tlačítek.

## 1. Co se testuje

Tři image v `out/`, vždy stejný kernel 7.3.3 a stejný testovací initramfs:

| Image | „kernel“ v boot.img | „second“ (DTB pro bootloader) | Co ověřuje |
|---|---|---|---|
| `troika-test-A.img` | uniLoader s vloženým kernelem a mainline DTB | **původní DTB** ze zálohy | jestli mainline nabootuje, když bootloader dostane DTB, na který mu sedne overlay |
| `troika-test-B.img` | totéž | mainline DTB | jestli bootloader snese DTB, na který overlay nesedne |
| `troika-test-C.img` | holý kernel `Image` | mainline DTB | jestli to jde úplně bez uniLoaderu |

Začni **A**. B a C mají smysl až potom; říkají, jak půjde udělat běžný
balíček pro pmbootstrap (kap. 6).

## 2. Příprava

1. Telefon nabitý aspoň na 50 %, LineageOS bootuje normálně.
2. Záloha existuje: `ls -l ~/troika-backup/` (6 image + `SHA256SUMS`).
3. Image už jsou sestavené v `~/nura-troika/out/`. Znovu je vytvoříš takto
   (netrvá dlouho, kernel je sestavený):

```bash
~/nura-troika/tools/make-test-bootimg.sh
```

   Kdybys potřeboval přestavět i kernel:

```bash
cd ~/nura-troika/linux && . ../tools/env.sh && make -j8 O=../build/linux Image dtbs
```

4. Kontrola součtů:

```bash
cd ~/nura-troika/out && sha256sum -c SHA256SUMS
```

## 3. Přechod do fastbootu

```bash
adb reboot bootloader
```

Počkej na obrazovku bootloaderu a ověř spojení:

```bash
fastboot devices
```

Volitelně ulož výpis proměnných (jen čte):

```bash
fastboot getvar all 2>&1 | tee ~/nura-troika/device-info/fastboot-getvar.txt
```

Výpis obsahuje sériové číslo; nesdílej ho bez pročištění.

## 4. Test A

```bash
fastboot boot ~/nura-troika/out/troika-test-A.img
```

Měř čas od stisku Enter a foť displej v každé fázi.

### Co čekat

| Fáze | Kdy | Na displeji | Znamená |
|---|---|---|---|
| 0 | hned | v terminálu `Sending 'boot.img' ... OKAY`, `Booting ... OKAY` | bootloader image přijal |
| 1 | 0–5 s | obrazovka bootloaderu, případně varování o odemčeném bootloaderu | bootloader zpracovává DTB a overlay |
| 2 | krátce | **černá obrazovka, vlevo nahoře drobný text**: ASCII logo „uniLoader“, `welcome to uniLoader ... on motorola-troika`, `Booting kernel...` | uniLoader běží, adresa framebufferu sedí |
| 3 | do ~5 s | obraz se přepne na **konzoli kernelu**: větší písmo, řádky s časem `[    0.000000] ...` rychle odrolují | kernel nabootoval, RAM a simple-framebuffer fungují |
| 4 | do ~10 s | rámeček `TROIKA MAINLINE TEST - initramfs bezi` a pod ním hodnoty | initramfs běží |
| 5 | dál | po stisku tlačítka řádek `KLAVESA code=115 STISK` apod. | gpio-keys fungují |

Text uniLoaderu ve fázi 2 má být **jantarově žlutý** (barva 255, 192, 0).
Když je světle modrý, je prohozená červená a modrá, tedy formát není
`a8r8g8b8`, ale `a8b8g8r8`.

Ve fázi 4 zkontroluj:

- `model: Motorola One Action`
- `soc_id: EXYNOS9610` (jakákoli revize)
- `cpu: 8 online`
- `MemTotal` kolem 3,6–3,7 GB (4027 MiB minus asi 237 MiB rezervací a kernel)
- `fb: 1080,2520 32 bpp`
- barevný test: pruhy v pořadí **červená, zelená, modrá**
- vstupní zařízení `gpio-keys`

Ve fázi 5 zmáčkni postupně Vol+ (115), Vol− (114) a Power (116). Každé
tlačítko má dát `STISK` a po puštění `UVOLNENI`.

### Ukončení

Podrž **Power + Vol−** asi 10 s, dokud se telefon nerestartuje. Naběhne
normálně do LineageOS, protože na úložišti se nic nezměnilo. `reboot` z
kernelu nejspíš fungovat nebude (PSCI je popsané ve verzi bez resetu).

### Když to dopadne jinak

| Co vidíš | Pravděpodobná příčina | Co poslat / zkusit |
|---|---|---|
| `fastboot boot` skončí `FAILED (remote: ...)` | bootloader `boot` nepodporuje nebo image odmítl | přesný text chyby; nic se nezměnilo |
| Zůstane logo bootloaderu, pak restart do LineageOS | uniLoader se nespustil (jiná adresa načtení, hlavička) nebo zasáhl watchdog | čas do restartu |
| Černá obrazovka bez textu, telefon „visí“ | uniLoader kreslí jinam, nebo bootloader displej vypnul | fotku; ukonči Power + Vol− |
| Text uniLoaderu je, konzole kernelu ne | kernel spadl před inicializací displeje | čas, fotku posledního stavu |
| Obraz je zkosený, pruhovaný | špatný `stride` | fotku |
| Obraz je posunutý nebo s artefakty nahoře | framebuffer nezačíná přesně na `0xed000000` | fotku |
| Barvy prohozené (pruhy modrá, zelená, červená) | formát je `a8b8g8r8` | stačí napsat |
| Konzole běží, ale telefon se po N sekundách sám restartuje | watchdog nastavený bootloaderem (mainline ho bez clocků neobsluhuje) | N v sekundách |
| `cpu:` méně než 8 | sekundární jádra se přes PSCI nespustila | fotku logu |
| `Zadne /dev/input/event*` | pinctrl nebo gpio-keys se nenačetly | fotku výpisu pod hláškou |
| Tlačítko hlásí `STISK` hned po startu bez mačkání | obrácená polarita | které |

## 4a. Bisekce: proč bootloader image A nespustil

Výsledek testu A (2026-11-10): terminál vypsal `Booting OKAY`, ale na telefonu
zůstalo fastboot menu beze změny a pak naběhl LineageOS. Kalibrační boot
původního image (`~/troika-backup/boot_a.img`) přitom menu nahradil logem
nebo varováním a naběhl Android. Bootloader tedy image A nejspíš odmítl ještě
před skokem do něj.

Image E1–E3 mají všechny původní DTB ve „second“ a liší se jen kernelem a
ramdiskem. Sleduj hlavně jedno: **nahradí fastboot menu logo/varování**
(image přijat), nebo **menu zůstane** (odmítnut)? Dívej se i na drobný text
v menu, bootloader tam může vypsat důvod.

| Image | Kernel | Ramdisk | Když je přijat, uvidíš |
|---|---|---|---|
| `troika-test-E1.img` | původní | původní | logo, pak normálně Android |
| `troika-test-E3.img` | uniLoader | původní | logo, pak černou obrazovku s textem uniLoaderu a dál fáze 2–5 z kap. 4 |
| `troika-test-E2.img` | původní | testovací | logo, pak nejspíš nic dalšího (původní kernel s cizím initramfs); ukonči Power + Vol− |

Pořadí: **E1, pak E3, pak E2.** Mezi běhy vždy zpět do fastbootu.

```bash
fastboot boot ~/nura-troika/out/troika-test-E1.img
```

```bash
fastboot boot ~/nura-troika/out/troika-test-E3.img
```

```bash
fastboot boot ~/nura-troika/out/troika-test-E2.img
```

Co výsledky znamenají:

| E1 | E3 | E2 | Závěr |
|---|---|---|---|
| odmítnut | – | – | vadí samotné přebalení: image z `mkbootimg` bez AVB patičky bootloader nebere; další image netřeba zkoušet |
| přijat | přijat | odmítnut | vadí testovací ramdisk; E3 je zároveň první skutečný test mainline kernelu |
| přijat | odmítnut | přijat | vadí uniLoader jako kernel (hlavička, `text_offset`) |
| přijat | odmítnut | odmítnut | vadí obojí |
| přijat | přijat | přijat | A měl projít; zopakovat A a sledovat menu |

## 4b. Test D: diagnostika displeje

Výsledky bisekce (2026-11-10): E1 prošel (logo, Android), E3 bootloader přijal
(logo), ale obraz na logu zamrzl. Bootloader tedy uniLoader spouští; image A
odmítl kvůli testovacímu ramdisku. Po skoku se ale na displeji nic nezmění,
takže buď uniLoader spadne dřív, než kreslí, nebo kreslí jinam, než displej
zobrazuje.

`troika-test-D.img` obsahuje diagnostický uniLoader. **Kernel nespouští.** Dělá toto:

1. obarví celou 64MiB oblast `bootloader_fb` (`0xec000000`) **purpurově**,
2. přečte z řadiče displeje adresu zobrazovaného bufferu (jen čtení čtyř
   registrů na každém ze čtyř DMA kanálů) a textovou konzoli dá tam,
3. vypíše přečtené hodnoty, buffery ostatních kanálů obarví (zeleně, bíle, šedě),
4. odpočítá 30 s po pěti sekundách a požádá firmware o restart
   (PSCI `SYSTEM_RESET`, volání firmwaru, ne zápis do registru).

Kromě RAM a toho jednoho volání nikam nezapisuje – žádné registry PMU, DECON
ani regulátory.

```bash
fastboot boot ~/nura-troika/out/troika-test-D.img
```

Foť nebo nahrávej od začátku.

| Co uvidíš po logu | Co to znamená | Co poslat |
|---|---|---|
| Černá obrazovka, jantarový text `TROIKA DIAG: console at 0x...`, řádky `G0 en=... addr=...`, odpočet, pak restart | vše běží; adresa na prvním řádku je skutečný framebuffer | fotku všech řádků |
| Totéž, ale místo restartu `PSCI reset returned` | firmware reset přes PSCI neumí; jinak vše v pořádku | fotku; ukonči Power + Vol− |
| Purpurová plocha (celá nebo část), bez textu | buffer leží uvnitř `bootloader_fb`, ale ne na jeho začátku | fotku, hlavně kde purpurová začíná a končí |
| Zelená, bílá nebo šedá plocha | displej čte z jiného DMA kanálu, než je výchozí | barvu |
| Logo beze změny, telefon se **sám restartuje** asi 35–60 s po objevení loga | uniLoader běží a PSCI reset funguje, ale na zobrazovaný buffer nedosáhl | čas do restartu |
| Logo beze změny, ani po 2 minutách žádný restart | uniLoader se nedostal ani k prvnímu kroku, nebo reset nefunguje | napiš to; ukonči Power + Vol− |

Řádky `reset in 30 s`, `25 s`, ... mají přibývat po pěti sekundách. Když
jdou výrazně rychleji nebo pomaleji, napiš jak – říká to něco o časovači.

## 4c. Další krok: E3 znovu, tentokrát s trpělivostí

Výsledek testu D (2026-11-10): uniLoader běží a kreslí. Framebuffer je na
`0xec000000`, 1080 × 2520, formát `a8r8g8b8` – přesně jak je v device tree.
**(Chybné čtení fotky: správně je `0xed000000`, viz kap. 4f.)**
Restart přes PSCI firmware neumí, proto test končí podržením tlačítek.

E3 je první skutečný test mainline kernelu (uniLoader + kernel + mainline DTB
+ testovací initramfs uvnitř, původní ramdisk a DTB jen kvůli bootloaderu).
Při prvním pokusu byl sledován jen krátce.

```bash
fastboot boot ~/nura-troika/out/troika-test-E3.img
```

Nahrávej video a **nech to běžet aspoň 3 minuty**. Čekám toto pořadí:

1. logo nebo varování bootloaderu,
2. černá obrazovka a text uniLoaderu (logo `uniLoader`, `welcome to uniLoader ... on motorola-troika`),
3. poslední řádek uniLoaderu `Booting kernel...` – tady může být pauza,
4. konzole kernelu a rámeček `TROIKA MAINLINE TEST` (fáze 3–5 z kap. 4).

Když to skončí na kroku 3 a dál se nic neděje, kernel nenaběhl nebo nekreslí;
i to je důležitý výsledek – pošli fotku posledního stavu a čas.

## 4d. Test F: krokovací boot

Opakovaný E3 (sledovaný přes 3 minuty) zůstal na logu, ačkoli diagnostický
uniLoader z testu D na stejnou adresu kreslí. Cesta kódem až po první text je
u obou stejná, takže rozdíl dělá nejspíš něco v čase: buď logo po skoku ještě
něco překreslí přes rychle vypsaný text, nebo se text jen mihne a kernel hned
po startu způsobí reset.

`troika-test-F.img` je E3 s jedinou změnou: uniLoader po úvodním textu
**20 s čeká s odpočtem**, vypíše `STEP 2: copying kernel, then jumping` a
teprve pak spouští kernel.

```bash
fastboot boot ~/nura-troika/out/troika-test-F.img
```

Nahrávej video od Enteru, nech běžet aspoň 3 minuty.

| Co uvidíš | Co to znamená |
|---|---|
| Logo celou dobu, žádný text | běžná cesta se k textu nedostane nebo ho něco trvale překrývá |
| Logo, přes něj po chvíli přibývají řádky `kernel hand-off in ... s` | úvodní smazání a text přepsalo logo; uniLoader běží |
| Černá obrazovka s textem a odpočtem, po `STEP 2` text zůstane stát | uniLoader v pořádku, kernel po skoku nic nenakreslil |
| Totéž, po `STEP 2` se vrátí logo | kernel způsobil reset a bootloader zůstal na logu |
| Totéž, po `STEP 2` konzole kernelu a rámeček testu | funguje to; E3 selhával jen časováním |

## 4e. Testy H a G: proč kreslí jen diagnostika

Test F (krokovací) ukázal tři minuty jen logo, žádný text. Jediná varianta
uniLoaderu, která na troice kdy něco nakreslila, je diagnostická (D). Od
ostatních se liší dvěma věcmi a tyhle dva image je oddělí:

| Image | Co to je | Co testuje |
|---|---|---|
| `troika-test-H.img` | krokovací varianta (F) s rozložením paměti jako D – jen výplň, žádný jiný kód | jestli rozhoduje rozložení (poloha zásobníku, `image_size` v hlavičce) |
| `troika-test-G.img` | diagnostika (D), která po 15s odpočtu spustí kernel | jestli rozhoduje úvodní krok diagnostiky; zároveň první test kernelu |

Začni **H**:

```bash
fastboot boot ~/nura-troika/out/troika-test-H.img
```

- Černá obrazovka, text, odpočet `kernel hand-off in 20 s … 5 s`, `STEP 2` →
  rozhodovalo rozložení. Sleduj dál, co udělá kernel (kap. 4, fáze 3–5).
- Jen logo → pusť G:

```bash
fastboot boot ~/nura-troika/out/troika-test-G.img
```

- U G čekám totéž co u D (černá, purpurová shora, text s hodnotami `G0 …`),
  pak odpočet 15 s, `STEP 2` a kernel.
- Jen logo i u G → úspěch testu D nebyl opakovatelný; pusť znovu
  `troika-test-D.img` a napiš, jestli tentokrát kreslil.

Vždy nahrávej video a nech běžet aspoň 3 minuty.

## 4f. Aktuální hlavní test: K

**Kapitoly 4a–4e jsou historie hledání chyby.** Příčina byla jinde: bootloader
zobrazuje buffer na `0xed000000`, ne na `0xec000000`, kam uniLoader i kernel
dosud kreslily. Potvrdila to ostrá fotka testu G (`console at 0xed000000`).
V testu D jsem adresu z rozmazané fotky přečetl špatně.

`troika-test-K.img` má adresu opravenou v uniLoaderu i v device tree kernelu:

```bash
fastboot boot ~/nura-troika/out/troika-test-K.img
```

Čekám: logo → černá obrazovka s textem uniLoaderu (`... on motorola-troika`,
`Booting kernel...`) → konzole kernelu → rámeček `TROIKA MAINLINE TEST` →
reakce na tlačítka. Podrobný popis fází a tabulka poruch jsou v kap. 4;
ukončení podržením Power + Vol−.

Kdyby po `Booting kernel...` text zůstal stát i teď, kernel opravdu nenaběhl
nebo nekreslí – pošli fotku a nech to běžet aspoň 2 minuty.

## 5. Testy A, B, C: co bootloader snese

Test K prošel s původním ramdiskem i původním DTB v boot image. Tyhle tři
image mění vždy jednu věc a říkají, jak jednoduchý může být balíček pro
pmbootstrap. Všechny mají opravenou adresu framebufferu (přestavěno 23:30).

| Image | „kernel“ | ramdisk v boot image | „second“ | Otázka |
|---|---|---|---|---|
| `troika-test-K.img` (prošel) | uniLoader | původní | původní DTB | – |
| `troika-test-A.img` | uniLoader | **testovací (malý)** | původní DTB | snese bootloader malý neandroidí ramdisk? |
| `troika-test-B.img` | uniLoader | původní | **mainline DTB** | snese DTB, na který mu nesedne overlay? |
| `troika-test-C.img` | **holý kernel** | testovací | mainline DTB | jde to bez uniLoaderu? |

Pořadí A, B, C; mezi běhy vždy restart (Power + Vol−) a zpět do fastbootu.

```bash
fastboot boot ~/nura-troika/out/troika-test-A.img
```

```bash
fastboot boot ~/nura-troika/out/troika-test-B.img
```

```bash
fastboot boot ~/nura-troika/out/troika-test-C.img
```

U A a B čekám totéž co u K: text uniLoaderu, konzole kernelu, rámeček.
U prvního pokusu s A zůstalo fastboot menu (bez loga); tehdy ale uniLoader
kreslil na špatnou adresu, takže to o ramdisku nic neříkalo.

U C uniLoader není, takže po logu má přijít rovnou kernel. Paměť, ramdisk a
příkazovou řádku do DTB doplňuje bootloader. Původní systém od něj dostává
`console=null`; kdyby to předal i teď, log kernelu vidět nebude, ale rámeček
ano (testovací init píše přímo na displej). Řádek `cmdline:` v rámečku ukáže,
co bootloader předal – ten prosím vyfoť.

Každý test nech běžet aspoň 2 minuty.

| Výsledek | Důsledek pro balíček |
|---|---|
| A projde | do boot image může jít libovolný initramfs, tedy i ten z pmOS |
| A neprojde | ramdisk musí bootloaderu „vonět“; bude potřeba zjistit čím (velikost, obsah) |
| B projde | do „second“ může jít mainline DTB; `deviceinfo_bootimg_dtb_second` funguje, jak je |
| B neprojde | ve „second“ musí zůstat původní DTB – balíček by ho musel někde vzít |
| C projde | uniLoader není potřeba; balíčky v `pmaports/` fungují skoro tak, jak jsou |
| C neprojde, A i B ano | kernel balíčku musí být zabalený v uniLoaderu |

## 5a. Test P: image poskládaný jako z balíčku

Výsledky A/B/C (2026-11-10): A prošel, B a C skončily `Booting FAILED (remote: '')`.
Bootloader tedy vyžaduje ve „second“ původní DTB; ramdisk může být libovolný.

Balíček proto musí vyrábět image, kde:

- na místě kernelu je uniLoader s vloženým kernelem a mainline DTB,
- ve „second“ je původní DTB,
- ramdisk je běžný initramfs v boot image a příkazová řádka taky.

Polohu ramdisku a příkazovou řádku zná jen bootloader a předává je ve svém
DTB. uniLoader je odtamtud nově přebírá (dřív ten DTB zahazoval). `P` je
přesně takový image, jen s testovacím initramfs:

```bash
fastboot boot ~/nura-troika/out/troika-test-P.img
```

Co čekám:

1. Logo, text uniLoaderu. Nové řádky: `inherited linux,initrd-start`,
   `inherited linux,initrd-end`, `inherited bootargs (… bytes)`.
   Kdyby tam bylo `no device tree from the previous bootloader`, převzetí
   nefunguje – vyfoť to.
2. Konzole kernelu a rámeček testu. Protože ramdisk tentokrát není vložený
   v uniLoaderu, rámeček se objeví jen tehdy, když převzetí polohy initrd funguje.
3. V rámečku je řádek `cmdline:` tentokrát dlouhý (přes několik řádků):
   argumenty bootloaderu a mezi nimi **`console=tty0 fbcon=font:TER16x32 pmos_test=1`**.
   Stačí mi vědět, jestli tam `pmos_test=1` je.

Pozor: příkazová řádka od bootloaderu obsahuje sériové číslo a MAC adresy
telefonu. Když budeš posílat fotku, tu část začerni nebo ořízni.

## 5b. Test P2: co bootloader předává v x0

Výsledek testu P (2026-11-11): uniLoader i kernel běží, ale kernel hlásí
`Initramfs unpacking failed: read error` a `Freeing initrd memory: 4K`.
Převzetí polohy ramdisku z DTB bootloaderu tedy neproběhlo. Podezření:
uniLoader se při startu kopíruje přes místo, kam si bootloader svůj DTB uložil.

`troika-test-P2.img` je P se dvěma změnami: uniLoader a kernel jsou v paměti
výš (`0x94000000`, `0x98000000`) a uniLoader před spuštěním kernelu **20 s
ukazuje**, co od bootloaderu dostal:

```bash
fastboot boot ~/nura-troika/out/troika-test-P2.img
```

Na obrazovce uniLoaderu vyfoť řádky:

- `x0 from bootloader: 0x…`
- `at x0: magic=… totalsize=…` (platný DTB má `magic=d00dfeed`)

Po odpočtu a `STEP 2` ještě krátce problikne `previous device tree at 0x…` a
buď `inherited linux,initrd-start` atd., nebo varování. Pak kernel:

- **Rámeček se objeví** a v `cmdline:` je `pmos_test=1` → převzetí funguje.
- **Znovu `Initramfs unpacking failed`** → nefunguje; rozhodnou hodnoty z fotky.

Příkazová řádka v rámečku obsahuje sériové číslo a MAC adresy; při posílání
fotky tu část ořízni.

## 6. Balíčky pmaports

V `~/nura-troika/pmaports` (větev `motorola-troika-mainline`):

- `device/testing/linux-motorola-troika-mainline` – kernel 7.3.3 + 17 patchů;
  navíc sestaví uniLoader (upstream + 2 patche) s vloženým kernelem a mainline
  DTB a nainstaluje ho do `/usr/share/u-boot/motorola-troika/uniLoader`.
- `device/testing/device-motorola-troika` – `deviceinfo` a původní DTB pro
  „second“ (`exynos9610-troika-vendor.dtb`, zkopírovaný z tvé zálohy).

`deviceinfo` říká boot-deploy, aby do boot image dal místo kernelu uniLoader
(`deviceinfo_bootimg_override_payload`) a do „second“ původní DTB.

Co je a není ověřené:

- `pmbootstrap kconfig check` prochází.
- Sestavení uniLoaderu z upstream tarballu a dvou patchů jsem vyzkoušel ručně
  mimo pmbootstrap (stejné kroky jako v APKBUILD).
- Balíčky samotné jsem přes pmbootstrap **nestavěl** (potřebuje sudo) a image
  z `pmbootstrap install` nikdo nezkoušel.

Sestavení balíčků:

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 linux-motorola-troika-mainline
```

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 device-motorola-troika
```

I kdyby vše fungovalo, pmOS skončí v initramfs: mainline zatím neumí UFS ani
SD, takže nemá odkud připojit kořenový systém.

## 7. Návrat k původnímu stavu

**Po `fastboot boot` není co vracet** – stačí restart (kap. 4, Ukončení).

Kdyby telefon přesto nenabíhal do LineageOS:

1. Podrž Power + Vol− do restartu a zkus znovu.
2. V bootloaderu zvol „Start“; případně recovery (Vol− dvakrát, Power).
3. Teprve když boot oddíl opravdu nefunguje, obnov ho ze zálohy. Tyhle
   příkazy **zapisují do telefonu**, spouštíš je ty a já je nezkoušel:

```bash
cd ~/troika-backup && sha256sum -c SHA256SUMS
```

```bash
fastboot flash boot_a ~/troika-backup/boot_a.img
```

```bash
fastboot flash dtbo_a ~/troika-backup/dtbo_a.img
```

```bash
fastboot flash vbmeta_a ~/troika-backup/vbmeta_a.img
```

   Pro slot B obdobně s `_b`. Aktivní byl při zálohování slot A.

Záloha obsahuje **LineageOS 22**, ne tovární firmware. K továrnímu firmwaru
Motoroly se dá vrátit jen oficiálním nástrojem Rescue and Smart Assistant;
jeho image nemám a postup jsem neověřoval.

## 8. Co mi pak poslat

- výstup `fastboot boot` z terminálu,
- fotky displeje z každé fáze (čitelné, klidně víc),
- časy: kdy se objevil uniLoader, konzole, rámeček; případně kdy se telefon restartoval,
- u fáze 5, co vypsalo každé tlačítko.
