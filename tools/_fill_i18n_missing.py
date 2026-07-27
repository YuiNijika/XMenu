# -*- coding: utf-8 -*-
"""Fill jp/ru missing keys from zh (structure) + en (English) with hand translations for known blocks."""
import json
from pathlib import Path

base = Path(r"e:/GTA/dev/XMenu/src/data/i18n")

# Explicit translations for missing blocks (prefer over raw English)
JP = {
    "settings.listMenuTheme": "リストモードテーマ色",
    "theme.list.classic": "白黒クラシック (Classic)",
    "theme.list.red": "R*レッド (Rockstar Red)",
    "theme.list.vice": "ヴァイスピンク (Vice Pink)",
    "theme.list.midnight": "ミッドナイトブルー (Midnight Blue)",
    "theme.list.emerald": "エメラルドグリーン (Emerald Green)",
    "theme.list.sa": "SA オレンジ (SA Orange)",
    "theme.list.light": "ピュアホワイト (Pure Light)",
    "world.freecam": "フリーカメラ (SA のみ)",
    "world.enableFreecam": "フリーカメラを有効",
    "world.freecamControls": "操作：W/A/S/D 移動。マウス視点。Ctrl+ホイール ズーム。ホイール 速度。Shift 加速。Enter でプレイヤーをカメラ位置へ。",
    "world.freecamFov": "視野 (FOV)",
    "world.freecamSpeedMul": "速度倍率",
    "world.freecamTeleported": "プレイヤーをカメラ位置へ転送しました",
    "world.topDownCam": "俯瞰カメラ (SA のみ)",
    "world.enableTopDownCam": "俯瞰カメラを有効",
    "world.topDownCamZoom": "ズーム高さ",
    "world.randomCheats": "ランダムチート (SA のみ)",
    "world.enableRandomCheats": "ランダムチートを有効",
    "world.showRandomCheatsProgress": "進捗バーを表示",
    "world.randomCheatsInterval": "間隔 (秒)",
    "world.randomCheatsList": "チート一覧",
    "vehicle.neon": "ネオン",
    "vehicle.neonR": "ネオン R",
    "vehicle.neonG": "ネオン G",
    "vehicle.neonB": "ネオン B",
    "vehicle.paint": "塗装とテクスチャ (SA のみ)",
    "vehicle.color1": "カラー 1",
    "vehicle.color2": "カラー 2",
    "vehicle.color3": "カラー 3",
    "vehicle.color4": "カラー 4",
    "vehicle.resetColors": "色をリセット",
    "vehicle.autoDrive": "ウェイポイントへ自動運転",
}

RU = {
    "settings.listMenuTheme": "Тема списочного меню",
    "theme.list.classic": "Ч/б классика (Classic)",
    "theme.list.red": "Rockstar Red",
    "theme.list.vice": "Vice Pink",
    "theme.list.midnight": "Midnight Blue",
    "theme.list.emerald": "Emerald Green",
    "theme.list.sa": "SA Orange",
    "theme.list.light": "Pure Light",
    "world.freecam": "Свободная камера (только SA)",
    "world.enableFreecam": "Включить свободную камеру",
    "world.freecamControls": "Управление: W/A/S/D — движение. Мышь — обзор. Ctrl+колесо — зум. Колесо — скорость. Shift — ускорение. Enter — телепорт игрока к камере.",
    "world.freecamFov": "FOV",
    "world.freecamSpeedMul": "Множитель скорости",
    "world.freecamTeleported": "Игрок телепортирован к камере",
    "world.topDownCam": "Вид сверху (только SA)",
    "world.enableTopDownCam": "Включить вид сверху",
    "world.topDownCamZoom": "Высота зума",
    "world.randomCheats": "Случайные читы (только SA)",
    "world.enableRandomCheats": "Включить случайные читы",
    "world.showRandomCheatsProgress": "Показывать прогресс",
    "world.randomCheatsInterval": "Интервал (сек)",
    "world.randomCheatsList": "Список читов",
    "vehicle.neon": "Неон",
    "vehicle.neonR": "Неон R",
    "vehicle.neonG": "Неон G",
    "vehicle.neonB": "Неон B",
    "vehicle.paint": "Покраска и текстуры (только SA)",
    "vehicle.color1": "Цвет 1",
    "vehicle.color2": "Цвет 2",
    "vehicle.color3": "Цвет 3",
    "vehicle.color4": "Цвет 4",
    "vehicle.resetColors": "Сбросить цвета",
    "vehicle.autoDrive": "Автопилот к метке",
}

TRANS = {"jp": JP, "ru": RU}


def load_keys(lang: str):
    keys = {}
    root = base / lang
    for p in root.rglob("*.json"):
        if p.name == "index.json":
            continue
        d = json.loads(p.read_text(encoding="utf-8"))
        if isinstance(d, dict):
            for k, v in d.items():
                keys[k] = (p, v)
    return keys


def file_map(lang: str):
    """key -> path where zh stores it (same relative name under lang)."""
    m = {}
    root = base / lang
    for p in root.rglob("*.json"):
        if p.name == "index.json":
            continue
        d = json.loads(p.read_text(encoding="utf-8"))
        if isinstance(d, dict):
            for k in d:
                m[k] = p
    return m


def main():
    zh = load_keys("zh")
    en = load_keys("en")
    zh_files = file_map("zh")

    for lang in ("jp", "ru"):
        cur = load_keys(lang)
        miss = sorted(set(zh) - set(cur))
        print(f"=== {lang} missing before: {len(miss)}")
        by_rel = {}
        for k in miss:
            zh_path, _ = zh[k]
            rel = zh_path.relative_to(base / "zh")
            tgt = base / lang / rel
            by_rel.setdefault(tgt, []).append(k)

        for tgt, keys in by_rel.items():
            if tgt.exists():
                data = json.loads(tgt.read_text(encoding="utf-8"))
            else:
                data = {}
                tgt.parent.mkdir(parents=True, exist_ok=True)

            for k in keys:
                if k in TRANS[lang]:
                    data[k] = TRANS[lang][k]
                elif k in en:
                    data[k] = en[k][1]  # English fallback better than Chinese for jp/ru
                else:
                    data[k] = zh[k][1]
            # Keep stable-ish order: existing keys first, then new sorted
            tgt.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            print(f"  wrote {len(keys)} keys -> {tgt.relative_to(base)}")

        cur2 = load_keys(lang)
        miss2 = sorted(set(zh) - set(cur2))
        print(f"=== {lang} missing after: {len(miss2)}")
        if miss2:
            for k in miss2[:30]:
                print(" ", k)


if __name__ == "__main__":
    main()