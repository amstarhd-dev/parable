"""Negative catalog fixtures are explicit mutations of the committed real catalog."""
import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("registry", PROJECT / "tools/soundforge/validate_registry.py")
registry = importlib.util.module_from_spec(spec)
spec.loader.exec_module(registry)


class RegistryTests(unittest.TestCase):
    def setUp(self):
        self.catalog = registry.read_json(registry.REGISTRY)

    def rejected(self, edit, message):
        catalog = copy.deepcopy(self.catalog)
        edit(catalog)
        with self.assertRaisesRegex(ValueError, message):
            registry.validate(catalog)

    def test_committed_catalog(self):
        self.assertEqual(registry.validate(self.catalog), 11)

    def test_format_and_missing_fields(self):
        for version in (0, 2, True, "1"):
            self.rejected(lambda c: c.update(version=version), "format/version")
        self.rejected(lambda c: c.update(format="other"), "format/version")
        self.rejected(lambda c: c.pop("project_license"), "fields")
        self.rejected(lambda c: c["assets"][0].pop("ownership_notes"), "fields")
        self.rejected(lambda c: c["assets"][0].pop("sources"), "fields")
        self.rejected(lambda c: c["assets"][0]["evidence"].pop("physical_daisy"), "fields")

    def test_ids_types_statuses_dependencies(self):
        self.rejected(lambda c: c["assets"].append(copy.deepcopy(c["assets"][0])), "Duplicate asset ID")
        self.rejected(lambda c: c["assets"][0].update(type="plugin"), "asset type")
        self.rejected(lambda c: c["assets"][0]["evidence"]["host_numerical"].update(status="production-ready"), "evidence status")
        self.rejected(lambda c: c["assets"][0]["evidence"]["host_numerical"].update(paths=[]), "text list")
        self.rejected(lambda c: c["assets"][0]["dependencies"]["assets"].append("missing"), "asset dependency")

    def test_local_paths(self):
        for path in ("../outside", "/etc/passwd", "C:\\outside", "a/../README.md", "./README.md", "a//b"):
            self.rejected(lambda c: c["assets"][0].update(paths=[path]), "Unsafe repository path")
        self.rejected(lambda c: c["assets"][0].update(paths=["missing-file"]), "Missing or escaping")
        self.rejected(lambda c: c["assets"][0]["evidence"]["host_numerical"].update(paths=["../evidence"]), "Unsafe repository path")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "root"
            root.mkdir()
            outside = Path(directory) / "outside"
            outside.write_text("not in repository")
            (root / "escape").symlink_to(outside)
            with self.assertRaisesRegex(ValueError, "escaping"):
                registry.local_file(root, "escape")

    def test_provenance_and_notices(self):
        self.rejected(lambda c: c["assets"][0]["sources"][0].pop("relationship"), "fields")
        self.rejected(lambda c: c["assets"][0]["sources"][0].update(relationship="maybe-used"), "relationship")
        self.rejected(lambda c: c["assets"][0]["sources"][0].update(candidate=99), "candidate")
        self.rejected(lambda c: c["assets"][0]["sources"][0].update(notice_paths=[]), "text list")
        self.rejected(lambda c: c["assets"][0]["sources"][0].update(notice_paths=["../notice"]), "Unsafe repository path")
        self.rejected(lambda c: c["assets"][0]["sources"][0].update(provenance_path="missing-provenance"), "Missing or escaping")
        # A source becomes imported only after a deliberate provenance/license decision.
        for index in (2, 3):
            self.rejected(lambda c: c["assets"][0]["sources"][0].update(candidate=index), "not cleared")

    def test_import_eligibility(self):
        cases = (("MIT", True), ("BSD-2-Clause", True), ("GPL-2.0", False),
                 ("GPL-3.0", False), ("AGPL-3.0", False), ("proprietary", False),
                 ("unlicensed", False), ("unknown", False), ("custom-license", False),
                 ("Apache-2.0", False), ("MIT OR GPL-2.0", False))
        for license_name, eligible in cases:
            for relationship in ("used", "derived", "audit-only"):
                with self.subTest(license=license_name, relationship=relationship), tempfile.TemporaryDirectory() as directory:
                    root = Path(directory)
                    (root / "fixture.txt").write_text("Test fixture, not imported code.")
                    candidate = dict(url="https://example.com/source", revision="a"*40, license=license_name)
                    (root / "provenance.json").write_text(json.dumps({"candidates": [candidate]}))
                    catalog = copy.deepcopy(self.catalog)
                    asset = catalog["assets"][1]
                    catalog["assets"] = [asset]
                    asset["paths"] = ["fixture.txt"]
                    for evidence in asset["evidence"].values():
                        if evidence["paths"]:
                            evidence["paths"] = ["fixture.txt"]
                    asset["sources"] = [dict(provenance_path="provenance.json", candidate=0,
                                             relationship=relationship, scope="License-policy fixture",
                                             notice_paths=[] if relationship == "audit-only" else ["fixture.txt"])]
                    if eligible or relationship == "audit-only":
                        self.assertEqual(registry.validate(catalog, root), 1)
                    else:
                        with self.assertRaisesRegex(ValueError, "not cleared"):
                            registry.validate(catalog, root)

    def test_bad_json_and_missing_source_fields(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "catalog.json"
            for contents in ("{", '{"version":1,"version":2}', '{"value":NaN}'):
                path.write_text(contents)
                with self.assertRaises(ValueError):
                    registry.read_json(path)
            # Exercise malformed provenance using a real local file under a temporary repo.
            for missing in ("url", "revision", "license"):
                catalog = copy.deepcopy(self.catalog)
                candidate = {"url": "https://example.com/source", "revision": "a"*40, "license": "MIT"}
                candidate.pop(missing)
                path.write_text(json.dumps({"candidates": [candidate]}))
                catalog["assets"] = [catalog["assets"][0]]
                asset = catalog["assets"][0]
                asset["paths"] = ["catalog.json"]
                asset["sources"] = [dict(provenance_path="catalog.json", candidate=0,
                                         relationship="used", notice_paths=["catalog.json"], scope="test")]
                with self.assertRaisesRegex(ValueError, "nonempty text"):
                    registry.validate(catalog, root)


if __name__ == "__main__":
    unittest.main()
