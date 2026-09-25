import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent.parent
schema_path = root / "src" / "data" / "ui-schema.json"

inline_keys = {"games", "surfaces"}


def scalar(value) -> bool:
    return isinstance(value, (str, int, float, bool)) or value is None


def dump_scalar(value) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if value is None:
        return "null"
    if isinstance(value, str):
        return json.dumps(value, ensure_ascii=False)
    return json.dumps(value, ensure_ascii=False)


def is_flat(item) -> bool:
    if not isinstance(item, dict):
        return False
    for value in item.values():
        if scalar(value):
            continue
        if isinstance(value, list) and all(scalar(entry) for entry in value):
            continue
        return False
    return True


def emit_flat(item: dict) -> str:
    parts = []
    for key, value in item.items():
        if isinstance(value, list):
            inner = ", ".join(dump_scalar(entry) for entry in value)
            parts.append(f'"{key}": [{inner}]')
        else:
            parts.append(f'"{key}": {dump_scalar(value)}')
    return "{ " + ", ".join(parts) + " }"


def emit(value, level: int) -> list[str]:
    pad = "  " * level
    if isinstance(value, dict):
        if is_flat(value):
            return [f"{pad}{emit_flat(value)}"]
        lines = [f"{pad}{{"]
        items = list(value.items())
        for index, (key, entry) in enumerate(items):
            comma = "," if index < len(items) - 1 else ""
            if scalar(entry):
                lines.append(f'{pad}  "{key}": {dump_scalar(entry)}{comma}')
            elif isinstance(entry, list) and all(scalar(item) for item in entry):
                inner = ", ".join(dump_scalar(item) for item in entry)
                lines.append(f'{pad}  "{key}": [{inner}]{comma}')
            elif is_flat(entry):
                lines.append(f'{pad}  "{key}": {emit_flat(entry)}{comma}')
            elif isinstance(entry, list):
                lines.append(f'{pad}  "{key}": [')
                for item_index, item in enumerate(entry):
                    item_comma = "," if item_index < len(entry) - 1 else ""
                    sub = emit(item, level + 2)
                    sub[-1] += item_comma
                    lines.extend(sub)
                lines.append(f"{pad}  ]{comma}")
            else:
                sub = emit(entry, level + 1)
                sub[0] = f'{pad}  "{key}": ' + sub[0].strip()
                sub[-1] += comma
                lines.extend(sub)
        lines.append(f"{pad}}}")
        return lines
    if isinstance(value, list):
        lines = [f"{pad}["]
        for index, item in enumerate(value):
            comma = "," if index < len(value) - 1 else ""
            sub = emit(item, level + 1)
            sub[-1] += comma
            lines.extend(sub)
        lines.append(f"{pad}]")
        return lines
    return [f"{pad}{dump_scalar(value)}"]


def main() -> int:
    if not schema_path.is_file():
        print(f"缺少注册表文件 {schema_path}", file=sys.stderr)
        return 1

    data = json.loads(schema_path.read_text(encoding="utf-8-sig"))
    text = "\n".join(emit(data, 0)) + "\n"
    schema_path.write_text(text, encoding="utf-8", newline="")
    print(f"已格式化 {schema_path.name}，{text.count(chr(10))} 行")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
