# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Make pybind11-stubgen output for slang deterministic and valid Python."""

import keyword
from pathlib import Path
import re
import sys


_FUNCTION = re.compile(
    r"^(?P<head>\s*(?:async\s+)?def\s+\w+\s*\()"
    r"(?P<args>.*)(?P<tail>\)\s*->.*)$"
)
_CAPSULE_VALUE = re.compile(
    r'(_C_API:\s*typing\.Any)\s+# value = <capsule object "[^"]+" at 0x[0-9a-fA-F]+>'
)


def _has_top_level(text: str, token: str) -> bool:
    depths = {"(": 0, "[": 0, "{": 0}
    pairs = {")": "(", "]": "[", "}": "{"}
    quote = None
    escaped = False
    for char in text:
        if quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in "'\"":
            quote = char
        elif char in depths:
            depths[char] += 1
        elif char in pairs:
            depths[pairs[char]] -= 1
        elif char == token and not any(depths.values()):
            return True
    return False


def _split_parameters(text: str) -> list[str]:
    result = []
    start = 0
    depths = {"(": 0, "[": 0, "{": 0}
    pairs = {")": "(", "]": "[", "}": "{"}
    quote = None
    escaped = False
    for index, char in enumerate(text):
        if quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in "'\"":
            quote = char
        elif char in depths:
            depths[char] += 1
        elif char in pairs:
            depths[pairs[char]] -= 1
        elif char == "," and not any(depths.values()):
            result.append(text[start:index])
            start = index + 1
    result.append(text[start:])
    return result


def _normalize_parameter_order(line: str) -> str:
    match = _FUNCTION.match(line)
    if match is None:
        return line

    parameters = _split_parameters(match.group("args"))
    saw_default = False
    keyword_only = False
    for index, parameter in enumerate(parameters):
        stripped = parameter.strip()
        if stripped.startswith("*"):
            keyword_only = True
            continue
        if stripped == "/" or not stripped:
            continue
        has_default = _has_top_level(parameter, "=")
        if saw_default and not keyword_only and not has_default:
            parameters[index] = f"{parameter} = ..."
        saw_default = saw_default or has_default

    return f"{match.group('head')}{','.join(parameters)}{match.group('tail')}"


def _normalize_keywords(line: str) -> str:
    for name in keyword.kwlist:
        line = re.sub(
            rf"(?P<prefix>[,(]\s*){re.escape(name)}(?=\s*:)",
            rf"\g<prefix>{name}_",
            line,
        )
        line = re.sub(
            rf"^(?P<indent>\s*){re.escape(name)}(?=\s*:)",
            rf"\g<indent>{name}_",
            line,
        )
    return line


def normalize(path: Path) -> None:
    content = path.read_text(encoding="utf-8")
    lines = []
    for line in content.splitlines(keepends=True):
        ending = "\n" if line.endswith("\n") else ""
        body = line[:-1] if ending else line
        body = _normalize_keywords(body)
        body = _normalize_parameter_order(body)
        lines.append(body + ending)
    normalized = _CAPSULE_VALUE.sub(r"\1", "".join(lines))
    path.write_text(normalized, encoding="utf-8")


def main() -> None:
    for argument in sys.argv[1:]:
        normalize(Path(argument))


if __name__ == "__main__":
    main()
