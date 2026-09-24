import json
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
schema_path = root / "src" / "data" / "ui-schema.json"
deployed_path = root / "build" / "bin" / "XBase" / "Mods" / "XMenu" / "data" / "ui-schema.json"
ui_source = root / "src" / "ui" / "UiSchema.cpp"
controller_dir = root / "src" / "controllers"
i18n_dir = root / "src" / "data" / "i18n"
languages = ["zh", "en", "jp", "ru"]

live_registers = [
    r"registerLiveToggle\(\s*\"([^\"]+)\"",
    r"registerProofFlag\(\s*\"([^\"]+)\"",
    r"registerVehicleAttribute\(\s*\"([^\"]+)\"",
    r"RegisterLiveNumber\(\s*\"([^\"]+)\"",
]


def read(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def load_json(path: pathlib.Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def controller_text() -> str:
    text = ""
    for path in sorted(controller_dir.glob("*.cpp")):
        text += read(path)
    return text


def collect(pattern: str, text: str) -> set[str]:
    return set(re.findall(pattern, text))


def language_keys() -> dict[str, set[str]]:
    keys: dict[str, set[str]] = {}
    for code in languages:
        found: set[str] = set()
        for path in sorted((i18n_dir / code).glob("*.json")):
            try:
                data = load_json(path)
            except Exception:
                continue
            if isinstance(data, dict):
                found.update(data.keys())
        keys[code] = found
    return keys


def main() -> int:
    if not schema_path.is_file():
        print(f"缺少注册表文件 {schema_path}", file=sys.stderr)
        return 1
    if not ui_source.is_file():
        print(f"缺少解析器源码 {ui_source}", file=sys.stderr)
        return 1

    schema = load_json(schema_path)
    ui = read(ui_source)
    controllers = controller_text()

    states = collect(r'\{"(\w+)", &MenuState', ui)
    feature_caps = collect(r'\{"(\w+)", XBase::FeatureCapability', ui)
    domain_caps = collect(r'\{"(\w+)", XBase::Capability', ui)
    published = collect(r'"([A-Za-z]\w*)"', ui.split("static const char* const names[]")[1].split("};")[0])
    changes = collect(r'name == "([A-Za-z.]+)"', ui)
    actions = collect(r'id == "([A-Za-z0-9.]+)"', ui)
    sources = collect(r'RegisterSelectSource\("([^"]+)"', controllers)

    live: set[str] = set()
    for pattern in live_registers:
        live |= collect(pattern, controllers)

    errors: list[str] = []
    notes: list[str] = []
    counts: dict[str, int] = {}
    labels: list[tuple[str, str, str]] = []

    for tab in schema.get("tabs", []):
        for page in tab.get("pages", []):
            for section in page.get("sections", []):
                where = f"{tab['id']}/{section['id']}"
                for key in ("labelKey", "hintKey"):
                    if section.get(key):
                        labels.append((section[key], where, key))

                caps_here = []
                if section.get("capability"):
                    caps_here.append(section["capability"])
                caps_here.extend(c["capability"] for c in section.get("controls", []) if c.get("capability"))
                for name in caps_here:
                    if name not in feature_caps and name not in domain_caps:
                        errors.append(f"{where} 能力 {name} 不在能力映射表里")
                    if name not in published:
                        errors.append(f"{where} 能力 {name} 没有下发给网页端")

                for control in section.get("controls", []):
                    kind = control["kind"]
                    counts[kind] = counts.get(kind, 0) + 1
                    cid = control["id"]
                    if control.get("labelKey"):
                        labels.append((control["labelKey"], where, cid))

                    if kind == "action":
                        if cid not in actions:
                            errors.append(f"{where} 动作 {cid} 在 RunAction 里没有分支")
                        continue

                    if kind == "select":
                        if control.get("source") not in sources:
                            errors.append(f"{where} 下拉 {cid} 的来源 {control.get('source')} 没有登记")
                        continue

                    if control.get("state") and control["state"] not in states:
                        errors.append(f"{where} 控件 {cid} 的 state {control['state']} 不在解析表里")
                    if not control.get("state") and cid not in live:
                        errors.append(f"{where} 控件 {cid} 没有 state 也没有实时读写函数")
                    if control.get("onChange") and control["onChange"] not in changes:
                        errors.append(f"{where} 控件 {cid} 的 onChange {control['onChange']} 没有实现")

                    if control.get("visibleWhen"):
                        target = control["visibleWhen"].lstrip("!")
                        ids_here = {c["id"] for c in section.get("controls", [])}
                        if target not in ids_here:
                            errors.append(f"{where} 控件 {cid} 的 visibleWhen 指向了不在本分区的 {target}")

    keys = language_keys()
    for key, where, field in labels:
        if key.startswith("##"):
            continue
        missing = [code for code in languages if key not in keys[code]]
        if missing:
            notes.append(f"{where} 的 {field} {key} 缺少语言 {' '.join(missing)}")

    if deployed_path.is_file():
        if load_json(deployed_path) != schema:
            errors.append("产物里的注册表与源码不一致，需要重新构建")

    total = sum(counts.values())
    kinds = " / ".join(f"{name} {count}" for name, count in sorted(counts.items()))
    print(f"分区 {sum(len(p['sections']) for t in schema['tabs'] for p in t['pages'])}  控件 {total}（{kinds}）")

    for note in notes:
        print(f"[提示] {note}")
    for error in errors:
        print(f"[错误] {error}", file=sys.stderr)

    if errors:
        print(f"检查未通过，共 {len(errors)} 项错误", file=sys.stderr)
        return 1
    print("检查通过")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
