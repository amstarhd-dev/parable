#!/usr/bin/env python3
"""Offline structural/path/provenance checks, not certification of evidence or rights."""
import argparse
import json
import re
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[3]
REGISTRY = Path(__file__).with_name("registry.json")
EVIDENCE = {
    "host_numerical": {"passed", "not-performed", "not-applicable"},
    "independent_circuit": {"reduced-section-compared", "full-circuit-compared", "not-performed", "not-applicable"},
    "synthetic_audition": {"generated", "not-performed", "not-applicable"},
    "real_guitar_audition": {"auditioned", "not-performed", "not-applicable"},
    "arm_cross_compile": {"compiled", "not-performed", "not-applicable"},
    "physical_daisy": {"executed", "not-performed", "not-applicable"},
    "patch_runtime": {"integrated", "not-integrated", "not-applicable"},
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def fields(value, names):
    require(isinstance(value, dict) and set(value) == set(names.split()),
            f"Expected fields: {names}")


def text(value):
    require(isinstance(value, str) and bool(value.strip()), "Expected nonempty text")


def strings(value, nonempty=False):
    require(isinstance(value, list) and (not nonempty or bool(value)), "Expected text list")
    for item in value:
        text(item)


def local_file(root, value):
    text(value)
    require(not value.startswith("/") and "\\" not in value and ":" not in value
            and all(part not in {"", ".", ".."} for part in value.split("/")),
            f"Unsafe repository path: {value}")
    path = (root / value).resolve()
    require(path.is_relative_to(root.resolve()) and path.is_file(),
            f"Missing or escaping repository file: {value}")
    return path


def paths(root, values, nonempty=False):
    strings(values, nonempty)
    return [local_file(root, value) for value in values]


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def invalid_constant(value):
    raise ValueError(f"Non-JSON constant: {value}")


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object,
                      parse_constant=invalid_constant)


def validate(catalog, root=ROOT):
    root = Path(root).resolve()
    fields(catalog, "format version project_license assets")
    require(catalog["format"] == "parable.soundforge" and type(catalog["version"]) is int
            and catalog["version"] == 1, "Unsupported registry format/version")
    fields(catalog["project_license"], "status notes")
    require(catalog["project_license"]["status"] == "unspecified", "Project license has not been assigned")
    text(catalog["project_license"]["notes"])
    assets = catalog["assets"]
    require(isinstance(assets, list) and bool(assets), "Expected nonempty assets")
    ids = set()
    for asset in assets:
        fields(asset, "id type paths sources ownership_notes physics_notes behavior limitations dependencies evidence")
        text(asset["id"])
        require(re.fullmatch(r"[a-z][a-z0-9.-]*", asset["id"]) is not None, "Invalid stable ID")
        require(asset["id"] not in ids, "Duplicate asset ID")
        ids.add(asset["id"])
        require(asset["type"] in ("primitive", "model", "test", "reference"), "Unknown asset type")
        paths(root, asset["paths"], nonempty=True)
        for key in ("ownership_notes", "physics_notes", "behavior"):
            text(asset[key])
        strings(asset["limitations"], nonempty=True)
        fields(asset["dependencies"], "assets external")
        strings(asset["dependencies"]["assets"])
        strings(asset["dependencies"]["external"])
        require(isinstance(asset["sources"], list), "Expected source list")
        for source in asset["sources"]:
            fields(source, "provenance_path candidate relationship notice_paths scope")
            provenance = read_json(local_file(root, source["provenance_path"]))
            require(isinstance(provenance, dict) and isinstance(provenance.get("candidates"), list),
                    "Missing provenance candidates")
            index = source["candidate"]
            require(type(index) is int and 0 <= index < len(provenance["candidates"]), "Invalid provenance candidate")
            candidate = provenance["candidates"][index]
            require(isinstance(candidate, dict), "Missing source provenance")
            for key in ("url", "revision", "license"):
                text(candidate.get(key))
            require(urlparse(candidate["url"]).scheme == "https" and bool(urlparse(candidate["url"]).netloc),
                    "Invalid upstream URL")
            require(re.fullmatch(r"[0-9a-f]{40}", candidate["revision"]) is not None, "Unpinned source revision")
            require(source["relationship"] in ("used", "derived", "audit-only"), "Unknown source relationship")
            imported = source["relationship"] != "audit-only"
            paths(root, source["notice_paths"], nonempty=imported)
            # These evaluated candidates are explicitly uncleared for import in SF-001.
            if imported:
                require(candidate["license"] not in ("GPL-3.0", "no source license found in audited tree"),
                        "Evaluated source is not cleared for import")
            text(source["scope"])
        fields(asset["evidence"], " ".join(EVIDENCE))
        for category, allowed in EVIDENCE.items():
            evidence = asset["evidence"][category]
            fields(evidence, "status scope paths")
            require(isinstance(evidence["status"], str) and evidence["status"] in allowed,
                    f"Unknown evidence status for {category}")
            text(evidence["scope"])
            positive = evidence["status"] not in ("not-performed", "not-applicable", "not-integrated")
            paths(root, evidence["paths"], nonempty=positive)
    for asset in assets:
        for dependency in asset["dependencies"]["assets"]:
            require(dependency in ids and dependency != asset["id"], "Unknown or self asset dependency")
    return len(assets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("registry", nargs="?", type=Path, default=REGISTRY)
    args = parser.parse_args()
    try:
        count = validate(read_json(args.registry))
    except (ValueError, OSError) as error:
        parser.exit(1, f"Invalid Soundforge registry: {error}\n")
    print(f"Validated {count} Soundforge assets offline; evidence claims require human review.")


if __name__ == "__main__":
    main()
