# nura-troika – mainline Linux on the Motorola One Action

> **English in short:** a work-in-progress port of Nura (postmarketOS) to the
> Motorola One Action (`troika`, Exynos 9609 – an underclocked Exynos 9610)
> running a **mainline Linux 7.3.3** kernel with out-of-tree patches. It boots
> from the stock bootloader through `fastboot boot` (nothing is ever flashed),
> runs the Phosh desktop with Panfrost GPU acceleration and works over Wi-Fi.
> The documentation is written in Czech; the tools, patches and commit
> messages of the kernel work are in English.

Port Nura (dříve postmarketOS) na **Motorola One Action** (`troika`,
SoC Exynos 9609 = podtaktovaný Exynos 9610, A/B oddíly) s **mainline kernelem
Linux 7.3.3**. Výsledek se spouští přes `fastboot boot` a běží z něj
grafické prostředí Phosh s akcelerovaným GPU a WiFi.

**Stav: rozpracováno, experimentální.** Telefon se dá používat jako
zkušební stroj po USB a WiFi, ne jako běžný telefon (viz [Co funguje](#co-funguje)).

## Bezpečnost – přečti si, než cokoli zkusíš

- **Nic se nenahrává do telefonu.** Všechny testy jdou přes `fastboot boot`
  (načte obraz do paměti; po restartu je telefon v původním stavu). Příkazy
  `fastboot flash`, `fastboot erase` a `pmbootstrap flasher` se v projektu
  **nepoužívají** a nedoporučují se.
- **Úložiště telefonu (UFS) je v systému jen pro čtení** – initramfs ho
  hlídá pravidlem a pojistkou při startu a systém běží po USB (netboot).
  Důvod: mainline ovladač UFS zatím nemá ověřenou bezpečnost zápisu.
- Před čímkoli dalším si udělej **úplnou zálohu oddílů** (`tools/backup-all-partitions.sh`)
  a ověř ji. Bez zálohy nezačínej.
- Napájecí větve, nabíjení a regulátory nejsou v device tree popsané
  záměrně (riziko poškození hardwaru). Jedinou výjimkou je pozvolný průzkum
  zvuku (viz níže).
- Vše je bez záruky a ověřeno jen na jednom kusu telefonu s odemčeným
  bootloaderem.

## Co funguje

Ověřeno na telefonu (podrobnosti, hodnoty a zdroje ve [`FINDINGS.md`](FINDINGS.md)):

| Oblast | Stav |
|---|---|
| Start mainline kernelu přes původní bootloader | funguje (řetěz [uniLoader](https://github.com/ivoszbg/uniLoader) → kernel + DTB) |
| Úložiště UFS | čtení funguje, zápis je záměrně zakázán |
| USB (režim zařízení) | funguje, síť přes USB a SSH |
| Dotyková obrazovka (Himax HX83112A) | funguje |
| Tlačítka, restart, vypnutí, restart do bootloaderu | funguje |
| Displej | framebuffer z bootloaderu (simpledrm); vlastní ovladač panelu chybí |
| Podsvícení (SGM37603A) | funguje, jas z Phoshe, zhasínání s obrazovkou |
| Grafické prostředí Phosh | funguje, GPU Mali-G72 přes Panfrost |
| Frekvence GPU | 260–764 MHz řízené přes firmware ACPM, teplotní brzdění od 88 °C |
| Teploty (TMU) | čtení tří čidel, zóny s ochranným vypnutím při 105 °C |
| WiFi | funguje (ovladač Samsung SCSC mimo strom), WPA2 na 5 GHz, hledání, připojení, internet |

## Co nefunguje nebo není hotové

- **Zvuk** – celý zvuk vede přes zvukovou jednotku čipu (ABOX), pro kterou
  mainline nic nemá. Rozebráno, zdokumentováno a částečně rozběhnuto
  (překlad adres hotový); bez jejího firmwaru přehrávání nejde.
  Viz kapitoly 31 a 33 ve [`FINDINGS.md`](FINDINGS.md).
- **Bluetooth**, **fotoaparáty**, **modem/SIM**, **SD karta**, **uspání**,
  **CPU frekvence** (zatím jen pevné z bootloaderu), **nabíjení a stav baterie**.
- **WiFi:** adresa MAC se předává z bootloaderu a nastavuje pravidlem udev
  (ověření chybí), chybí databáze zemí; WPA3 a delší provoz neověřeny.
- **Instalace na úložiště** – systém běží z počítače přes USB (netboot).

## Jak je to uspořádané

Projekt tvoří dva repozitáře:

| Repozitář | Obsah |
|---|---|
| **tento** (`nura-troika`) | dokumentace, patche kernelu, nástroje, testovací initramfs, ovladač WiFi |
| [`pmaports-troika`](https://github.com/letress/pmaports-troika) | balíčky pro postmarketOS/Nura: `device-motorola-troika`, `linux-motorola-troika-mainline`, `firmware-motorola-troika` |

Obsah tohoto repozitáře:

| Cesta | Co tam je |
|---|---|
| [`PROGRESS.md`](PROGRESS.md) | **začni tady** – aktuální stav, pravidla, deník práce |
| [`FINDINGS.md`](FINDINGS.md) | výsledky průzkumu a testů se zdroji hodnot (kapitoly po tématech) |
| [`TESTING.md`](TESTING.md) | postup jednotlivých testů a jak je spustit |
| `kernel-patches/` | čisté patche nad Linuxem 7.3.3 (hodiny, USB, UFS, dotyk, GPU, podsvícení, ACPM, TMU, WiFi uzly, …) |
| `kernel-patches-debug/` | ladicí patche (výpisy registrů a pokusné ovladače), do balíčku nejdou |
| `patches-src/` | původní patche z lore.kernel.org, ze kterých se vyšlo |
| `wifi/` | ovladač Samsung SCSC 11.19 přenesený na Linux 7.3 a nastavení pro troiku ([`wifi/README.md`](wifi/README.md)) |
| `tools/` | prostředí, sestavení testovacích obrazů, záloha a výpis z telefonu, rozbor firmwaru |
| `test-initramfs/` | testovací `init` a malé nástroje (čtení registrů, I²C, restart do bootloaderu) |

## Jak to vyzkoušet

Celé podrobně je v [`TESTING.md`](TESTING.md). Zhruba:

1. Telefon s **odemčeným bootloaderem** a **úplnou zálohou oddílů**.
2. Linux 7.3.3 + patche z `kernel-patches/` (`tools/update-kernel-pkg.sh`
   je zapíše do balíčku), toolchain podle `tools/env.sh`.
3. Balíčky z [`pmaports-troika`](https://github.com/letress/pmaports-troika)
   a `pmbootstrap` (`build`, `install`, `export`).
4. Obraz s `fastboot boot` a systém z `pmbootstrap netboot serve`.

Rychlé zkoušky bez balíčků: `tools/make-clk-test-bootimg.sh` (malý testovací
obraz s telnetem) a `tools/make-local-pmos-bootimg.sh` (plný systém s místním
kernelem).

## Jak se na projektu pracuje

- Žádná hardwarová hodnota (adresa, velikost paměti, GPIO, napětí) se
  nevymýšlí: každá má doložený zdroj, jinak je označená jako TODO.
  Zdroje: původní kernel a device tree z LineageOS, výpisy ze samotného
  telefonu, mailing listy, referenční port tabletu Galaxy Tab S6 Lite.
- Změny hardwaru se zkouší po malých krocích a každý se zapisuje do
  [`FINDINGS.md`](FINDINGS.md) i s tím, co se nepovedlo.
- Do repozitáře se nedostávají data z telefonu (`device-info/`, `photos/`):
  obsahují sériové číslo, adresy MAC a jiné identifikátory.

## Poděkování a zdroje

- Tým [postmarketOS](https://postmarketos.org) a komunita kolem Nury.
- [LineageOS](https://lineageos.org) – zdrojáky kernelu a device tree
  Motorola One Action, bez kterých by hodnoty nebyly z čeho vzít.
- Autor portu Exynos 9611 pro Galaxy Tab S6 Lite
  ([`gta4xl-linux`](https://github.com/arhammahajan/gta4xl-linux)) – návod
  pro ACPM, GPU a přenos ovladače WiFi.
- [uniLoader](https://github.com/ivoszbg/uniLoader) (Ivaylo Ivanov).
- Autoři mainline ovladačů Exynos, hlavně `exynos-acpm` (Linaro/Google)
  a podpory Exynos 9610 na [lore.kernel.org](https://lore.kernel.org).

## Licence

Patche kernelu jsou pod licencí GPL-2.0 jako Linux. Ovladač v `wifi/scsc/` je
kód Samsungu z kernelu Galaxy Tab S6 Lite (GPL-2.0, viz `wifi/scsc/COPYING`
a [`wifi/README.md`](wifi/README.md)). Skripty a dokumentace nemají zatím
vlastní licenční soubor. Firmware výrobce (rádio WiFi, modul ACPM) v
repozitáři **není**; balíček `firmware-motorola-troika` ho stahuje z veřejných
zdrojů a značí ho jako proprietární.
