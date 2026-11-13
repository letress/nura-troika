#!/usr/bin/env python3
"""Vypíše úsek firmwaru (Thumb) jako instrukce, u načtení konstant doplní hodnotu.

    tools/fw-disasm.py SOUBOR OD DO      (OD, DO šestnáctkově)

Potřebuje modul capstone (pip install --user capstone). Jen čte soubor."""
import sys, struct
from capstone import *
d = open(sys.argv[1], 'rb').read()
a, b = int(sys.argv[2], 16), int(sys.argv[3], 16)
md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
md.skipdata = True
for i in md.disasm(d[a:b], a):
    note = ''
    if 'pc' in i.op_str and i.mnemonic.startswith('ldr') and '#' in i.op_str:
        try:
            imm = int(i.op_str.split('#')[1].rstrip(']'), 16)
            lit = ((i.address + 4) & ~3) + imm
            note = '   ; =0x%08x' % struct.unpack_from('<I', d, lit)[0]
        except Exception:
            pass
    print('%05x: %-7s %s%s' % (i.address, i.mnemonic, i.op_str, note))
