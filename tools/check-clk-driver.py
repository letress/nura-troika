#!/usr/bin/env python3
"""Porovná mainline ovladač hodin Exynos 9610 s tabulkami downstream kernelu.

Použití:
    tools/check-clk-driver.py [clk-exynos9610.c] [adresář cal-if/exynos9610]

Zdrojem pravdy jsou tabulky Samsungu z downstream kernelu (cmucal-sfr.c:
registry a bitová pole, cmucal-node.c: strom hodin). Skript nic neopravuje,
jen vypíše rozdíly:

  REG     offset registru v ovladači neodpovídá downstreamu
  FIELD   posun/šířka pole MUX/DIV/GATE neodpovídá downstreamu
  KIND    na registru je v downstreamu jiný druh uzlu
  DUP     dva uzly ovladače na stejném registru a bitu
  PARENT  rodič v ovladači neodpovídá stromu downstreamu
  ORPHAN  rodič není nikde registrován (ani jako vstup z DT)
  NREGS   registr použitý hodinami chybí v *_clk_regs (suspend/resume)
  INFO    pole hodin existuje, ale cmu_info na něj neodkazuje
  MISSING uzel downstreamu, který ovladač v daném CMU nemá
"""
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
drv_path = sys.argv[1] if len(sys.argv) > 1 else \
    os.path.join(ROOT, 'linux/drivers/clk/samsung/clk-exynos9610.c')
ds_dir = sys.argv[2] if len(sys.argv) > 2 else \
    os.path.join(ROOT, 'refs/downstream-kernel/drivers/soc/samsung/cal-if/exynos9610')

# Hodiny, které do CMU přicházejí zvenku (z DT nebo jako pevné hodiny desky).
EXTERNAL = {'oscclk'}


def split_args(s):
    """Rozdělí argumenty makra podle čárek na nejvyšší úrovni."""
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def macro_calls(text, name):
    """Vrátí seznamy argumentů všech volání makra name(...) v textu."""
    res = []
    for m in re.finditer(r'(?<![A-Za-z0-9_])' + name + r'\(', text):
        i, depth = m.end(), 1
        while depth:
            depth += {'(': 1, ')': -1}.get(text[i], 0)
            i += 1
        res.append(split_args(text[m.end():i - 1]))
    return res


def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    return re.sub(r'//[^\n]*', '', t)


# ---------------------------------------------------------------- downstream
sfr_c = strip_comments(open(os.path.join(ds_dir, 'cmucal-sfr.c')).read())
node_c = strip_comments(open(os.path.join(ds_dir, 'cmucal-node.c')).read())

ds_block = {a[0]: int(a[1], 16) for a in macro_calls(sfr_c, 'SFR_BLOCK')}
ds_sfr = {a[0]: (a[2], int(a[1], 16)) for a in macro_calls(sfr_c, 'SFR')}
ds_field = {a[0]: (int(a[1]), int(a[2]), a[3]) for a in macro_calls(sfr_c, 'SFR_ACCESS')}
ds_parents = {m.group(1): [p for p in re.split(r'[\s,]+', m.group(2)) if p]
              for m in re.finditer(r'enum clk_id (\w+)\[\]\s*=\s*\{([^}]*)\}', node_c)}


def field_key(field):
    """(blok, offset) registru, ve kterém pole leží."""
    return ds_sfr[ds_field[field][2]]


ds_node = {}   # jméno uzlu -> dict(kind, key, parents, shift, width)


def ds_add(name, kind, field, parents):
    """Uzel s polem EMPTY_CAL_ID nemá registr (virtuální uzel)."""
    if field in ds_field:
        sh, w, _ = ds_field[field]
        ds_node[name] = dict(kind=kind, key=field_key(field), parents=parents, shift=sh, width=w)
    else:
        ds_node[name] = dict(kind=kind, key=None, parents=parents, shift=None, width=None)


for a in macro_calls(node_c, 'CLK_MUX'):
    ds_add(a[0], 'mux', a[2], ds_parents[a[1]])
for a in macro_calls(node_c, 'CLK_DIV'):
    ds_add(a[0], 'div', a[2], [a[1]])
for a in macro_calls(node_c, 'CLK_GATE'):
    ds_add(a[0], 'gate', a[2], [a[1]])
for a in macro_calls(node_c, 'CLK_PLL'):
    ds_node[a[1]] = dict(kind='pll', key=field_key(a[4]), parents=[a[2]], shift=None, width=None,
                         lock=field_key(a[3]), type=a[0])
for a in macro_calls(node_c, 'FIXEDRATE'):
    ds_node[a[0]] = dict(kind='frate', key=None, parents=[], rate=int(a[1]))
for a in macro_calls(node_c, 'FIXEDFACTOR'):
    ds_node[a[0]] = dict(kind='ffactor', key=None, parents=[a[1]], ratio=a[2])

ds_by_key = defaultdict(list)   # (blok, offset) -> [jména uzlů]
for n, d in ds_node.items():
    if d['key']:
        ds_by_key[d['key']].append(n)
ds_sfr_by_key = defaultdict(list)
for n, k in ds_sfr.items():
    ds_sfr_by_key[k].append(n)

# ------------------------------------------------------------------- ovladač
raw = open(drv_path).read()
drv = strip_comments(raw)

defines = {m.group(1): int(m.group(2), 16)
           for m in re.finditer(r'#define\s+(\w+)\s+(0x[0-9a-fA-F]+)\b', drv)}
pnames = {m.group(1): re.findall(r'"([^"]+)"', m.group(2))
          for m in re.finditer(r'PNAME\((\w+)\)\s*=\s*\{([^}]*)\}', drv)}

arrays = {}   # jméno pole -> (typ, text)
for m in re.finditer(r'static const (?:struct (\w+)|unsigned long) (\w+)\[\]\s*__initconst\s*=\s*\{(.*?)\n\};',
                     drv, flags=re.S):
    arrays[m.group(2)] = (m.group(1) or 'regs', m.group(3))

infos = {}    # cmu -> {pole: hodnota}
for m in re.finditer(r'static const struct samsung_cmu_info (\w+)_cmu_info\s*__initconst\s*=\s*\{(.*?)\n\};',
                     drv, flags=re.S):
    infos[m.group(1)] = dict(re.findall(r'\.(\w+)\s*=\s*([^,]+),', m.group(2)))

problems = defaultdict(list)


def report(cmu, kind, msg):
    problems[cmu].append((kind, msg))


clocks = {}                  # jméno -> dict
by_key = defaultdict(list)   # (BLOK, offset) -> [jména]
for cmu, info in sorted(infos.items()):
    blk = cmu.upper()
    used = {v for k, v in info.items() if not k.startswith('nr_')}
    for arr, (typ, _) in arrays.items():
        if arr.startswith(cmu + '_') and typ != 'regs' and typ.endswith('_clock') and arr not in used:
            report(cmu, 'INFO', f'pole {arr} není v {cmu}_cmu_info')
    regs = set(re.findall(r'\b([A-Z][A-Z0-9_]+)\b', arrays.get(info.get('clk_regs', ''), ('', ''))[1]))

    def add(name, kind, reg, shift, width, parents, cid, registered):
        if name in clocks:
            report(cmu, 'DUP', f'jméno hodin "{name}" je definováno dvakrát')
        key = (blk, defines[reg]) if reg else None
        clocks[name] = dict(cmu=cmu, kind=kind, reg=reg, key=key, shift=shift, width=width,
                            parents=parents, id=cid, registered=registered)
        if key:
            by_key[key].append(name)
            if registered and reg not in regs:
                report(cmu, 'NREGS', f'{reg} ({name}) chybí v {cmu}_clk_regs')

    for arr, (typ, text) in arrays.items():
        if not arr.startswith(cmu + '_'):
            continue
        reg_ok = arr in used
        for a in macro_calls(text, 'MUX'):
            add(a[1].strip('"'), 'mux', a[3], int(a[4]), int(a[5]), pnames[a[2]], a[0], reg_ok)
        for a in macro_calls(text, 'DIV'):
            add(a[1].strip('"'), 'div', a[3], int(a[4]), int(a[5]), [a[2].strip('"')], a[0], reg_ok)
        for a in macro_calls(text, 'GATE'):
            add(a[1].strip('"'), 'gate', a[3], int(a[4]), 1, [a[2].strip('"')], a[0], reg_ok)
        for a in macro_calls(text, 'PLL'):
            add(a[2].strip('"'), 'pll', a[5], None, None, [a[3].strip('"')], a[1], reg_ok)
            clocks[a[2].strip('"')].update(lock=(blk, defines[a[4]]), type=a[0])
        for a in macro_calls(text, 'FRATE'):
            add(a[1].strip('"'), 'frate', None, None, None, [], a[0], reg_ok)
            clocks[a[1].strip('"')]['rate'] = int(a[4])
        for a in macro_calls(text, 'FFACTOR'):
            add(a[1].strip('"'), 'ffactor', None, None, None, [a[2].strip('"')], a[0], reg_ok)

# --- 1) registry: offset každého #define proti downstreamu (podle jména nebo místa)
cmu_of_define = {}
cur = None
for line in drv.split('\n'):
    m = re.match(r'#define\s+(\w+)\s+0x', line)
    if m:
        cmu_of_define[m.group(1)] = None
        continue
    m = re.match(r'static const unsigned long (\w+)_clk_regs\[\]', line)
    if m:
        for k, v in cmu_of_define.items():
            if v is None:
                cmu_of_define[k] = m.group(1)
for name, cmu in cmu_of_define.items():
    if not cmu or cmu not in infos:
        continue
    blk, off = cmu.upper(), defines[name]
    if name in ds_sfr:
        if ds_sfr[name] != (blk, off):
            report(cmu, 'REG', f'{name} = {off:#06x}, downstream {ds_sfr[name][0]} {ds_sfr[name][1]:#06x}')
    elif (blk, off) not in ds_sfr_by_key:
        report(cmu, 'REG', f'{name} = {off:#06x}: v downstreamu na tomto offsetu bloku {blk} žádný registr není')

# --- 2) pole, druh uzlu, duplicity, rodiče


def ds_to_drv(node):
    """Jméno hodin ovladače odpovídající uzlu downstreamu (podle registru)."""
    d = ds_node.get(node)
    if d is None:
        return None, f'{node}?'
    if d['key'] is None:
        n = node.lower()
        if n.startswith('oscclk_rco'):
            return n, n
        if n.startswith('oscclk_'):
            return 'oscclk', n
        return n, n
    cands = [c for c in by_key.get(d['key'], []) if clocks[c]['kind'] == d['kind']]
    if d['kind'] in ('mux', 'div'):
        cands = [c for c in cands if clocks[c]['shift'] == d['shift']] or cands
    if d['kind'] == 'gate':
        cands = [c for c in cands if clocks[c]['shift'] == d['shift']] or cands
    return (cands[0] if cands else None), node


for name, c in sorted(clocks.items()):
    cmu = c['cmu']
    for p in c['parents']:
        if p not in clocks and p not in EXTERNAL:
            report(cmu, 'ORPHAN', f'{name}: rodič "{p}" není nikde registrován')
        elif p in clocks and not clocks[p]['registered']:
            report(cmu, 'ORPHAN', f'{name}: rodič "{p}" je v poli, které se neregistruje')
    if not c['key']:
        continue
    same = [o for o in by_key[c['key']] if o != name and clocks[o]['kind'] == c['kind']
            and clocks[o]['shift'] == c['shift']]
    if same and name < same[0]:
        report(cmu, 'DUP', f'{name} a {same[0]} používají stejný registr {c["reg"]} a bit {c["shift"]}')
    nodes = ds_by_key.get(c['key'], [])
    match = [n for n in nodes if ds_node[n]['kind'] == c['kind']]
    if not match:
        kinds = ', '.join(f'{n} ({ds_node[n]["kind"]})' for n in nodes) or 'nic'
        report(cmu, 'KIND', f'{name} ({c["kind"]}, {c["reg"]}): v downstreamu je na registru {kinds}')
        continue
    d = ds_node[match[0]]
    if c['kind'] != 'pll' and (c['shift'], c['width']) != (d['shift'], d['width']):
        report(cmu, 'FIELD', f'{name}: pole [{c["shift"]}, šířka {c["width"]}], '
                             f'downstream {match[0]} [{d["shift"]}, šířka {d["width"]}]')
    if c['kind'] == 'pll':
        if c['lock'] != d['lock']:
            report(cmu, 'REG', f'{name}: locktime registr {c["lock"]}, downstream {d["lock"]}')
        if c['type'].lower() != d['type'].lower():
            report(cmu, 'FIELD', f'{name}: typ {c["type"]}, downstream {d["type"]}')
    exp = [ds_to_drv(p) for p in d['parents']]
    got = c['parents']
    if c['kind'] == 'mux':
        if len(exp) != len(got):
            report(cmu, 'PARENT', f'{name}: {len(got)} rodičů, downstream {match[0]} jich má {len(exp)}: '
                                  + ', '.join(e[1] for e in exp))
        for i, (e, g) in enumerate(zip(exp, got)):
            if e[0] != g:
                report(cmu, 'PARENT', f'{name}[{i}]: "{g}", downstream {e[1]} -> {e[0] or "v ovladači chybí"}')
    else:
        e = exp[0]
        if e[0] != got[0]:
            report(cmu, 'PARENT', f'{name}: rodič "{got[0]}", downstream {match[0]} <- {e[1]} '
                                  f'-> {e[0] or "v ovladači chybí"}')

# --- 3) uzly downstreamu, které v ovladači chybí (jen CMU, které ovladač má)
have = {c.upper() for c in infos}
for n, d in sorted(ds_node.items()):
    if not d['key'] or d['key'][0] not in have:
        continue
    if not [c for c in by_key.get(d['key'], []) if clocks[c]['kind'] == d['kind']]:
        report(d['key'][0].lower(), 'MISSING', f'{d["kind"]} {n} ({d["key"][1]:#06x}), rodič '
                                               + ', '.join(d['parents']))

# --- výpis
order = ['REG', 'FIELD', 'KIND', 'DUP', 'PARENT', 'ORPHAN', 'INFO', 'NREGS', 'MISSING']
total = defaultdict(int)
for cmu in sorted(problems):
    print(f'== CMU_{cmu.upper()} ({len([c for c in clocks.values() if c["cmu"] == cmu])} hodin)')
    for kind, msg in sorted(problems[cmu], key=lambda x: (order.index(x[0]), x[1])):
        total[kind] += 1
        print(f'  {kind:7} {msg}')
print('\nCelkem:', ', '.join(f'{k} {total[k]}' for k in order if total[k]) or 'bez nálezu')
print(f'ovladač: {len(clocks)} hodin v {len(infos)} CMU; downstream: {len(ds_node)} uzlů')
