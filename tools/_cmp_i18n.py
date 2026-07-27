# -*- coding: utf-8 -*-
import json, os
from pathlib import Path
base = Path(r"e:/GTA/dev/XMenu/src/data/i18n")

def load_lang(L):
    keys = {}
    root = base / L
    for p in root.rglob("*.json"):
        if p.name == "index.json":
            continue
        try:
            d = json.loads(p.read_text(encoding="utf-8"))
        except Exception as e:
            print("fail", p, e)
            continue
        if isinstance(d, dict):
            for k, v in d.items():
                keys[k] = (str(p.relative_to(base)), v)
    return keys

zh = load_lang("zh")
en = load_lang("en")
for L in ("jp", "ru", "en"):
    cur = load_lang(L)
    miss = sorted(set(zh) - set(cur))
    print(f"=== {L} missing vs zh: {len(miss)} (zh={len(zh)} {L}={len(cur)})")
    by_file = {}
    for k in miss:
        f, v = zh[k]
        by_file.setdefault(f, []).append((k, v))
    for f, items in sorted(by_file.items()):
        print(f"  [{f}] {len(items)}")
        for k, v in items[:20]:
            print(f"    {k}: {v!r}")
        if len(items) > 20:
            print(f"    ... +{len(items)-20}")