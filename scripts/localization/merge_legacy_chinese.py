"""Merge reviewed legacy Chinese text into MuMain resources, without importing gameplay values."""
import argparse
import collections
import copy
import gzip
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

from opencc import OpenCC

ROOT = Path(__file__).resolve().parents[2]
LOC = ROOT / "src/Localization"
OUTPUT = ROOT / ".localization-output"
HAN = re.compile(r"[\u3400-\u9fff]")
PRINTF = re.compile(r"%(?:%|[-+#0]*\d*(?:\.\d+)?(?:hh|ll|[hlLjztI])*[cCdiuoxXfFeEgGaAspnS])")
BRACES = re.compile(r"(?<!\{)\{[^{}]+\}(?!\})")
CONVERTER = OpenCC("t2s")


def normalize(text):
    return re.sub(r"\s+", " ", text).strip().casefold()


def formats(text):
    return PRINTF.findall(text), BRACES.findall(text)


def valid(source, target):
    return bool(target.strip()) and formats(source) == formats(target)


def read_resx(path):
    return {node.attrib["name"]: node.findtext("value") or "" for node in ET.parse(path).findall("data")}


def write_resx(path, source_path, values):
    tree = ET.parse(source_path)
    for node in tree.findall("data"):
        node.find("value").text = values[node.attrib["name"]]
    ET.indent(tree, space="  ")
    tree.write(path, encoding="utf-8", xml_declaration=True)


def legacy_lookup(legacy):
    lookup = {}
    for row in legacy["texts"] + legacy["resources"]:
        source = row.get("en", row.get("source", ""))
        target = row["zh"]
        if HAN.search(target) and valid(source, target):
            lookup[normalize(source)] = target
    return lookup


def choose(source, existing, traditional, lookup, manual):
    candidates = [
        (manual.get(source), "manual"),
        (existing if existing != source else None, "existing"),
        (lookup.get(normalize(source)), "legacy"),
        (CONVERTER.convert(traditional) if traditional else None, "traditional"),
    ]
    for target, origin in candidates:
        if target is not None and valid(source, target):
            return target, origin
    return source, "untranslated"


def merge_group(group, lookup, manual):
    source = read_resx(LOC / (group + ".en.resx"))
    target_path = LOC / (group + ".zh-CN.resx")
    existing = read_resx(target_path) if target_path.exists() else {}
    traditional_path = LOC / (group + ".zh-TW.resx")
    traditional = read_resx(traditional_path) if traditional_path.exists() else {}
    result, audit = {}, []
    for key, english in source.items():
        target, origin = choose(english, existing.get(key), traditional.get(key), lookup, manual)
        if group == "Dialog":
            target = re.sub(r"(?<=[㐀-鿿，。！？、；：‘’“”「」]) +(?=[㐀-鿿，。！？、；：‘’“”「」])", "", target)
        result[key] = target
        audit.append({"group": group, "key": key, "en": english, "zh": target, "origin": origin})
    write_resx(target_path, LOC / (group + ".en.resx"), result)
    return audit


def translate_json(node, lookup, manual, audit, trail=""):
    if isinstance(node, list):
        for i, value in enumerate(node):
            translate_json(value, lookup, manual, audit, trail + "/" + str(i))
        return
    if not isinstance(node, dict):
        return
    if isinstance(node.get("en"), str):
        english = node["en"]
        target, origin = choose(english, node.get("zh-CN"), node.get("zh-TW"), lookup, manual)
        if origin != "untranslated":
            node["zh-CN"] = target
        audit.append({"key": trail, "en": english, "zh": target, "origin": origin})
    for key, value in list(node.items()):
        if key != "zh-CN":
            translate_json(value, lookup, manual, audit, trail + "/" + key)


def without_chinese(node):
    if isinstance(node, dict):
        return {k: without_chinese(v) for k, v in node.items() if k != "zh-CN"}
    if isinstance(node, list):
        return [without_chinese(v) for v in node]
    return node


def merge_data(lookup, manual):
    base = ROOT / "src/bin/Data"
    paths = list((base / "Items").rglob("*.json")) + [base / "Monsters/Monsters.json", base / "Local/ServerList.json"]
    all_audit = []
    for path in paths:
        if not path.exists():
            continue
        original = json.loads(path.read_text(encoding="utf-8"))
        updated = copy.deepcopy(original)
        audit = []
        translate_json(updated, lookup, manual, audit)
        assert without_chinese(original) == without_chinese(updated), path
        for row in audit:
            row["file"] = str(path.relative_to(base))
        all_audit.extend(audit)
        if original != updated:
            path.write_text(json.dumps(updated, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return all_audit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--manual", type=Path, required=True)
    args = parser.parse_args()
    legacy = json.loads(args.input.read_text(encoding="utf-8"))
    manual = json.loads(args.manual.read_text(encoding="utf-8"))
    lookup = legacy_lookup(legacy)
    audit = []
    for group in ("Game", "Dialog", "Metadata"):
        audit.extend(merge_group(group, lookup, manual))
    data_audit = merge_data(lookup, manual)
    OUTPUT.mkdir(exist_ok=True)
    (OUTPUT / "merge-audit.json").write_text(json.dumps(audit + data_audit, ensure_ascii=False, indent=2), encoding="utf-8")
    unresolved = [r for r in audit + data_audit if r["origin"] == "untranslated" and re.search(r"[A-Za-z]{3}", r["en"])]
    (OUTPUT / "untranslated.json").write_text(json.dumps(unresolved, ensure_ascii=False, indent=2), encoding="utf-8")
    print("RESOURCE_COUNTS", dict(collections.Counter((r["group"], r["origin"]) for r in audit)), flush=True)
    print("DATA_COUNTS", dict(collections.Counter(r["origin"] for r in data_audit)), flush=True)
    print("UNRESOLVED", len(unresolved), flush=True)


if __name__ == "__main__":
    main()
