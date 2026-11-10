# nura-troika

Port Nura (dříve postmarketOS) na Motorola One Action (troika, Exynos 9609)
s mainline kernelem.

- `PROGRESS.md` – stav práce a co zbývá (začni tady)
- `FINDINGS.md` – výsledky průzkumu se zdroji
- `TESTING.md` – postup testu přes `fastboot boot`
- `kernel-patches/` – větev `troika-mainline` nad Linuxem 7.3.3 jako patche
- `patches-src/` – původní patche z lore.kernel.org
- `tools/` – skripty (prostředí, sestavení testovacích image, záloha z telefonu)
- `test-initramfs/` – testovací init

Související repozitáře: `uniLoader-troika` (větev `troika`) a
`pmaports-troika` (větev `motorola-troika-mainline`).

Stažené zdroje, toolchain, výstupy buildu a data vyčtená z telefonu
v repozitáři záměrně nejsou (viz `.gitignore`).
