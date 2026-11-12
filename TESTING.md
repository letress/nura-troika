# TESTING – první boot mainline kernelu na troice

> **Stav 2026-11-11: prošel i test P2 (kap. 5b)** – image poskládaný tak, jak ho má
> vyrábět balíček (uniLoader bez vloženého ramdisku, initramfs a příkazová řádka
> v boot image, původní DTB ve „second“).
>
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

## 5c. Test PM: boot image z balíčků (`pmbootstrap install`)

`out/troika-test-PM.img` je kopie `/tmp/postmarketOS-export/boot.img` z 2026-11-11
13:14 (sha256 začíná `556aae996ccec75c`). Nic se neflashuje.

Co jsem na něm ověřil rozborem (bez telefonu):

- hlavička v1, stránka 2048, stejné adresy jako P2;
- „kernel“ je uniLoader bajt po bajtu shodný s tím v balíčku kernelu r2;
- „second“ je původní DTB, shodný s P2 i se zálohou;
- ramdisk je initramfs pmOS (12,8 MB, zstd; kernel má `RD_ZSTD`).

Čím se liší od P2 (a co proto může dopadnout jinak):

- `os_version` v hlavičce je 0 (P2 mělo 15.0.0 / 2025-04). Jestli to bootloaderu
  vadí, nevím; projevilo by se to jako `Booting FAILED` v terminálu.
- cmdline nemá `fbcon=font:TER16x32`, takže text kernelu bude drobný.
- initramfs je 20× větší než testovací.

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-PM.img
```

Co čekat na displeji, když vše funguje:

1. logo bootloaderu, pak výpis uniLoaderu (bez 20s pauzy, takže krátce);
2. drobný text kernelu;
3. úvodní obrazovka pmOS (plymouth) s nápisem „Loading“, pak
   „Waiting for root partition“ asi 30 s;
4. chybová zpráva „root partition not found!“ a dál se nic neděje.

Bod 4 je **očekávaný konec**: kernel zatím neumí UFS ani SD, takže kořenový
oddíl najít nemůže. Jestli se plymouth na simple-framebufferu vůbec vykreslí,
nevím; kdyby ne, zůstane na displeji text kernelu nebo černá obrazovka.

Ukončení: podrž Power + Vol- asi 10 s, telefon se restartuje do LineageOS.

**Výsledek 2026-11-11: prošel.** `Booting OKAY`, kernel i initramfs pmOS běží,
po ~43 s skončí v debug shellu („postmarketOS debug shell“, `Device: Motorola One
Action (motorola-troika)`, `Kernel: 7.3.3-troika`) s klávesnicí na displeji.
Klávesnice nereaguje, protože dotyková vrstva zatím nemá ovladač. Místo bodu 4
tedy uvidíš textový výpis a výzvu `~ #`.

## 5d. Test CLK1: první boot s ovladačem hodin

`out/troika-test-CLK1.img` (z `tools/make-clk-test-bootimg.sh`): kernel z větve
`troika-clk-test`, jinak stejné složení jako P (uniLoader jako v balíčku, původní
DTB ve „second“, testovací initramfs). Nic se neflashuje.

Co je jinak než dosud: kernel poprvé **zapisuje do registrů hodin**. U každého
z devíti CMU zapne automatické hradlování (jeden registr, offset 0x800). Frekvence
ani napětí nemění, regulátorů se nedotýká. Co přesně udělají tři z pěti
zapisovaných bitů na tomto čipu, nevím (FINDINGS.md 11.5). Nejhorší, co čekám,
je zamrznutí nebo restart; po restartu je vše v původním stavu.

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-CLK1.img
```

Když vše funguje, uvidíš stejný rámeček testu jako u K a P2 a pod barevným
testem novou část:

```
--- HODINY ---
hodin: (několik set)  sirotku: (ideálně 0)
(případná jména sirotků)
 fout_shared0_pll               1599000000
 fout_shared1_pll               1332500000
 fout_mmc_pll                   (kolem 800000000)
 …dalších 10 řádků s frekvencemi…
clock-controller@12100000: option 0x… -> 0x…     (9 řádků)
pd dispaud: status 0x…                           (7 řádků)
```

Frekvence u `shared0`/`shared1` jsou to, co čekám podle tabulek ovladače; jestli
je bootloader nastavuje stejně, se teprve ukáže. Pak následuje test tlačítek.

**Vyfoť celý displej, ostře a zblízka** – potřebuju přečíst čísla (frekvence
a hexadecimální hodnoty). Klidně dvě fotky: horní a dolní polovinu.

| Co vidíš | Co to znamená |
|---|---|
| `Booting FAILED` v terminálu | bootloader image nepřijal (nečekám, složení je jako P) |
| logo nebo text uniLoaderu a dál nic | kernel zamrzl dřív, než se rozsvítila konzole – nejspíš při zápisu do hodin |
| text kernelu, ale žádný rámeček | kernel spadl později; vyfoť poslední řádky |
| rámeček bez části „HODINY“ | ovladač hodin se nezaregistroval |
| rámeček i „HODINY“ | úspěch; pošli fotku |
| displej zhasne nebo se rozpadne obraz | kernel vypnul něco, co displej potřebuje |
| telefon se sám restartuje | napiš, v jakém okamžiku |

Ukončení: podrž Power + Vol- asi 10 s.

**Výsledek 2026-11-11: prošel.** 566 hodin, 0 sirotků, frekvence odpovídají PLL,
displej i tlačítka fungují dál. Přečtené hodnoty jsou v FINDINGS.md 11.7.

## 5e. Test USB1: první pokus o USB (síť a telnet přes kabel)

`out/troika-test-USB1.img` (z `tools/make-clk-test-bootimg.sh USB1`): kernel
s hodinami a USB, testovací initramfs. Nic se neflashuje.

Co je jinak než dosud: kernel zapisuje do registru PMU `0x11860704` (zapnutí USB
PHY – máš to odsouhlasené) a resetuje USB PHY. Regulátorů se nedotýká a napětí
na USB nepouští.

**Kabel po celou dobu nevytahuj.** Datové vodiče vedou přes přepínač, který
kernel neovládá; po `fastboot boot` zůstává přepnutý na USB.

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-USB1.img
```

Na displeji pod barevným testem:

```
--- HODINY ---
hodin: 566  sirotku: 0
--- USB ---
pmu 704: 0x… -> 0x…          (po zápisu čekám bity 0, 1 a 16, tedy …10003)
pre  00=… 04=…               (5 řádků: registry PHY, jak je nechal bootloader)
post 00=… 04=…               (5 řádků: po inicializaci)
gadget NCM zapnut na 13200000.usb
Na pocitaci zkus: ping 172.16.42.1 a telnet 172.16.42.1
USB stav: configured          ← tohle je úspěch
```

Na počítači by se mělo objevit nové síťové rozhraní a dostat adresu 172.16.42.2:

```bash
ip -br addr
```

```bash
ping -c 3 172.16.42.1
```

`telnet` na počítači nainstalovaný nemáš, shell jde otevřít i přes `nc`:

```bash
nc 172.16.42.1 23
```

Až napíšeš, že telefon naběhl, zkusím se na něj z počítače podívat sám
a stáhnu si log kernelu, takže nemusíš nic opisovat.

| Co vidíš | Co to znamená |
|---|---|
| `USB stav: configured` a ping odpovídá | USB funguje |
| `USB stav: not attached` nebo `default` | PHY nebo přepínač D+/D− nepracuje; vyfoť řádky `pmu`, `pre`, `post` |
| `!! Zadny USB radic (UDC)` | ovladač dwc3 nebo PHY se nenavázal; vyfoť hlášky pod tím |
| text kernelu a pád (`Internal error`, `SError`) | čtení registrů PHY selhalo; vyfoť konec výpisu |
| logo nebo uniLoader a dál nic | kernel zamrzl brzy; napiš |

Ukončení: podrž Power + Vol- asi 10 s.

**Výsledek 2026-11-11: prošel.** Počítač telefon vidí (`cdc_ncm`), dostal adresu
172.16.42.2, ping i shell fungují. Příkaz na telefonu jde spustit z počítače:
`tools/phone-sh.py 'dmesg | tail'`. Podrobnosti: FINDINGS.md 12.7.

## 5f. Test NET1: pmOS po síti (netboot) a SSH

Cíl: plný pmOS s běžícím SSH, aniž by se cokoli zapsalo na úložiště telefonu.
Kořenový systém zůstane v souboru na počítači a telefon si ho připojí přes USB
(NBD). LineageOS v telefonu zůstává nedotčený. Nic se neflashuje.

Předpoklady, které jsou hotové: balíčky v pmaports mají hodiny a USB
(kernel r3, zařízení r2) a pmbootstrap má nastaveno
`extra_packages = postmarketos-mkinitfs-hook-netboot`.

1. Sestav balíčky (kernel zase asi 1,5 h):

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 linux-motorola-troika-mainline
```

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 device-motorola-troika
```

2. Vytvoř systém a vyexportuj boot image:

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

Hotovo 2026-11-11 17:25: boot image je zkontrolovaný (uniLoader = balíček r3,
původní DTB v „second“, `netboot.sh` a `nbd.ko` v initramfs) a uložený jako
`out/troika-test-NET1.img`.

3. V samostatné záložce terminálu spusť server s kořenovým systémem a nech ho běžet:

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve
```

Napíše „Waiting for postmarketOS device to appear...“ a čeká.

Server se na sudo heslo ptá dvakrát: hned na začátku (kopie obrazu, 1,4 GB) a
možná znovu ve chvíli, kdy telefon najde („Found postmarketOS device“), pokud
mezitím uběhlo víc než pár minut. Sleduj tu záložku; telefon se mezitím
připojuje pořád dokola, takže pozdě zadané heslo nevadí.

4. Nabootuj telefon (kabel nevytahuj):

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-NET1.img
```

5. Když server napíše „Found postmarketOS device, serving image...“ a telefon
doběhne, přihlas se heslem, které jsi zadal při `install`:

```bash
ssh user@172.16.42.1
```

**Výsledek 2026-11-11: prošel.** Telefon nabootoval, počkal v initramfs na
server (ten byl spuštěn až po bootu, nevadí to), po „Found postmarketOS device“
naběhl plný systém a `ssh user@172.16.42.1` se přihlásil („Welcome to Nura!“).
USB síť tedy přechod z initramfs do systému se systemd přežije.

Pozor: kořenový systém visí na kabelu. Vytažení kabelu nebo ukončení serveru
systém v telefonu zasekne; pomůže jen nový boot (držet vypínač, LineageOS v
telefonu je nedotčený). Změny v běžícím systému se zapisují do kopie obrazu
v `images_netboot` na počítači.

## 5g. Test NET2: kernel pkgrel 4 se zram

Cíl: ověřit, že po zapnutí `CONFIG_ZRAM` naběhne `postmarketos-zram-swap.service`
a systém není `degraded`. Postup je stejný jako u NET1, jen s novým kernelem.

1. Ukonči běžící `netboot serve` (Ctrl+C). Systém v telefonu tím zamrzne –
   telefon restartuj podržením vypínače, naběhne LineageOS.

2. Sestav kernel a vytvoř systém znovu:

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 linux-motorola-troika-mainline
```

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

Hotovo 2026-11-11 19:37: boot image je zkontrolovaný (uniLoader = balíček r4,
kernel se zram) a uložený jako `out/troika-test-NET2.img`.

3. Server spusť s `--replace`, jinak by dál nabízel starý obraz z NET1
   (zeptá se, jestli ho smí nahradit):

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve --replace
```

4. `adb reboot bootloader`, `fastboot boot ~/nura-troika/out/troika-test-NET2.img`,
   `ssh user@172.16.42.1` (ssh bude hlásit změněný klíč hostitele – je to nový
   obraz; starý záznam odstraní `ssh-keygen -R 172.16.42.1`).

**Výsledek 2026-11-11: prošel.** `systemctl is-system-running` vrací `running`,
žádná spadlá služba, `/dev/zram0` (zstd, 5,4 GB) je aktivní swap.

## 5h. Test UFS1: vnitřní úložiště jen pro čtení

Cíl: zjistit, jestli ovladač UFS (větev `troika-clk`, FINDINGS.md kap. 14.5)
naváže spojení s úložištěm a kernel uvidí disky a oddíly. Nic se nepřipojuje
a nic se nezapisuje do oddílů.

Co se při tomhle bootu stane navíc proti dřívějším testům (všechno dělá i
původní kernel při každém startu):

- zápis do PMU `0x11860724`, bit 0 (izolace UFS PHY) – povoleno 2026-11-11;
- zápis do sysreg FSYS `0x13411010`, bity 8 a 9 (koherence IO);
- dvě volání zabezpečeného firmwaru (SMC `FMP_SECURITY`, `SMU`), kterými se
  nastavuje ochranná jednotka UFS;
- reset řadiče a zařízení UFS, nové navázání linky, standardní inicializace
  zařízení (`fDeviceInit` a spol.).

Testovací initramfs každý nalezený disk hned přepne na „jen pro čtení“
(`blockdev --setro`), vypíše ho a dál na něj nesahá. Android a bootloader na
úložišti zůstávají, jak jsou. Regulátory se nepopisují, napájení úložiště
zůstává, jak ho nechal bootloader.

Postup:

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-UFS1.img
```

Na displeji hledej oddíl `--- UFS ---`:

- **úspěch**: řádky `sda: SAMSUNG … MiB ro=1 oddilu=N` (víc disků je v pořádku,
  UFS má několik logických jednotek);
- **neúspěch**: `!! Zadny disk` a pod tím poslední hlášky ovladače.

Kabel nech zapojený. Po naběhnutí USB (`USB stav: configured`) si celý log
stáhnu sám přes `tools/phone-sh.py`, fotka displeje není nutná – stačí napsat,
že telefon naběhl.

Kdyby telefon zůstal viset bez výpisu (černý displej, logo), podrž vypínač
a hlasitost dolů asi 10 s; naběhne LineageOS a nic se nezměnilo.

**Výsledek 2026-11-11: prošel napoprvé.** `sda` (Micron 128 GB, 42 oddílů),
`sdb`, `sdc`; režim `FAST series_B G_3 L_1`; hashe čtyř oddílů shodné se
zálohou; všechno `ro=1`. Podrobnosti ve FINDINGS.md 14.6.

### Test UFS2: totéž na uklizeném kódu

Po UFS1 jsem z větve odstranil úpravy pro druhou linku (řadič má jen jednu)
a vypnul nastavování masky synchronizace, které tady selhávalo. UFS2 je
stejný test na tomto kódu:

```bash
fastboot boot ~/nura-troika/out/troika-test-UFS2.img
```

Čekám stejný výpis jako u UFS1 a v logu už žádné `dme-set … failed`.

**Výsledek 2026-11-11: prošel**, přesně podle očekávání.

## 5i. Testy TS1 a TS2: dotyková obrazovka

Cíl: zjistit, jestli se ohlásí dotykový čip Himax HX83112A a jestli chodí
souřadnice dotyků (FINDINGS.md kap. 15). Bez regulátorů, bez zápisu do PMU.
Ovladač při startu krátce resetuje dotykovou část čipu displeje, stejně jako
původní kernel. UFS je v kernelu také, disky se zase hned přepínají na jen
pro čtení.

**TS1 (2026-11-11): čip odpověděl, ovladač se nenavázal – chyba v mém ladicím
výpisu.** První čtení 1 ms po resetu vrátilo ID `83112a` (shoduje se s tím, co
čekám). Můj ladicí kód pak četl registry firmwaru a ID kontroloval ovladač až
po nich; to už čip vracel samé nuly (firmware se po resetu teprve nahrává
z flash), takže ovladač skončil s `Unknown product id: 0`. Sběrnice I2C4
běží (378 kHz), žádná chyba přenosu. Log: `device-info/dmesg-mainline-TS1.txt`.

**TS2:** ladicí čtení je až po kontrole ID a registraci přerušení, s pauzou
300 ms. Ovladač sám je stejný jako v TS1.

**TS2 (2026-11-11): prošel.** Ovladač se navázal, přerušení chodí jen při
dotyku, tah z levého horního rohu do pravého dolního dal x 30 → 1016 a
y 27 → 2513 (FINDINGS.md 15.5). Výpis `DOTYK` na displeji v tomto image
nefunguje (chyba v testovacím initu, opravená pro další image); souřadnice
jsem četl přes telnet.

```bash
adb reboot bootloader
```

```bash
fastboot boot ~/nura-troika/out/troika-test-TS2.img
```

Na displeji hledej oddíl `--- DOTYK ---`:

- řádky `TSDBG …` (ID, `int_is_edge`, `rx/tx/maxpt`, `yres/xres`);
- pak „Sahni na displej“. Přejeď prstem z levého horního rohu do pravého
  dolního. Měly by naskakovat řádky `DOTYK x=… y=…`; vlevo nahoře malá čísla,
  vpravo dole kolem `x=1079 y=2519`. Výpis chodí po dávkách s malým zpožděním.
- `!! Dotykove zarizeni se neobjevilo.` = ovladač se nenavázal; pod tím budou
  jeho hlášky.

Napiš mi, jestli se čísla objevují a jestli rohy sedí. Kabel nech zapojený,
log si stáhnu sám. Kdyby po bootu zhasl obraz, napiš to také – log přes USB
půjde stáhnout i tak.

## 5j. Test RB1: restart a vypnutí z kernelu

Cíl: ověřit restart, restart do bootloaderu a vypnutí (FINDINGS.md kap. 16).
Kernel při tom zapisuje do PMU jen to, co jsi povolil: `0x11860400` (SWRESET),
`0x1186330c` (PS_HOLD_CONTROL, bit 8) a `0x1186081c` (důvod restartu).

**RB1 (2026-11-11): prošel.** Restart do bootloaderu, obyčejný restart
i vypnutí fungují (FINDINGS.md 16.4–16.5). Z testovacích image od RB1 výš se
dá do fastbootu dostat přes telnet příkazem `reboot-arg bootloader`, bez
držení tlačítek.

```bash
fastboot boot ~/nura-troika/out/troika-test-RB1.img
```

Na displeji hledej oddíl `--- RESTART A VYPNUTI ---` (čtyři řádky registrů,
tři řádky „ovladac navazan“) a na konci nabídku:

- **Vol+** – restart do bootloaderu. Očekávám menu fastbootu.
- **Power** – obyčejný restart. Očekávám start LineageOS.
- **Vol−** – vypnutí. Očekávám zhasnutí; se zapojeným kabelem se telefon
  nejspíš sám zapne do nabíjecí obrazovky.

Akce se provede po puštění tlačítka. Každá ukončí test, takže pro všechny tři
je potřeba image spustit třikrát. Doporučené pořadí: **Vol+** (když vyjde,
jsi rovnou ve fastbootu a můžeš image spustit znovu), pak **Power**, nakonec
**Vol−**.

Napiš mi ke každé akci, co telefon udělal. Když se místo restartu objeví
`!! Prikaz se vratil` nebo výpis chyby kernelu, pošli fotku a ukonči to
Power + Vol−.

## 5k. Test NET3: balíček kernelu pkgrel 5 (UFS, dotyk, restart)

Cíl: ověřit v plném systému to, co už prošlo v testovacích image: vnitřní
úložiště (jen pro čtení), dotykovou obrazovku a restart/vypnutí.

Kernel při tom zapisuje do PMU jen dříve povolené registry (`0x704`, `0x724`,
`0x400`, `0x330c`, `0x81c`). Plný systém i jeho initramfs uvidí úložiště
telefonu; balíček zařízení (pkgrel 4) proto přidává pravidlo udev, které jeho
disky hned po objevení přepne na jen pro čtení, a háček v initramfs, který to
před hledáním kořenového oddílu zkontroluje a jinak start zastaví
(FINDINGS.md 17.2). **První kontrola po přihlášení je, že pravidlo zabralo**
(krok 5).

1. Kernel r5 je sestavený (2026-11-12 00:52). První `install` + `export`
   (01:21) jsem k testu nepustil: initramfs měl úložiště zapisovatelné.
   Po opravě balíčku zařízení (pkgrel 4) stačí systém vytvořit znovu, kernel se
   nesestavuje (asi 7 minut):

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

2. Hotovo 2026-11-12 01:47: třetí `install` + `export` zkontrolován a uložen
   jako `out/troika-test-NET3.img` (sha256 začíná `d200dd47`; FINDINGS.md
   17.3).

   Kdyby se start telefonu zastavil s hlášením „Internal storage is not
   read-only“, je to ten háček: nic dalšího nedělej, vypni telefon (Power +
   Vol−) a napiš mi.

3. Server s novým obrazem:

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve --replace
```

4. Do fastbootu (`adb reboot bootloader` z LineageOS, nebo Power + Vol−) a:

```bash
fastboot boot ~/nura-troika/out/troika-test-NET3.img
```

```bash
ssh-keygen -R 172.16.42.1
```

```bash
ssh user@172.16.42.1
```

5. V telefonu (v okně ssh) jako první:

```bash
lsblk -o NAME,SIZE,RO,MOUNTPOINTS
```

   U `sda`, `sdb`, `sdc` a všech oddílů `sda1`–`sda42` má být `RO` = 1 a
   žádný přípojný bod. Kdyby někde bylo 0, nic dalšího nedělej a napiš mi.

   Potom ještě, jestli pravidlo zabralo už v initramfs (má vypsat 0; jiné
   číslo znamená, že disky musel přepnout až háček – napiš mi to):

```bash
sudo dmesg | grep -c "was not read-only"
```

6. Další kontroly: `systemctl is-system-running`, dotyk
   (`grep hx83112a /proc/interrupts` před dotykem a po něm; konzolové rozhraní
   na dotyk samo nereaguje) a nakonec `sudo reboot`.

**Výsledek 2026-11-12: prošlo** (FINDINGS.md 17.4 a 17.5). Všechny disky a
oddíly UFS RO = 1 a nepřipojené, systém `running`, dotyk dává přerušení,
`sudo reboot` restartuje. `sudo poweroff` z plného systému se nezkoušel.

## 5l. Test GUI1: Phosh přes netboot (softwarové vykreslování)

Cíl: zkusit grafické rozhraní na tom, co už funguje – obrazovka od bootloaderu
(`simple-framebuffer`, v kernelu `DRM_SIMPLEDRM=y`) a dotyk. Kernel ani
balíček zařízení se nemění; mění se jen volba rozhraní v pmbootstrapu
(`ui = phosh`, nastaveno 2026-11-12). Systém dál běží z PC přes netboot,
do telefonu se nic nezapisuje.

Co čekat: vykreslování počítá procesor (bez GPU), bude to líné. Displej nejde
zhasnout ani měnit jas. Nefunguje zvuk, WiFi, modem ani stav baterie.
**Nevyzkoušeno** – je možné, že se rozhraní napoprvé nespustí a bude potřeba
ladit (SSH přes kabel má fungovat i tak).

Phosh přináší `udiskie` (automatické připojování vyměnitelných disků). Disky
UFS zůstávají jen pro čtení (pravidlo udev + háček v initramfs, FINDINGS.md
17.2), takže zápis nehrozí; kontrola `lsblk` v kroku 5 ukáže, jestli se
některý oddíl telefonu nepřipojil aspoň pro čtení.

1. Vytvoř systém znovu (stáhne se hodně balíčků; obraz bude mít několik GB):

```bash
pmbootstrap -p ~/nura-troika/pmaports install --zap
```

   Heslo uživatele `user` zvol **jen z číslic** – zamykací obrazovka Phoshe má
   číselnou klávesnici.

   Kdyby instalace spadla na `losetup --json --list` (vada pmbootstrapu z
   FINDINGS.md 17.3): `pmbootstrap -p ~/nura-troika/pmaports shutdown` a
   `install` znovu, bez `--zap`.

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

2. Hotovo 2026-11-12 02:33: zkontrolováno a uloženo jako
   `out/troika-test-GUI1.img` (sha256 začíná `e9242d22`; FINDINGS.md 18.1).

3. Server s novým obrazem:

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve --replace
```

4. Do fastbootu a:

```bash
fastboot boot ~/nura-troika/out/troika-test-GUI1.img
```

5. Sleduj obrazovku (má se objevit přihlašovací obrazovka Phoshe) a zároveň
   se přihlas přes SSH a jako první zkontroluj úložiště:

```bash
ssh-keygen -R 172.16.42.1
```

```bash
ssh user@172.16.42.1
```

```bash
lsblk -o NAME,SIZE,RO,MOUNTPOINTS
```

   U `sda`, `sdb`, `sdc` a všech oddílů má být `RO` = 1. Další kroky dopíšu
   podle toho, co bude na obrazovce.

**Výsledek GUI1 (2026-11-12): prošlo** – Phosh nastartoval, dotyk funguje,
úložiště telefonu jen pro čtení a nepřipojené; vykreslování je trhané, ale
použitelné. Bez nastavení měřítka je vše malé (FINDINGS.md 18.2–18.3).

## 5m. Test GUI2: měřítko displeje z balíčku zařízení (pkgrel 5)

Cíl: přihlašovací obrazovka i plocha mají rozumnou velikost hned po startu,
bez ručního nastavení. Kernel se nesestavuje.

1. Sestav balíček zařízení zvlášť (aby se `install` vyhnul vadě s
   `losetup --json`), pak systém a export:

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 device-motorola-troika
```

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

   Kdyby `install` přesto spadl na `losetup --json --list`:
   `pmbootstrap -p ~/nura-troika/pmaports shutdown` a `install` znovu.

   Před sestavením musí být telefon vypnutý a netboot server ukončený
   (Ctrl+C), jinak `build` i `install` spadnou na `umount …/netboot`.

2. Hotovo 2026-11-12 03:12: zkontrolováno a uloženo jako
   `out/troika-test-GUI2.img` (sha256 začíná `184d4456`; FINDINGS.md 18.5).
   Měřítko je 3 (přání uživatele).

3. `netboot serve --replace`, `fastboot boot ~/nura-troika/out/troika-test-GUI2.img`,
   a podívej se, jestli je přihlašovací klávesnice přes celý displej.
   Přes SSH znovu `lsblk -o NAME,SIZE,RO,MOUNTPOINTS`.

**Výsledek GUI2 (2026-11-12): prošlo** – velikost vyhovuje („skvělá“), UFS
jen pro čtení a nepřipojené (FINDINGS.md 18.6).

## 5n. Test GPU1: ovladač panfrost se naváže na Mali-G72

Cíl: zjistit, jestli se GPU s popisem v DT (FINDINGS.md kap. 19) ohlásí
ovladači panfrost a na jaké frekvenci po bootloaderu běží. Místní testovací
image (kernel z větve `troika-clk-test`, testovací initramfs), žádný balíček.
Kernel při tom nemění napětí ani frekvenci a kvůli GPU nezapisuje do PMU.

Před testem: telefon vypnutý nebo ve fastbootu, netboot server ukončený.

1. Do fastbootu a:

```bash
fastboot boot ~/nura-troika/out/troika-test-GPU1.img
```

2. Na displeji bude textový výpis s oddílem `--- GPU ---`. Nic dalšího dělat
   nemusíš; napiš mi, že běží. Stav si přečtu sám přes telnet
   (`tools/phone-sh.py`) a pak telefon na tvůj pokyn pošlu zpět do fastbootu.

Kdyby displej zůstal černý nebo zamrzl, je to nejspíš pád kernelu při sahání
na GPU: podrž Power + Vol− asi 10 s a napiš mi to.

**Výsledek GPU1 (2026-11-12): prošlo** – panfrost se navázal na Mali-G72
r0p1 se třemi jádry, hodiny 260 MHz, vznikl `renderD128`, žádná chyba
(FINDINGS.md 19.6).

## 5o. Test GPU2: Phosh z netbootu s místním kernelem (s GPU)

Cíl: zjistit, jestli Phosh vykresluje na GPU, když displej dál obsluhuje
simpledrm. Systém na PC je stejný jako u GUI2; mění se jen kernel v boot
image (místní sestavení z větve `troika-clk`, FINDINGS.md 19.7).

1. Server (obraz z GUI2 už v něm je, není potřeba `--replace`):

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve
```

2. Do fastbootu a:

```bash
fastboot boot ~/nura-troika/out/troika-test-GPU2.img
```

3. Přihlas se přes SSH a zkontroluj úložiště a GPU:

```bash
ssh user@172.16.42.1
```

```bash
lsblk -o NAME,SIZE,RO,MOUNTPOINTS | head -4
```

```bash
ls /dev/dri/; grep panf /proc/interrupts
```

   Chvíli se pohybuj v rozhraní a druhý příkaz zopakuj: když čísla
   u přerušení přibývají, Phosh vykresluje na GPU.

4. Napiš mi, jestli se Phosh chová jinak (plynuleji) než u GUI2. Další
   příkazy doplním podle toho, co uvidím.

**Výsledek GPU2 (2026-11-12): prošlo** – Phosh vykresluje na GPU sám od sebe
(`panfrost-job` 15 638 přerušení), UFS jen pro čtení (FINDINGS.md 19.9).
První pokus selhal na neshodě BTF modulů z balíčku s místním kernelem
(FINDINGS.md 19.8); místní kernel proto potřebuje
`CONFIG_MODULE_ALLOW_BTF_MISMATCH=y`.

## 5p. Test BL1: čtení čipu podsvícení (bez zápisu)

Cíl: ověřit, že sběrnice HSI2C_3 v mainline funguje a čip SGM37603A na adrese
0x36 odpovídá; přečíst, jak ho nastavil bootloader (FINDINGS.md 20.2, 20.6).
Do čipu se nic nezapisuje.

1. Telefon do fastbootu (netboot server není potřeba) a:

```bash
fastboot boot ~/nura-troika/out/troika-test-BL1.img
```

2. Na displeji bude oddíl `--- PODSVICENI ---` se šesti registry. Napiš mi,
   že běží; hodnoty si přečtu přes telnet.

3. Zápis jasu zkusím až potom a až mi ho odsouhlasíš – napíšu předem, které
   hodnoty do kterých registrů.

**Výsledek BL1 (2026-11-12): prošlo** – sběrnice funguje, čip odpovídá,
kód jasu po bootloaderu je 0xCCC, žádná chyba (FINDINGS.md 20.7).

Pokusný zápis jasu (FINDINGS.md 20.8) prošel: displej se ztlumil a zhasl.

## 5q. Test BL2: ovladač podsvícení v Phoshi

Cíl: posuvník jasu v Phoshi mění jas displeje; zjistit, co se děje při
zhasnutí obrazovky. Místní kernel (větev `troika-clk` s ovladačem
`sgm37603a`), systém z netbootu jako u GPU2.

1. Server:

```bash
pmbootstrap -p ~/nura-troika/pmaports netboot serve
```

2. Do fastbootu a:

```bash
fastboot boot ~/nura-troika/out/troika-test-BL2.img
```

3. V Phoshi stáhni horní lištu a zkus posuvník jasu. Pak nech obrazovku
   zhasnout (nebo krátce stiskni Power) a podívej se, jestli podsvícení
   opravdu zhasne, nebo jen zčerná obraz.

4. Přes SSH:

```bash
ssh user@172.16.42.1
```

```bash
cat /sys/class/backlight/*/brightness /sys/class/backlight/*/max_brightness
```

**Výsledek BL2 (2026-11-12):** ovladač podsvícení funguje; posuvník jasu
v Phoshi jen s `PHOSH_DEBUG=fake-builtin` (pak funguje správně); při
nečinnosti podsvícení zhasne, při zhasnutí tlačítkem Power zůstává svítit
(FINDINGS.md 20.10–20.12).

## 5r. Test BL3: podsvícení zhasíná s obrazovkou

Cíl: po stisku Power zhasne i podsvícení a po probuzení se vrátí na stejný
jas. Místní kernel se změnou simpledrm (FINDINGS.md 20.12), systém z netbootu
(v něm zůstal doplněk služby z testu BL2, takže posuvník bude).

1. Netboot server nech běžet. Telefon pošli z Phoshe do fastbootu (zároveň
   se tím ověří restart do bootloaderu z plného systému):

```bash
sudo reboot bootloader
```

2. Na PC:

```bash
fastboot boot ~/nura-troika/out/troika-test-BL3.img
```

3. Po přihlášení: stiskni Power – má zhasnout obraz i podsvícení. Stiskni
   znovu – displej se má rozsvítit na původní jas. Zkus i posuvník jasu
   a zhasnutí nečinností.

**Výsledek BL3 (2026-11-12): prošlo** – Power zhasíná obraz i podsvícení,
probuzení vrací jas, posuvník funguje; `sudo reboot bootloader` z plného
systému funguje (FINDINGS.md 20.13).

## 5s. Test PKG6: balíček kernelu pkgrel 6 a zařízení pkgrel 6

Cíl: totéž, co prošlo s místními image (GPU, podsvícení, posuvník, zhasínání),
ale z balíčků, bez ručních zásahů v systému.

Před sestavením: telefon vypnutý (`sudo poweroff`), netboot server ukončený
(Ctrl+C).

1. Sestavení kernelu (asi 1,5 h), potom systém a export:

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 linux-motorola-troika-mainline
```

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 device-motorola-troika
```

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

2. Napiš mi; zkontroluju image a uložím ho jako `out/troika-test-PKG6.img`.

3. `netboot serve --replace`, `fastboot boot`, a ověřit: `lsblk` (RO = 1),
   `grep panf /proc/interrupts` (GPU), posuvník jasu bez ručního nastavení,
   zhasnutí tlačítkem Power.

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
- Balíčky bez hodin a USB (kernel r2, zařízení r1) se přes pmbootstrap sestavily
  a image z nich na telefonu nabootoval (test PM, 2026-11-11).
- Od 2026-11-11 15:05 je v pmaports kernel **r3** (52 patchů: základ + hodiny +
  USB) a zařízení **r2** (`clk_ignore_unused`). Stejný kód běžel na telefonu
  z místně sestaveného kernelu (testy CLK1 a USB1). Jako balíček se r3 ještě
  **nestavěl**; všech 52 patchů jde aplikovat na čistý strom 7.3.3 a
  `kconfig check` prochází.

Když pmbootstrap skončí chybou `sudo losetup --json --list` hned po řádku
`Build complete`, balíček je hotový. Selhal jen úklid, protože sudo se
nedočkalo hesla.

Sestavení balíčků:

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 linux-motorola-troika-mainline
```

```bash
pmbootstrap -p ~/nura-troika/pmaports build --arch aarch64 device-motorola-troika
```

Image z balíčků (nic neflashuje; `install` jen vytvoří soubory v pracovním
adresáři pmbootstrapu a zeptá se na heslo pro uživatele v pmOS):

```bash
pmbootstrap -p ~/nura-troika/pmaports install
```

```bash
pmbootstrap -p ~/nura-troika/pmaports export
```

Po `export` je boot image v `/tmp/postmarketOS-export/boot.img`. Oba příkazy
prošly 2026-11-11; rozbor image a test na telefonu jsou v kapitole 5c.
`pmbootstrap flasher` nespouštěj.

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
