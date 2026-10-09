import json
import pathlib
import re
import sys
from collections import Counter

root = pathlib.Path(__file__).resolve().parent.parent
base = root / "src" / "data" / "i18n"
required_languages = {"zh", "en", "jp", "ru"}


def load_json(path: pathlib.Path):
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate translation key: {key}")
            result[key] = value
        return result

    try:
        return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique_object)
    except Exception as error:
        raise RuntimeError(f"Invalid JSON: {path} ({error})") from error


def validate_language(code: str) -> list[str]:
    errors: list[str] = []
    language_dir = base / code
    index_path = language_dir / "index.json"

    if not language_dir.is_dir():
        return [f"Missing language directory: {language_dir}"]
    if not index_path.is_file():
        return [f"Missing language index: {index_path}"]

    index = load_json(index_path)
    if index.get("code") != code:
        errors.append(f"Language code mismatch in {index_path}: expected {code}")
    if not index.get("name"):
        errors.append(f"Missing language name in {index_path}")

    files = index.get("files")
    if not isinstance(files, list) or not files:
        errors.append(f"Missing files list in {index_path}")
        return errors

    for filename in files:
        if not isinstance(filename, str) or not filename.endswith(".json"):
            errors.append(f"Invalid module entry in {index_path}: {filename}")
            continue

        module_path = language_dir / filename
        if not module_path.is_file():
            errors.append(f"Missing language module: {module_path}")
            continue

        module = load_json(module_path)
        if not isinstance(module, dict):
            errors.append(f"Language module must be an object: {module_path}")
            continue
        for key, value in module.items():
            if not isinstance(value, str) or not value.strip():
                errors.append(f"Empty or invalid translation: {module_path}: {key}")

    return errors


def validate_alignment() -> list[str]:
    errors: list[str] = []
    reference_index = load_json(base / "zh" / "index.json")
    placeholder = re.compile(r"%(?:[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|j|z|t|L)?[diuoxXfFeEgGaAcsp]|%)")
    for code in sorted(required_languages - {"zh"}):
        index = load_json(base / code / "index.json")
        if index.get("files") != reference_index["files"]:
            errors.append(f"Language module list differs from zh: {code}")
        if index.get("fallback") and index["fallback"] not in required_languages:
            errors.append(f"Unknown fallback language: {code}: {index['fallback']}")
        for filename in reference_index["files"]:
            reference = load_json(base / "zh" / filename)
            translated = load_json(base / code / filename)
            missing = reference.keys() - translated.keys()
            extra = translated.keys() - reference.keys()
            if missing:
                errors.append(f"Missing keys: {code}/{filename}: {', '.join(sorted(missing))}")
            if extra:
                errors.append(f"Extra keys: {code}/{filename}: {', '.join(sorted(extra))}")
            for key in reference.keys() & translated.keys():
                expected = Counter(placeholder.findall(reference[key]))
                actual = Counter(placeholder.findall(translated[key]))
                if expected != actual:
                    errors.append(f"Format placeholders differ: {code}/{filename}: {key}")
    return errors


def main() -> int:
    errors: list[str] = []
    for code in sorted(required_languages):
        errors.extend(validate_language(code))
    if not errors:
        errors.extend(validate_alignment())

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
