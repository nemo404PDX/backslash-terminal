# Copyright (c) Backslash Terminal contributors.
# Licensed under the MIT license.
"""Headless checks for the Git-root/application-root boundary; no build or user state."""
from pathlib import Path
import os
import re
import xml.etree.ElementTree as ET

repo = Path(__file__).resolve().parents[3]
app = repo / "backslash"
obsolete_roots = ["src", "bin", "obj", "packages", ".nuget", "branding", "build", "dep", "doc",
                  "oss", "policies", "res", "samples", "scratch", "tools"]
for name in obsolete_roots:
    assert not os.path.lexists(repo / name), f"Obsolete physical root path exists: {name}"
required = ["README.md", "LICENSE", "NOTICE.md", ".gitignore", ".editorconfig", ".clang-format",
            ".config/dotnet-tools.json", ".github/CONTRIBUTING.md", ".github/SECURITY.md",
            "docs/README.md", "docs/backslash/UPSTREAM-SOURCE.md"]
for name in required:
    assert (repo / name).is_file(), f"Missing checkout-level file: {name}"
for name in ["OpenConsole.slnx", "common.openconsole.props", "Directory.Build.props", "Directory.Build.targets",
             "custom.props", "NuGet.Config", "vcpkg.json", ".vsconfig", "tools/backslash/Build.ps1"]:
    assert (app / name).is_file(), f"Missing application-level file: {name}"
    if name != "tools/backslash/Build.ps1":
        assert not (repo / name).is_file(), f"Stale root build file: {name}"
projects = ET.parse(app / "OpenConsole.slnx").getroot().iter("Project")
count = 0
for project in projects:
    path = project.get("Path")
    if path:
        target = (app / path.replace("\\", "/")).resolve()
        assert target.is_relative_to(app), f"Project escapes app tree: {path}"
        assert target.is_file(), f"Missing solution project: {path}"
        count += 1
readme = (repo / "README.md").read_text(encoding="utf-8-sig")
images = re.findall(r'<img\b[^>]*\bsrc=["\']([^"\']+)', readme)
for image in images:
    if not re.match(r"[A-Za-z]+:", image):
        assert (repo / image).is_file(), f"Missing README/gallery image: {image}"
resources = (app / "src/cascadia/CascadiaResources.build.items").read_text(encoding="utf-8-sig")
for name in ["backslash-badge.svg", "backslash-badge-outline.svg", "backslash-badge-currentcolor.svg"]:
    assert (repo / "docs/images" / name).is_file(), f"Missing supplied badge: {name}"
assert "$(OpenConsoleDir)..\\docs\\images\\" in resources, "Badge packaging must cross to root docs"
print(f"PASS: checkout/app boundary, {count} solution projects, {len(images)} gallery images, badge packaging paths and 15 absent obsolete physical roots")
