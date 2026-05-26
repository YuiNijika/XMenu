import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CHEAT_MENU = ROOT.parent / "Cheat-Menu" / "resource"
SOURCES = {
    "sa": CHEAT_MENU / "CheatMenuSA" / "data",
    "vc": CHEAT_MENU / "CheatMenuVC" / "data",
    "iii": CHEAT_MENU / "CheatMenuIII" / "data",
}
LANGUAGES = ("zh", "en", "jp", "ru")
SECTION_RE = re.compile(r"^\[(.+)\]$")
ASSIGN_RE = re.compile(r"^\s*['\"]?(.+?)['\"]?\s*=\s*['\"]([^'\"]*)['\"]\s*$")

WEATHERS = {
    "sa": [
        "EXTRASUNNY LA", "SUNNY LA", "EXTRASUNNY SMOG LA", "SUNNY SMOG LA", "CLOUDY LA",
        "SUNNY SF", "EXTRASUNNY SF", "CLOUDY SF", "RAINY SF", "FOGGY SF",
        "SUNNY VEGAS", "EXTRASUNNY VEGAS", "CLOUDY VEGAS", "EXTRASUNNY COUNTRYSIDE",
        "SUNNY COUNTRYSIDE", "CLOUDY COUNTRYSIDE", "RAINY COUNTRYSIDE",
        "EXTRASUNNY DESERT", "SUNNY DESERT", "SANDSTORM DESERT", "UNDERWATER",
        "EXTRACOLOURS 1", "EXTRACOLOURS 2",
    ],
    "vc": ["SUNNY", "CLOUDY", "RAINY", "FOGGY", "EXTRA_SUNNY", "HURRICANE", "EXTRACOLORS"],
    "iii": ["SUNNY", "CLOUDY", "RAINY", "FOGGY"],
}

translations: dict[str, str] = {}
localized_translations: dict[str, dict[str, str]] = {language: {} for language in LANGUAGES}

WEATHER_I18N = {
    "weather": {
        "zh": "天气",
        "en": "Weather",
        "jp": "天気",
        "ru": "Погода",
    },
    "EXTRASUNNY LA": {"zh": "洛圣都特晴", "en": "Extrasunny LA", "jp": "LS 特別快晴", "ru": "Очень ясно, Лос-Сантос"},
    "SUNNY LA": {"zh": "洛圣都晴朗", "en": "Sunny LA", "jp": "LS 晴れ", "ru": "Ясно, Лос-Сантос"},
    "EXTRASUNNY SMOG LA": {"zh": "洛圣都烟雾特晴", "en": "Extrasunny Smog LA", "jp": "LS スモッグ特別快晴", "ru": "Очень ясно со смогом, Лос-Сантос"},
    "SUNNY SMOG LA": {"zh": "洛圣都烟雾晴朗", "en": "Sunny Smog LA", "jp": "LS スモッグ晴れ", "ru": "Ясно со смогом, Лос-Сантос"},
    "CLOUDY LA": {"zh": "洛圣都多云", "en": "Cloudy LA", "jp": "LS 曇り", "ru": "Облачно, Лос-Сантос"},
    "SUNNY SF": {"zh": "圣菲耶罗晴朗", "en": "Sunny SF", "jp": "SF 晴れ", "ru": "Ясно, Сан-Фиерро"},
    "EXTRASUNNY SF": {"zh": "圣菲耶罗特晴", "en": "Extrasunny SF", "jp": "SF 特別快晴", "ru": "Очень ясно, Сан-Фиерро"},
    "CLOUDY SF": {"zh": "圣菲耶罗多云", "en": "Cloudy SF", "jp": "SF 曇り", "ru": "Облачно, Сан-Фиерро"},
    "RAINY SF": {"zh": "圣菲耶罗雨天", "en": "Rainy SF", "jp": "SF 雨", "ru": "Дождь, Сан-Фиерро"},
    "FOGGY SF": {"zh": "圣菲耶罗雾天", "en": "Foggy SF", "jp": "SF 霧", "ru": "Туман, Сан-Фиерро"},
    "SUNNY VEGAS": {"zh": "拉斯云祖华晴朗", "en": "Sunny Vegas", "jp": "LV 晴れ", "ru": "Ясно, Лас-Вентурас"},
    "EXTRASUNNY VEGAS": {"zh": "拉斯云祖华特晴", "en": "Extrasunny Vegas", "jp": "LV 特別快晴", "ru": "Очень ясно, Лас-Вентурас"},
    "CLOUDY VEGAS": {"zh": "拉斯云祖华多云", "en": "Cloudy Vegas", "jp": "LV 曇り", "ru": "Облачно, Лас-Вентурас"},
    "EXTRASUNNY COUNTRYSIDE": {"zh": "乡村特晴", "en": "Extrasunny Countryside", "jp": "田舎 特別快晴", "ru": "Очень ясно, сельская местность"},
    "SUNNY COUNTRYSIDE": {"zh": "乡村晴朗", "en": "Sunny Countryside", "jp": "田舎 晴れ", "ru": "Ясно, сельская местность"},
    "CLOUDY COUNTRYSIDE": {"zh": "乡村多云", "en": "Cloudy Countryside", "jp": "田舎 曇り", "ru": "Облачно, сельская местность"},
    "RAINY COUNTRYSIDE": {"zh": "乡村雨天", "en": "Rainy Countryside", "jp": "田舎 雨", "ru": "Дождь, сельская местность"},
    "EXTRASUNNY DESERT": {"zh": "沙漠特晴", "en": "Extrasunny Desert", "jp": "砂漠 特別快晴", "ru": "Очень ясно, пустыня"},
    "SUNNY DESERT": {"zh": "沙漠晴朗", "en": "Sunny Desert", "jp": "砂漠 晴れ", "ru": "Ясно, пустыня"},
    "SANDSTORM DESERT": {"zh": "沙漠沙尘暴", "en": "Sandstorm Desert", "jp": "砂漠 砂嵐", "ru": "Песчаная буря, пустыня"},
    "UNDERWATER": {"zh": "水下", "en": "Underwater", "jp": "水中", "ru": "Под водой"},
    "EXTRACOLOURS 1": {"zh": "额外色彩 1", "en": "Extra Colours 1", "jp": "追加カラー 1", "ru": "Доп. цвета 1"},
    "EXTRACOLOURS 2": {"zh": "额外色彩 2", "en": "Extra Colours 2", "jp": "追加カラー 2", "ru": "Доп. цвета 2"},
    "SUNNY": {"zh": "晴朗", "en": "Sunny", "jp": "晴れ", "ru": "Ясно"},
    "CLOUDY": {"zh": "多云", "en": "Cloudy", "jp": "曇り", "ru": "Облачно"},
    "RAINY": {"zh": "雨天", "en": "Rainy", "jp": "雨", "ru": "Дождь"},
    "FOGGY": {"zh": "雾天", "en": "Foggy", "jp": "霧", "ru": "Туман"},
    "EXTRA_SUNNY": {"zh": "特晴", "en": "Extra Sunny", "jp": "特別快晴", "ru": "Очень ясно"},
    "HURRICANE": {"zh": "飓风", "en": "Hurricane", "jp": "ハリケーン", "ru": "Ураган"},
    "EXTRACOLORS": {"zh": "额外色彩", "en": "Extra Colors", "jp": "追加カラー", "ru": "Доп. цвета"},
}


def stable_id(value: str) -> str:
    normalized = re.sub(r"[^a-zA-Z0-9]+", "_", value.strip().lower())
    return normalized.strip("_") or "unknown"
 

def add_text(key: str, text: str) -> str:
    translations[key] = text or key
    return key


def add_localized_text(key: str, fallback: str, localized: dict[str, str] | None = None) -> str:
    translations[key] = fallback or key
    if localized:
        for language in LANGUAGES:
            localized_translations[language][key] = localized.get(language, fallback or key)
    return key


def category_key(domain: str, game: str, category: str) -> str:
    return add_text(f"{domain}.category.{game}.{stable_id(category)}", category)


def item_key(domain: str, game: str, category: str, name: str) -> str:
    return add_text(f"{domain}.{game}.{stable_id(category)}.{stable_id(name)}", name)


def clean_section(raw: str) -> str:
    return raw.strip().strip('"').strip("'") or "Common"


def parse_sectioned(path: Path):
    if not path.exists():
        return []
    categories = []
    current = None
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        section = SECTION_RE.match(stripped)
        if section:
            current = {"category": clean_section(section.group(1)), "entries": []}
            categories.append(current)
            continue
        match = ASSIGN_RE.match(stripped)
        if match and current is not None:
            name, value = match.groups()
            current["entries"].append({"name": name.strip().strip('"').strip("'"), "value": value.strip()})
    return [category for category in categories if category["entries"]]


def write_json(game: str, filename: str, key: str, categories) -> None:
    destination = ROOT / "src" / "data" / game / filename
    destination.write_text(json.dumps({key: categories}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    total = sum(len(category["entries"]) for category in categories)
    print(f"{game}: {total} {key} -> {destination}")


def build_animations(game: str, base: Path):
    categories = []
    for category in parse_sectioned(base / "animations.toml"):
        raw_category = category["category"]
        entries = []
        for entry in category["entries"]:
            value = entry["value"]
            raw_name = entry["name"]
            item = {
                "name": item_key("scene.animation", game, raw_category, raw_name),
                "value": raw_name,
            }
            if "$" in value:
                ifp_id, anim_id = value.split("$", 1)
                item["group"] = raw_category
                item["ifpId"] = int(ifp_id) if ifp_id.isdigit() else -1
                item["animId"] = int(anim_id) if anim_id.isdigit() else -1
            else:
                item["group"] = value or raw_category
                item["ifpId"] = -1
                item["animId"] = -1
            entries.append(item)
        categories.append({"category": category_key("scene.animation", game, raw_category), "entries": entries})
    write_json(game, "animations.json", "animations", categories)


def build_simple(game: str, base: Path, toml_name: str, json_name: str, key: str, domain: str, value_key: str):
    categories = []
    for category in parse_sectioned(base / toml_name):
        raw_category = category["category"]
        entries = []
        for entry in category["entries"]:
            raw_name = entry["name"]
            item = {"name": item_key(domain, game, raw_category, raw_name), value_key: entry["value"]}
            if key == "cutscenes":
                item["value"] = raw_name
            entries.append(item)
        categories.append({"category": category_key(domain, game, raw_category), "entries": entries})
    write_json(game, json_name, key, categories)


def localized_category_key(domain: str, game: str, category: str) -> str:
    return add_localized_text(f"{domain}.category.{game}.{stable_id(category)}", category, WEATHER_I18N.get(category))


def localized_item_key(domain: str, game: str, category: str, name: str) -> str:
    return add_localized_text(f"{domain}.{game}.{stable_id(category)}.{stable_id(name)}", name, WEATHER_I18N.get(name))


def build_visuals(game: str):
    category = "weather"
    categories = [{
        "category": localized_category_key("visual", game, category),
        "entries": [
            {"name": localized_item_key("visual", game, category, name), "id": index}
            for index, name in enumerate(WEATHERS[game])
        ],
    }]
    write_json(game, "visuals.json", "visuals", categories)


def update_i18n_index(language_dir: Path, module_name: str) -> None:
    index_path = language_dir / "index.json"
    index = json.loads(index_path.read_text(encoding="utf-8-sig"))
    files = index.setdefault("files", [])
    if module_name not in files:
        insert_at = len(files)
        for preferred in ("visual.json", "scene.json"):
            if preferred in files:
                insert_at = max(insert_at, files.index(preferred) + 1)
        files.insert(insert_at, module_name)
    index_path.write_text(json.dumps(index, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def write_i18n() -> None:
    module_name = "scene_visual_data.json"
    for language in LANGUAGES:
        language_dir = ROOT / "src" / "data" / "i18n" / language
        language_dir.mkdir(parents=True, exist_ok=True)
        data = dict(translations)
        data.update(localized_translations[language])
        (language_dir / module_name).write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        update_i18n_index(language_dir, module_name)
        print(f"{language}: {len(data)} scene/visual i18n -> {language_dir / module_name}")


def main():
    translations.clear()
    for values in localized_translations.values():
        values.clear()
    for game, base in SOURCES.items():
        build_animations(game, base)
        build_simple(game, base, "particles.toml", "particles.json", "particles", "scene.particle", "effect")
        build_simple(game, base, "cutscenes.toml", "cutscenes.json", "cutscenes", "scene.cutscene", "interior")
        build_visuals(game)
    write_i18n()


if __name__ == "__main__":
    main()