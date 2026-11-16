# Plán dalších kroků (po otestování balíčku)

Stav k 2026-11-15: sestavuje se balíček (jádro pkgrel 9, zařízení 9, firmware 2) se
zvukem, displejem, baterií a Bluetoothem. Tento plán se spouští až po jeho otestování.
Nic z něj se zatím neprovádí.

Značení: **[A]** lze připravit a otestovat autonomně (čtení zdrojů, psaní a překlad kódu,
sestavení testovacích obrazů, testy jen se čtením, nebo testy bez rizika pro telefon);
**[D]** vyžaduje dohled nebo souhlas (zápis do PMU, regulátorů a GPIO, riziko zamrznutí,
fyzická akce, zvuk, pozorování displeje). Platí stálá pravidla: nic neflashovat,
záloha jen ke čtení, žádné hodnoty hardwaru bez zdroje, žádné změny regulátorů a
napájení bez výslovného souhlasu, před slyšitelným testem potvrzení „Jsem tu a poslouchám“.

## Stav po noční práci (2026-11-16)

Podrobně ve FINDINGS 35 (35.1a, 35.3a, 35.4a/b). Vše z noci je jen v testovací větvi `troika-clk-test` a v
testovacích obrazech; balíček ani čistá větev se nezměnily (kromě ověřeného PKG9).

- **Balíčky:** r9/r9/r2 ověřeny na telefonu (PKG9): displej, baterie, zvuk, Bluetooth, Wi-Fi.
- **cpufreq (jen dolů a zpět, se souhlasem):** HOTOVO. Žádosti o frekvenci přes ACPM fungují pro A53 (CPUCL0) i A73
  (CPUCL1): výkon škáluje lineárně s frekvencí (403–1053 MHz a 936–1456 MHz), teploty beze změny, bez zamrznutí; obě
  domény vráceny na výchozí. Nahoru nad frekvenci z bootloaderu se nešlo. Obraz `CPU3`.
- **SD karta (se souhlasem, karta vložená):** HOTOVO pro základ. Řadič, piny, LDO35/LDO2 (3,0 V) fungují; SDHC karta
  7,29 GiB se rozpoznala, první blok přečten, po `unbind` se regulátory vypnou. Sběrnice jen ~10 MHz (hodiny `ciu`),
  UHS nezkoušeno. Obraz `SD2`.
- **Fotoaparát (OV12A10, jen ID čipu):** HOTOVO. Napájení, MCLK 26 MHz a I2C fungují (po doplnění USI obalu
  CAMI2C_0 do DT), ID čipu `0x561241`. Obraz `CAM5`.
- **Uspání:** NEZKOUŠENO. V DT není RTC a jediný zdroj probuzení je tlačítko napájení (fyzický stisk), takže s2idle
  v noci jen s rizikem, že se telefon do rána neprobudí. Zkusit s tebou u telefonu.
- **Modem:** studie hotová (FINDINGS 35.5), doporučení „zatím nejít".

Co je dál (podle tebe): (1) přenést cpufreq, SD a zdrojové části z `troika-clk-test` do čisté větve a do balíčku (po
tvém souhlasu), (2) s2idle s dohledem a případně RTC, (3) cpufreq nahoru po stupních (s dohledem), SD rychlost
(`ciu` 800 MHz) a UHS, (4) ovladač senzoru OV12A10 a příjem obrazu (CSIS), (5) modem až nakonec.

## 0. Ověření balíčku (první krok)

Po `build`, `install` a `export` se projde `boot.img` ([A]) a ty ho spustíš
(`fastboot boot`, [D] kvůli pozorování). Kontrolní seznam:

- displej: obraz, zhasnutí a rozsvícení tlačítkem, jas;
- zvuk (reproduktor, [D] poslech), restart firmwaru po chybě;
- baterie: údaj v liště, `/sys/class/power_supply/battery`;
- Bluetooth: `hci0` po startu s adresou z `androidboot.btmacaddr`, hledání, připojení,
  audio (modul se načítá sám, `bt_addr` se zapíše před načtením);
- Wi-Fi beze změny, USB/SSH;
- `dmesg` bez nových chyb, firmware v `/usr/lib/firmware` (calliope, cirrus, bt.hcf).

Cokoli selže, opravím a znovu sestavím jedním kolem (jádro stavět nejvýš jednou za kolo).

## 1. Frekvence procesoru (cpufreq)

**Cíl:** procesor mění frekvence podle zátěže, telefon se méně hřeje a méně spotřebovává.

**Co víme:**
- Frekvence a napětí řídí firmware ACPM (domény CPUCL0 = A53 a CPUCL1 = A73 mají v
  jeho tabulkách čísla 2 a 3, FINDINGS 21.x). Totéž už funguje pro GPU
  (patche 0089–0094: OPP tabulka, thermální škrcení).
- Tabulky frekvencí a napětí (`fvmap`) jsou ve sdílené paměti ACPM (SRAM + 0x6000),
  frekvence čte i zapisuje ovladač `exynos-acpm` (dotaz `FREQ_GET`, žádost o nastavení).
- Hodiny CMU_CPUCL0 jsou v ovladači hodin, **CMU_CPUCL1 (A73) chybí**.
- Teploty (TMU) jsou čitelné, zóny s ochranným vypnutím 105 °C jsou v DT.

**Kroky:**
1. [A] Přečíst `fvmap` pro CPUCL0 a CPUCL1 (jen čtení ACPM SRAM, jako u GPU) a zapsat
   si tabulky frekvence/napětí do FINDINGS se zdrojem.
2. [A] Projít vendorový `exynos-cpufreq` / `cal-if` a doložit pravidla (společná
   frekvence clusteru, minimální a maximální frekvence 9609 oproti 9610: 2,2 GHz A73?).
3. [A] Napsat OPP tabulky a uzly `cpus` (`operating-points-v2`, `opp-shared`,
   `clocks = <&acpm_clk …>`) a cooling mapy navázané na TMU; přeložit, dtbs_check.
4. [A] Sestavit testovací obraz (initramfs, bez rootfs po USB): vypíše dostupné
   frekvence, `cpufreq` governor `userspace` zatím bez nastavování.
5. [D] Nejnižší stupeň clusteru (opatrný zápis žádosti o frekvenci), pak postupně
   nahoru; po každém kroku zátěž a teplota. Riziko: zamrznutí (pomůže podržení
   tlačítka). Souhlas: žádosti o frekvenci přes ACPM (stejný druh jako u GPU).
6. [D] Governor `schedutil`, zátěžový test (např. překlad, `stress`), kontrola teplot
   a stability po delší dobu (ty u telefonu nebo aspoň k dispozici pro restart).

**Hotovo když:** oba clustery mění frekvenci, teplota při zátěži drží, telefon se
nezasekne při 10+ minutách zátěže; v balíčku je jen DT a konfigurace, ne ladění.

## 2. Uspání (suspend) a nečinnost

**Cíl:** displej zhasnutý → procesor spí, telefon se probudí tlačítkem napájení.

**Co víme:** PSCI v DT je (`suspend 0xC4000001`), ale firmware **nepodporuje PSCI
SYSTEM_RESET** (kap. 10); skutečná podpora `SYSTEM_SUSPEND` neověřena. Ovladač
hodin už ukládá a obnovuje registry bran (patch 0041). `pinctrl_cmgp` nemá poduzel
`wakeup-interrupt-controller` (nález P1), takže probouzecí přerušení z CMGP nefungují.

**Kroky (od bezpečného po rizikové):**
1. [A] Zjistit, co kernel o uspání říká (`/sys/power/state`, `mem_sleep`, PSCI features),
   vyčíst z vendorového DT stavy `cpuidle` a doložit je.
2. [A] Opravit P1 (poduzel `wakeup-interrupt-controller`), označit tlačítko napájení
   jako `wakeup-source`; sestavit testovací obraz.
3. [A] `cpuidle` (PSCI stavy), bez zápisu do PMU: jen kontrola v `/sys/.../cpuidle`.
4. [D] `s2idle` (`echo s2idle > /sys/power/mem_sleep`, `echo freeze > /sys/power/state`):
   nejméně rizikové uspání, probouzí se tlačítkem. Testovat v testovacím obrazu
   s USB telnetem; **ne přes netboot**, protože kořenový systém po USB by se uspal.
5. [D] Ověřit probuzení periferií: USB, UFS, Wi-Fi, ABOX (po probuzení znovu nahrát
   firmware), displej a podsvícení, dotyk.
6. [D] Skutečné `mem` (PSCI `SYSTEM_SUSPEND`) jen pokud krok 1 ukáže podporu firmwaru;
   jinak končit u `s2idle` a cpuidle. Riziko zamrznutí je vyšší, vždy s tebou u telefonu.

**Hotovo když:** `s2idle` projde 20× za sebou s probuzením tlačítkem, po probuzení
funguje displej, dotyk, USB, zvuk a Wi-Fi.

## 3. Fotoaparát

**Cíl:** zachytit snímek z jednoho (zadního hlavního) senzoru.

**Co víme:** FINDINGS 34.3. Čtyři senzory (12a10, 12a10ff, 5e9, 16885c) každý na
vlastní sběrnici I2C, napájení přes GPIO spínače (AVDD/DVDD/IOVDD), MCLK z pinů
`gpc2-*`/`gpg3-5`, ISP řízený firmwarem. V mainline není ovladač těchto senzorů,
aktuátoru `lc898217`, přijímače CSI-2 (CSIS) ani ISP.

**Kroky:**
1. [A] Nastudovat vendorové `fimc-is-cis-12a10*.c` (registrové mapy, inicializace,
   režimy 12 Mpx) a dokumentaci MIPI/CSIS z vendorového kernelu; zapsat do FINDINGS.
2. [A] Napsat ovladač senzoru `12a10` (V4L2 subdev) podle vendorových zdrojů; zatím jen
   přeložit.
3. [A] Navrhnout přijímač: CSIS + D-PHY + DMA do paměti (registry z vendorových zdrojů),
   do DT i reserved-memory; přeložit.
4. [D] **Souhlas s konkrétními piny**: `AVDD_EN gpg2-6`, `DVDD_EN gpg2-4`,
   `IOVDD_EN gpg2-5`, reset `gpc1-0`, MCLK `gpc2-0` a I2C `hsi2c_12`. Bez něj se
   na senzory ani nesahá.
5. [D] Zapnout napájení a hodiny senzoru v pořadí z vendorového `power_up`, přečíst
   ID čipu přes I2C (jen čtení), zase vypnout. Riziko: zkrat/špatné pořadí napájení.
6. [D] Zachytit jeden snímek (raw) do paměti a uložit; kontrola očima (ty
   se podíváš na obraz), pak libcamera se softwarovým ISP, aktuátor a blesk (S2MU106).

**Hotovo když:** ID čipu se přečte a jeden raw snímek je rozpoznatelný; ostatní senzory,
autofokus, blesk a video jsou další kolo.

## 4. SD karta

**Cíl:** karta v čtečce se objeví jako blokové zařízení.

**Co víme (FINDINGS 14.4):** řadič `dwmmc2@13550000` (`samsung,exynos-dw-mshc`),
SPI 147, hodiny `ciu`, detekce karty `gpa0-7` (invertovaná). Napájení: `vmmc` = LDO35
a `vqmmc` = LDO2 v PMIC S2MPU09 přes ACPM; po startu jsou **vypnuté**. Dosud odloženo
kvůli pravidlu o napájení. Nově je tu hotový ovladač regulátoru přes ACPM
(`exynos-acpm-ldo`, použitý pro LDO42), který se dá znovu použít.

**Kroky:**
1. [A] Z vendorového ovladače S2MPU09 doložit registry a kódy napětí pro LDO35
   (obvykle 2,8–3,0 V) a LDO2 (1,8 V přepínané na 3,0 V u UHS); zapsat se zdrojem.
2. [A] Napsat uzly `vmmc`/`vqmmc` (stejný ovladač jako LDO42), uzel `dwmmc2` v
   `exynos9610.dtsi`, podporu řadiče v mainline (`dw_mmc-exynos`) a pinctrl; přeložit.
3. [A] Sestavit testovací obraz; bez karty jen zkontrolovat `dmesg` (řadič se
   zaregistruje, žádná karta).
4. [D] **Souhlas**: zapnutí LDO35 a LDO2 (regulátory). Výslovně potřebuju, protože
   to je výjimka z pravidla o regulátorech; hodnoty budou jen z vendorových zdrojů.
5. [D] Vložit kartu (ty), `fdisk -l`, čtení; pak zápis jen na zkušební kartu.
   Nejdřív 3,0 V (ne UHS), pak případně 1,8 V a vyšší rychlosti.

**Hotovo když:** karta se zjistí při vložení/vyjmutí, čte se i zapisuje spolehlivě
a po vyjmutí se LDO vypnou.

## 5. Telefonní modem

**Cíl:** zjistit, jestli je vůbec cesta k datům a hovorům v mainline.

**Co víme:** modem je Samsung Shannon (`modem_if`, sdílená paměť `0xf0000000`
o velikosti 0x8a00000, tabulka rezervované paměti ve FINDINGS); komunikace jde přes vendorový stack `cpif`/
`modem_if`, firmware modemu je binární a v systému potřebuje i uzavřený RIL.
V mainline není pro tento modem nic.

**Kroky:**
1. [A] Studie proveditelnosti (jen ze zdrojů): jaké rozhraní vendorový `cpif`
   používá (sdílená paměť, mailbox, IPC), jaké firmwary a oddíly modem potřebuje,
   co by bylo třeba napsat (ovladač `cpif`, protokol, rozhraní `/dev/ttyACM`/net),
   jestli existuje otevřená reimplementace. Výstup: rozhodnutí „jít/nejít“ s odhadem.
2. [A] Pokud „jít“: napsat ovladač jen pro načtení firmwaru a navázání komunikace;
   přeložit. Nejdřív bez spuštění.
3. [D] **Souhlas**: čtení stavu modemu a načtení firmwaru (spouští samostatný procesor
   modemu, vyžaduje zápisy do PMU a sdílené paměti). Spuštění jen na tvůj pokyn.
4. [D] SIM karta v telefonu (ty), registrace do sítě, volání/data až na konec.

**Realistický výhled:** nejobtížnější z celé řady, výsledek studie může být „zatím ne“.
Proto jde až poslední a nic dalšího na něm nezávisí.

## Pořadí a závislosti

1. Ověření balíčku (0).
2. cpufreq (1) a studie modemu (5/1) a studie fotoaparátu (3/1–3) a SD (4/1–3):
   jejich [A] části jsou nezávislé a dají se připravit paralelně, hotové do první
   zkoušky na telefonu.
3. Uspání (2) navazuje na cpufreq (stabilní frekvence) a potřebuje testy bez netbootu.
4. SD (4) a fotoaparát (3) čekají na tvé souhlasy (regulátory, GPIO).
5. Modem (5) až nakonec.

## Co potřebuju od tebe předem (souhlasy)

- zápisy žádostí o frekvenci přes ACPM pro CPUCL0 a CPUCL1 (1.5);
- uspání `s2idle` v testovacím obrazu a případně `mem` (2.4, 2.6);
- zapnutí LDO35 a LDO2 pro SD kartu (4.4);
- konkrétní GPIO a hodiny pro jeden senzor fotoaparátu (3.4), a pokud to bude potřeba
  pro další, každý zvlášť;
- případně načtení firmwaru modemu (5.3), až studie ukáže cestu.

## Pracovní zásady pro každé kolo

- Nejdřív testovací obrazy z lokálního kernelu (`fastboot boot`), balíček jednou za
  kolo (jádro cca 1,5 h).
- Co se osvědčí, jde do čisté větve `troika-clk` jako samostatný commit a do patchů;
  ladicí věci zůstávají jen v `troika-clk-test`.
- Po každém kroku commit a push, zápis do FINDINGS/PROGRESS.
