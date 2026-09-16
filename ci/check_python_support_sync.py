#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Check that Python and wheel support declarations stay synchronized.

``pyproject.toml`` is authoritative for supported Python versions.  The wheel
workflow is authoritative for binary platforms.  Public documentation repeats
both for users, so this check makes any drift an actionable CI failure.
"""

import argparse
import re
import sys
import tomllib
from collections import defaultdict
from pathlib import Path


PYPROJECT = Path("pyproject.toml")
WHEEL_WORKFLOW = Path(".github/workflows/wheels.yml")
DOCUMENTATION = (
    Path("README.md"),
    Path("src/najaeda/README.rst"),
    Path("src/najaeda/najaeda/docs/source/introduction.rst"),
)

# Markers are intentionally user-facing terms rather than documentation
# formatting, so the same check works for Markdown and reStructuredText.
PLATFORM_MARKERS = {
    "manylinux_x86_64": "Linux x86_64",
    "manylinux_aarch64": "AArch64",
    "macosx_arm64": "macOS Apple Silicon (arm64)",
    "win_amd64": "Windows x86_64",
}


def version_key(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split("."))


def display_version(version: str) -> str:
    parts = version.split(".")
    while len(parts) > 1 and parts[-1] == "0":
        parts.pop()
    return ".".join(parts)


def version_from_matrix_code(code: str) -> tuple[str, bool] | None:
    match = re.fullmatch(r"(\d)(\d+)(t?)", code)
    if not match:
        return None
    major, minor, free_threaded = match.groups()
    return f"{major}.{int(minor)}", bool(free_threaded)


def parse_minimum_python(specifier: str) -> str:
    match = re.search(r"(?:^|,)\s*>=\s*(\d+\.\d+)(?:\.\d+)?(?:\s*|,|$)", specifier)
    if not match:
        raise ValueError(
            "[project].requires-python must contain a >= major.minor lower bound "
            f"that this check can validate; found {specifier!r}"
        )
    return match.group(1)


def load_project_support(root: Path) -> tuple[str, set[str]]:
    with (root / PYPROJECT).open("rb") as stream:
        project = tomllib.load(stream)["project"]

    minimum = parse_minimum_python(project["requires-python"])
    versions = set()
    for classifier in project.get("classifiers", []):
        match = re.fullmatch(r"Programming Language :: Python :: (\d+)\.(\d+)", classifier)
        if match:
            versions.add(f"{match.group(1)}.{int(match.group(2))}")
    return minimum, versions


def parse_matrix_entries(root: Path) -> list[dict[str, str]]:
    lines = (root / WHEEL_WORKFLOW).read_text(encoding="utf-8").splitlines()
    in_build_wheels = False
    in_include = False
    current: dict[str, str] | None = None
    entries: list[dict[str, str]] = []

    for line in lines:
        if line == "  build_wheels:":
            in_build_wheels = True
            continue
        if not in_build_wheels:
            continue
        if line.startswith("  ") and not line.startswith("    ") and line != "  build_wheels:":
            break
        if line == "        include:":
            in_include = True
            continue
        if not in_include:
            continue
        if line == "    steps:":
            break

        entry_match = re.fullmatch(r"\s+- os:\s*(\S+)\s*", line)
        if entry_match:
            if current is not None:
                entries.append(current)
            current = {"os": entry_match.group(1).strip("'\"")}
            continue

        field_match = re.fullmatch(r"\s+(python|platform_id|manylinux_image):\s*(\S+)\s*", line)
        if current is not None and field_match:
            current[field_match.group(1)] = field_match.group(2).strip("'\"")

    if current is not None:
        entries.append(current)
    return entries


def documented_support(path: Path) -> tuple[str | None, set[str], str]:
    text = path.read_text(encoding="utf-8")
    match = re.search(r"Requires Python (\d+\.\d+) or later\.", text)
    if not match:
        return None, set(), ""

    # The support list immediately follows the requirement. Limiting the scan
    # avoids unrelated platform mentions elsewhere in a README.
    support_block = text[match.start() : match.start() + 600]
    platforms = {
        platform
        for platform, marker in PLATFORM_MARKERS.items()
        if marker in support_block
    }
    return match.group(1), platforms, support_block


def check(root: Path) -> list[str]:
    failures: list[str] = []
    minimum, classifier_versions = load_project_support(root)
    entries = parse_matrix_entries(root)
    workflow_text = (root / WHEEL_WORKFLOW).read_text(encoding="utf-8")

    if not classifier_versions:
        failures.append("pyproject.toml has no minor-version Python classifiers")
        return failures

    oldest_classifier = min(classifier_versions, key=version_key)
    if minimum != oldest_classifier:
        failures.append(
            f"pyproject.toml requires-python starts at {minimum}, but the oldest "
            f"Python classifier is {oldest_classifier}"
        )

    coverage: dict[str, set[str]] = defaultdict(set)
    free_threaded_versions: set[str] = set()
    linux_images: set[str] = set()
    for entry in entries:
        missing = {"python", "platform_id"} - entry.keys()
        if missing:
            failures.append(
                "wheel matrix entry is missing "
                f"{', '.join(sorted(missing))}: {entry}"
            )
            continue

        parsed = version_from_matrix_code(entry["python"])
        if parsed is None:
            failures.append(f"unrecognized wheel matrix Python value: {entry['python']!r}")
            continue
        version, free_threaded = parsed
        if free_threaded:
            free_threaded_versions.add(version)
            continue

        platform = entry["platform_id"]
        coverage[version].add(platform)
        if platform.startswith("manylinux_"):
            if "manylinux_image" in entry:
                linux_images.add(entry["manylinux_image"])
            else:
                failures.append(
                    f"Python {version} {platform} has no manylinux_image in the matrix"
                )

    matrix_versions = set(coverage)
    if matrix_versions != classifier_versions:
        missing_from_matrix = classifier_versions - matrix_versions
        missing_from_classifiers = matrix_versions - classifier_versions
        if missing_from_matrix:
            failures.append(
                "Python classifiers without regular wheel jobs: "
                + ", ".join(sorted(missing_from_matrix, key=version_key))
            )
        if missing_from_classifiers:
            failures.append(
                "regular wheel jobs without Python classifiers: "
                + ", ".join(sorted(missing_from_classifiers, key=version_key))
            )

    unsupported_free_threaded = free_threaded_versions - classifier_versions
    if unsupported_free_threaded:
        failures.append(
            "free-threaded wheel jobs without corresponding Python classifiers: "
            + ", ".join(sorted(unsupported_free_threaded, key=version_key))
        )

    matrix_platforms = set().union(*coverage.values()) if coverage else set()
    unknown_platforms = matrix_platforms - PLATFORM_MARKERS.keys()
    if unknown_platforms:
        failures.append(
            "wheel platforms need documentation marker mappings in "
            "ci/check_python_support_sync.py: "
            + ", ".join(sorted(unknown_platforms))
        )

    for version in sorted(classifier_versions & matrix_versions, key=version_key):
        if coverage[version] != matrix_platforms:
            missing = matrix_platforms - coverage[version]
            extra = coverage[version] - matrix_platforms
            details = []
            if missing:
                details.append("missing " + ", ".join(sorted(missing)))
            if extra:
                details.append("unexpected " + ", ".join(sorted(extra)))
            failures.append(f"Python {version} wheel coverage differs: {'; '.join(details)}")

    if len(linux_images) > 1:
        failures.append(
            "documentation assumes one manylinux baseline, but the matrix uses: "
            + ", ".join(sorted(linux_images))
        )

    for platform in sorted(matrix_platforms):
        if platform.startswith("manylinux_"):
            architecture = platform.removeprefix("manylinux_").upper()
            variable = f"CIBW_MANYLINUX_{architecture}_IMAGE"
            if not re.search(rf"^\s+{re.escape(variable)}:", workflow_text, re.MULTILINE):
                failures.append(
                    f"{WHEEL_WORKFLOW}: does not forward matrix.manylinux_image via {variable}"
                )

    macos_targets = set(
        re.findall(r"\bMACOSX_DEPLOYMENT_TARGET=(\d+(?:\.\d+)*)", workflow_text)
    )
    if "macosx_arm64" in matrix_platforms and len(macos_targets) != 1:
        failures.append(
            f"{WHEEL_WORKFLOW}: expected one MACOSX_DEPLOYMENT_TARGET, found "
            + (", ".join(sorted(macos_targets)) if macos_targets else "none")
        )

    for relative_path in DOCUMENTATION:
        documented_minimum, documented_platforms, support_block = documented_support(
            root / relative_path
        )
        if documented_minimum is None:
            failures.append(
                f"{relative_path}: missing 'Requires Python X.Y or later.' support block"
            )
            continue
        if documented_minimum != minimum:
            failures.append(
                f"{relative_path}: documents Python {documented_minimum} or later, "
                f"but pyproject.toml requires {minimum} or later"
            )
        if documented_platforms != matrix_platforms:
            missing = matrix_platforms - documented_platforms
            extra = documented_platforms - matrix_platforms
            details = []
            if missing:
                details.append("missing " + ", ".join(sorted(missing)))
            if extra:
                details.append("unexpected " + ", ".join(sorted(extra)))
            failures.append(
                f"{relative_path}: platform documentation differs: {'; '.join(details)}"
            )

        if linux_images:
            manylinux_image = next(iter(linux_images))
            if manylinux_image not in support_block:
                failures.append(f"{relative_path}: does not document {manylinux_image}")
            baseline = re.fullmatch(r"manylinux_(\d+)_(\d+)", manylinux_image)
            if baseline:
                glibc_marker = f"glibc {baseline.group(1)}.{baseline.group(2)} or later"
                if glibc_marker not in support_block:
                    failures.append(f"{relative_path}: does not document {glibc_marker}")

        if len(macos_targets) == 1:
            macos_target = display_version(next(iter(macos_targets)))
            macos_marker = f"macOS {macos_target} or later"
            if macos_marker not in support_block:
                failures.append(f"{relative_path}: does not document {macos_marker}")

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="repository root (defaults to the parent of this script's directory)",
    )
    args = parser.parse_args()

    try:
        failures = check(args.root.resolve())
    except (KeyError, OSError, tomllib.TOMLDecodeError, ValueError) as error:
        print(f"Packaging support consistency check ERROR: {error}", file=sys.stderr)
        return 2

    if failures:
        print("Packaging support consistency check FAILED:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1

    minimum, versions = load_project_support(args.root.resolve())
    entries = parse_matrix_entries(args.root.resolve())
    platforms = sorted({entry["platform_id"] for entry in entries if "platform_id" in entry})
    print(
        "Python support is synchronized: "
        f">={minimum}; classifiers {', '.join(sorted(versions, key=version_key))}"
    )
    print("Wheel platforms are synchronized: " + ", ".join(platforms))
    print(f"Documentation is synchronized: {len(DOCUMENTATION)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
