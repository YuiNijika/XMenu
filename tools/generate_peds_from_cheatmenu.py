import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CHEAT_MENU = ROOT.parent / "Cheat-Menu" / "resource"
SOURCES = {
    "sa": CHEAT_MENU / "CheatMenuSA" / "data" / "peds.toml",
    "vc": CHEAT_MENU / "CheatMenuVC" / "data" / "peds.toml",
    "iii": CHEAT_MENU / "CheatMenuIII" / "data" / "peds.toml",
}
DESTINATIONS = {
    game: ROOT / "src" / "data" / game / "peds.json"
    for game in SOURCES
}
SECTION_RE = re.compile(r"^\[(.+)\]$")
ASSIGN_RE = re.compile(r'^"?([^"=]+?)"?\s*=\s*"([^"]+)"$')


def normalize_category(value: str) -> str:
    value = value.strip().lower()
    if value == "ganng":
        value = "gang"
    value = re.sub(r"[^a-z0-9]+", "_", value)
    return value.strip("_") or "common"


def parse_sa(path: Path) -> dict:
    entries = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = ASSIGN_RE.match(line.strip())
        if not match:
            continue
        key, name = match.groups()
        key = key.strip()
        if not key.isdigit():
            continue
        entries.append({"name": name.strip(), "id": int(key)})
    return {"peds": [{"category": "common", "entries": entries}]}


def parse_sectioned(path: Path) -> dict:
    categories = []
    current = None

    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        section = SECTION_RE.match(stripped)
        if section:
            current = {"category": normalize_category(section.group(1)), "entries": []}
            categories.append(current)
            continue

        match = ASSIGN_RE.match(stripped)
        if not match or current is None:
            continue

        name, raw_id = match.groups()
        raw_id = raw_id.strip()
        if not raw_id.isdigit():
            continue

        current["entries"].append({"name": name.strip().strip('"'), "id": int(raw_id)})

    categories = [category for category in categories if category["entries"]]
    return {"peds": categories}


def main() -> None:
    for game, source in SOURCES.items():
        if game == "sa":
            data = parse_sa(source)
        else:
            data = parse_sectioned(source)

        destination = DESTINATIONS[game]
        destination.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        total = sum(len(category["entries"]) for category in data["peds"])
        print(f"{game}: {total} peds -> {destination}")


if __name__ == "__main__":
    main()