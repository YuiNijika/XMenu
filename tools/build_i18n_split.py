import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent.parent
base = root / "src" / "data" / "i18n"
required_languages = {"zh", "en", "jp", "ru"}


def load_json(path: pathlib.Path):
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
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

    return errors


def main() -> int:
    errors: list[str] = []
    for code in sorted(required_languages):
        errors.extend(validate_language(code))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())