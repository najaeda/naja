#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import dataclasses
import pathlib
import re
from typing import Iterable


GCOV_LINE_RE = re.compile(r"^\s*(?P<count>[-#=0-9A-Za-z*]+):\s*(?P<lineno>\d+):(.*)$")
BRANCH_RE = re.compile(r"^\s*branch\s+(?P<index>\d+)\s+(?:(?:taken\s+(?P<taken>\d+))|(?P<never>never executed))(?:\s+.*)?$")


@dataclasses.dataclass
class GcovLine:
  lineno: int
  count_token: str
  source: str
  branches: list[int | None] = dataclasses.field(default_factory=list)
  excluded: bool = False

  @property
  def executable(self) -> bool:
    return self.count_token != "-"

  @property
  def covered(self) -> bool:
    if not self.executable:
      return False
    token = self.count_token
    if token in {"#####", "====="}:
      return False
    match = re.match(r"(\d+)", token)
    return bool(match and int(match.group(1)) > 0)

  @property
  def partially_covered(self) -> bool:
    known = [value for value in self.branches if value is not None]
    return bool(known) and any(value == 0 for value in known) and any(value > 0 for value in known)


def parse_gcov(path: pathlib.Path) -> list[GcovLine]:
  lines: list[GcovLine] = []
  current: GcovLine | None = None
  for raw_line in path.read_text().splitlines():
    line_match = GCOV_LINE_RE.match(raw_line)
    if line_match:
      current = GcovLine(
        lineno=int(line_match.group("lineno")),
        count_token=line_match.group("count"),
        source=line_match.group(3),
      )
      lines.append(current)
      continue

    branch_match = BRANCH_RE.match(raw_line)
    if branch_match and current is not None:
      if branch_match.group("never") is not None:
        current.branches.append(0)
      else:
        current.branches.append(int(branch_match.group("taken")))
  apply_lcov_exclusions(lines)
  return lines


def apply_lcov_exclusions(lines: list[GcovLine]) -> None:
  exclusion_depth = 0
  for line in lines:
    source = line.source
    start_count = source.count("LCOV_EXCL_START")
    stop_count = source.count("LCOV_EXCL_STOP")
    excl_line = "LCOV_EXCL_LINE" in source

    if exclusion_depth > 0 or start_count > 0 or stop_count > 0 or excl_line:
      line.excluded = True

    exclusion_depth += start_count
    exclusion_depth = max(0, exclusion_depth - stop_count)


def group_ranges(line_numbers: Iterable[int]) -> list[tuple[int, int]]:
  sorted_lines = sorted(set(line_numbers))
  if not sorted_lines:
    return []
  ranges: list[tuple[int, int]] = []
  start = end = sorted_lines[0]
  for lineno in sorted_lines[1:]:
    if lineno == end + 1:
      end = lineno
      continue
    ranges.append((start, end))
    start = end = lineno
  ranges.append((start, end))
  return ranges


def format_ranges(ranges: Iterable[tuple[int, int]]) -> list[str]:
  rendered = []
  for start, end in ranges:
    rendered.append(f"{start}" if start == end else f"{start}-{end}")
  return rendered


def write_summary(path: pathlib.Path, gcov_path: pathlib.Path, gcov_lines: list[GcovLine]) -> None:
  excluded = [line for line in gcov_lines if line.excluded]
  executable = [line for line in gcov_lines if line.executable and not line.excluded]
  covered = [line for line in executable if line.covered]
  uncovered = [line for line in executable if not line.covered]
  partial = [line for line in executable if line.partially_covered]
  coverage_pct = (100.0 * len(covered) / len(executable)) if executable else 100.0

  summary_lines = [
    f"gcov_file: {gcov_path}",
    f"excluded_lcov_lines: {len(excluded)}",
    f"executable_lines: {len(executable)}",
    f"covered_lines: {len(covered)}",
    f"uncovered_lines: {len(uncovered)}",
    f"line_coverage_pct: {coverage_pct:.2f}",
    f"partial_branch_lines: {len(partial)}",
  ]

  uncovered_ranges = format_ranges(group_ranges(line.lineno for line in uncovered))
  if uncovered_ranges:
    summary_lines.append("uncovered_ranges:")
    summary_lines.extend(f"  - {value}" for value in uncovered_ranges)

  summary_lines.append("")
  path.write_text("\n".join(summary_lines))


def write_uncovered(path: pathlib.Path, gcov_lines: list[GcovLine]) -> None:
  uncovered = [line for line in gcov_lines if line.executable and not line.excluded and not line.covered]
  ranges = format_ranges(group_ranges(line.lineno for line in uncovered))
  output = [
    "# Uncovered executable lines",
    "",
    f"line_count: {len(uncovered)}",
    "ranges:",
  ]
  output.extend(f"  - {value}" for value in ranges)
  output.append("")
  output.append("lines:")
  for line in uncovered:
    output.append(f"{line.lineno}: {line.source.rstrip()}")
  output.append("")
  path.write_text("\n".join(output))


def write_partial_branches(path: pathlib.Path, gcov_lines: list[GcovLine]) -> None:
  partial = [line for line in gcov_lines if line.executable and not line.excluded and line.partially_covered]
  output = [
    "# Partially covered branches",
    "",
    f"line_count: {len(partial)}",
    "",
  ]
  for line in partial:
    branch_counts = ", ".join(str(value) for value in line.branches if value is not None)
    output.append(f"{line.lineno}: [{branch_counts}] {line.source.rstrip()}")
  output.append("")
  path.write_text("\n".join(output))


def main() -> int:
  parser = argparse.ArgumentParser(description="Extract readable reports from a gcov file.")
  parser.add_argument("--gcov", required=True, type=pathlib.Path, help="Path to a .gcov file")
  parser.add_argument("--summary-out", required=True, type=pathlib.Path)
  parser.add_argument("--uncovered-out", required=True, type=pathlib.Path)
  parser.add_argument("--partial-branches-out", required=True, type=pathlib.Path)
  args = parser.parse_args()

  gcov_lines = parse_gcov(args.gcov)
  write_summary(args.summary_out, args.gcov, gcov_lines)
  write_uncovered(args.uncovered_out, gcov_lines)
  write_partial_branches(args.partial_branches_out, gcov_lines)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
