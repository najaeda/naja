"""Locations and build identity for native consumers of this NajaEDA wheel.

Only one provider runtime may own the native objects. Consumers must validate
the capsule's build and runtime identities before dereferencing borrowed data.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

_ROOT = Path(__file__).resolve().parent
_RUNTIMES = ("naja_nl", "naja_dnl", "naja_bne", "naja_opt", "naja_metrics", "naja_python")


def get_cmake_dir() -> str:
    return str(_ROOT / "sdk" / "cmake")


def get_include_dir() -> str:
    return str(_ROOT / "sdk" / "include")


def get_build_info() -> dict[str, object]:
    return json.loads((_ROOT / "sdk" / "build.json").read_text(encoding="utf-8"))


def _provider_files() -> tuple[Path, ...]:
    roots = [_ROOT, _ROOT.parent / "najaeda.libs"]
    return tuple(path for root in roots for path in root.rglob("*") if path.is_file())


def get_runtime_libraries() -> dict[str, str]:
    """Resolve original or wheel-repair-renamed DSOs, rejecting ambiguity."""
    files = _provider_files()
    libraries = {}
    for name in _RUNTIMES:
        pattern = re.compile(
            rf"(?:lib)?{name}(?:-[a-zA-Z0-9_]+)?(?:\.so(?:\.[0-9.]+)?|\.dylib|\.dll)$",
            re.IGNORECASE,
        )
        matches = [path.resolve() for path in files if pattern.fullmatch(path.name)]
        matches = sorted(set(matches))
        if len(matches) != 1:
            raise RuntimeError(f"Expected one {name} provider library, found {matches}")
        libraries[name] = str(matches[0])
    return libraries


def get_dependency_libraries() -> dict[str, str]:
    """Prefer provider-bundled TBB allocators over an additional system copy."""
    libraries = {}
    for name in ("tbb", "tbbmalloc"):
        pattern = re.compile(
            rf"(?:lib)?{name}(?:[0-9]+)?(?:[-.][a-zA-Z0-9_]+)*"
            r"(?:\.so(?:\.[0-9.]+)?|\.dylib|\.dll)$", re.IGNORECASE)
        matches = sorted({path.resolve() for path in _provider_files() if pattern.fullmatch(path.name)})
        if len(matches) > 1:
            raise RuntimeError(f"Ambiguous provider dependency {name}: {matches}")
        if matches:
            libraries[name] = str(matches[0])
    return libraries


def fixup_consumer(consumer: Path) -> None:
    """Point a build-tree Mach-O consumer at this installed provider only.

    Repaired provider dylibs may carry synthetic absolute install IDs such as
    /DLC/... . Linking inherits those IDs; adding an rpath cannot resolve them.
    Wheel repair subsequently makes these consumer references relocatable.
    """
    if sys.platform != "darwin":
        return
    consumer = consumer.resolve(strict=True)
    files = {path.resolve() for path in _provider_files()}
    if consumer in files:
        raise RuntimeError("Consumer fixup must not modify the NajaEDA provider")
    libraries = {}
    for library in sorted(files):
        if library.suffix != ".dylib":
            continue
        if library.name in libraries:
            raise RuntimeError(f"Ambiguous provider library: {library.name}")
        libraries[library.name] = library
    linkage = subprocess.check_output(["otool", "-L", str(consumer)], text=True)
    changed = False
    for line in linkage.splitlines()[1:]:
        old = line.strip().split(" (compatibility version", 1)[0]
        provider = libraries.get(Path(old).name)
        if provider is not None and old != str(provider):
            subprocess.run(["install_name_tool", "-change", old, str(provider),
                            str(consumer)], check=True)
            changed = True
    if changed:
        subprocess.run(["codesign", "--force", "--sign", "-", str(consumer)], check=True)


def _pe_exports(library: Path) -> list[tuple[str, bool]]:
    """Read named AMD64 PE exports, including the data/function distinction."""
    data = library.read_bytes()

    def unpack(fmt: str, offset: int) -> tuple[int, ...]:
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise RuntimeError(f"Truncated PE export table: {library}")
        return struct.unpack_from(fmt, data, offset)

    if data[:2] != b"MZ":
        raise RuntimeError(f"Not a PE library: {library}")
    (pe,) = unpack("<I", 0x3C)
    if data[pe:pe + 4] != b"PE\0\0":
        raise RuntimeError(f"Invalid PE signature: {library}")
    machine, count, _, _, _, optional_size, _ = unpack("<HHIIIHH", pe + 4)
    optional = pe + 24
    if machine != 0x8664 or unpack("<H", optional)[0] != 0x20B:
        raise RuntimeError(f"NajaEDA SDK requires an AMD64 PE32+ library: {library}")
    export_rva, export_size = unpack("<II", optional + 112)
    sections = []
    for index in range(count):
        section = optional + optional_size + index * 40
        virtual_size, rva, raw_size, raw = unpack("<IIII", section + 8)
        (flags,) = unpack("<I", section + 36)
        sections.append((rva, max(virtual_size, raw_size), raw, raw_size, flags))

    def locate(rva: int, *, require_data: bool = True) -> tuple[int, int]:
        for start, size, raw, raw_size, flags in sections:
            if start <= rva < start + size and (not require_data or rva - start < raw_size):
                return raw + rva - start, flags
        raise RuntimeError(f"Unmapped PE export RVA in {library}")

    offset, _ = locate(export_rva)
    _, _, _, _, _, _, function_count, name_count, functions, names, ordinals = unpack("<IIHHIIIIIII", offset)
    function_table, _ = locate(functions)
    name_table, _ = locate(names)
    ordinal_table, _ = locate(ordinals)
    exports = []
    for index in range(name_count):
        (name_rva,) = unpack("<I", name_table + 4 * index)
        name_offset, _ = locate(name_rva)
        end = data.find(b"\0", name_offset)
        if end < 0:
            raise RuntimeError(f"Unterminated PE export name in {library}")
        name = data[name_offset:end].decode("ascii")
        if not name or any(character in name for character in ('"', "\n", "\r")):
            raise RuntimeError(f"Invalid PE export name in {library}")
        (ordinal,) = unpack("<H", ordinal_table + 2 * index)
        if ordinal >= function_count:
            raise RuntimeError(f"Invalid PE export ordinal in {library}")
        (address,) = unpack("<I", function_table + 4 * ordinal)
        # An exported zero-initialized variable can live in a section whose
        # virtual size exceeds its file-backed bytes. Only its flags are
        # needed here; the export/name tables above still require real bytes.
        _, flags = locate(address, require_data=False)
        forwarded = export_rva <= address < export_rva + export_size
        exports.append((name, not forwarded and not flags & 0x20000020))
    if not exports:
        raise RuntimeError(f"NajaEDA provider exports no named symbols: {library}")
    return exports


def _import_library(name: str, library: Path, output_dir: Path | None) -> Path:
    original = _ROOT / "sdk" / "lib" / f"{name}.lib"
    if library.name.lower() == f"{name}.dll" and original.is_file():
        return original
    if output_dir is None:
        raise RuntimeError("A writable --import-lib-dir is required for repaired provider DLLs")
    digest = hashlib.sha256(library.read_bytes()).hexdigest()[:20]
    output_dir.mkdir(parents=True, exist_ok=True)
    destination = output_dir / f"{name}-{digest}.lib"
    if destination.is_file():
        return destination
    dlltool = shutil.which("llvm-dlltool")
    if dlltool is None:
        candidate = Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "LLVM/bin/llvm-dlltool.exe"
        if candidate.is_file():
            dlltool = str(candidate)
    librarian = shutil.which("lib.exe") if dlltool is None else None
    if dlltool is None and librarian is None:
        raise RuntimeError("Repaired NajaEDA DLLs require llvm-dlltool or MSVC lib.exe to build import libraries")
    exports = _pe_exports(library)
    with tempfile.TemporaryDirectory(prefix=f"{name}-", dir=output_dir) as temporary:
        definition = Path(temporary) / "provider.def"
        temporary_library = Path(temporary) / "provider.lib"
        definition.write_text(
            f'LIBRARY "{library.name}"\nEXPORTS\n' + "".join(
                f'  "{symbol}"' + (" DATA" if is_data else "") + "\n"
                for symbol, is_data in exports), encoding="ascii")
        command = ([dlltool, "-m", "i386:x86-64", "-d", str(definition), "-l", str(temporary_library)]
                   if dlltool else [librarian, "/nologo", "/machine:x64", f"/def:{definition}", f"/out:{temporary_library}"])
        subprocess.run(command, check=True, capture_output=True, text=True)
        temporary_library.replace(destination)
    return destination


def _cmake_value(value: object) -> str:
    text = str(value).replace("\\", "/")
    if any(character in text for character in (";", "\n", "\r", "]==]")):
        raise RuntimeError(f"Unsupported character in NajaEDA SDK value: {text!r}")
    return f"[==[{text}]==]"


def _cmake_config(import_lib_dir: Path | None = None) -> str:
    info = get_build_info()
    libraries = get_runtime_libraries()
    dependencies = get_dependency_libraries()
    include_root = Path(get_include_dir())
    include_dirs = sorted({include_root, *(path.parent for path in (include_root / "naja").rglob("*.h"))})
    values = {
        "NajaEDA_VERSION": info["version"],
        "NajaEDA_GIT_COMMIT": info["git_commit"],
        "NajaEDA_BUILD_ID": info["build_id"],
        "NajaEDA_COMPILER_ID": info["compiler_id"],
        "NajaEDA_COMPILER_VERSION": info["compiler_version"],
        "NajaEDA_POINTER_SIZE": info["pointer_size"],
        "NajaEDA_BOOST_VERSION": info["boost_version"],
        "NajaEDA_TBB_VERSION": info["tbb_version"],
        "NajaEDA_PYTHON_SOABI": info["python_soabi"],
    }
    lines = [f"set({name} {_cmake_value(value)})" for name, value in values.items()]
    lines.append("set(NajaEDA_INCLUDE_DIRS " + " ".join(_cmake_value(path) for path in include_dirs) + ")")
    all_libraries = {**libraries, **dependencies}
    lines.append("set(NajaEDA_RUNTIME_LIBRARIES " + " ".join(_cmake_value(path) for path in all_libraries.values()) + ")")
    for name, library in all_libraries.items():
        lines.append(f"set(NajaEDA_{name}_LIBRARY {_cmake_value(library)})")
        if sys.platform == "win32":
            implib = _import_library(name, Path(library), import_lib_dir)
            lines.append(f"set(NajaEDA_{name}_IMPLIB {_cmake_value(implib)})")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--cmake", action="store_true", help="emit installed CMake runtime metadata")
    action.add_argument("--fixup-consumer", type=Path, help="repair build-tree Mach-O provider references")
    parser.add_argument("--import-lib-dir", type=Path, help="build-directory cache for repaired Windows import libraries")
    args = parser.parse_args()
    if args.fixup_consumer is not None:
        fixup_consumer(args.fixup_consumer)
    else:
        print(_cmake_config(args.import_lib_dir) if args.cmake else json.dumps(get_build_info(), indent=2), end="\n")
