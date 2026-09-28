# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

_SOURCE = Path(__file__).resolve().parents[2] / "src/najaeda/najaeda/sdk.py"
_SPEC = importlib.util.spec_from_file_location("najaeda_sdk_test", _SOURCE)
sdk = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(sdk)


def pe_fixture():
    data = bytearray(0x800)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x8664, 3, 0, 0, 0, 240, 0)
    struct.pack_into("<H", data, 0x98, 0x20B)
    struct.pack_into("<II", data, 0x108, 0x1000, 0x100)
    for index, (rva, raw, flags) in enumerate(((0x1000, 0x200, 0x40000040),
                                               (0x2000, 0x400, 0x60000020),
                                               (0x3000, 0x600, 0xC0000040))):
        offset = 0x188 + 40 * index
        struct.pack_into("<IIII", data, offset + 8, 0x200, rva, 0x200, raw)
        struct.pack_into("<I", data, offset + 36, flags)
    struct.pack_into("<IIHHIIIIIII", data, 0x200, 0, 0, 0, 0, 0, 1, 2, 2,
                     0x1040, 0x1050, 0x1060)
    struct.pack_into("<II", data, 0x240, 0x2000, 0x3000)
    struct.pack_into("<II", data, 0x250, 0x10A0, 0x10B0)
    struct.pack_into("<HH", data, 0x260, 0, 1)
    data[0x2A0:0x2A9] = b"function\0"
    data[0x2B0:0x2B6] = b"state\0"
    return data


class SharedRuntimeSDKTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="naja sdk spaces ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "najaeda"
        (self.root / "sdk/include/naja/core").mkdir(parents=True)
        (self.root / "sdk/include/naja/core/NLUniverse.h").touch()
        self.patch = mock.patch.object(sdk, "_ROOT", self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)
        (self.root / "sdk/build.json").write_text(json.dumps({
            "version": "0.7.24.dev0", "git_commit": "a" * 40,
            "build_id": "b" * 64, "compiler_id": "Clang",
            "compiler_version": "20.1.0", "pointer_size": 8,
            "boost_version": "109000", "tbb_version": "2022.3.0",
            "python_soabi": "cpython-313-test",
        }))

    def make_libraries(self, suffix=".so", repair_hash=""):
        for name in sdk._RUNTIMES:
            (self.root / f"lib{name}{repair_hash}{suffix}").touch()

    def test_original_and_repaired_library_names(self):
        for suffix in (".so", ".so.1", ".dylib", ".dll"):
            with self.subTest(suffix=suffix):
                self.make_libraries(suffix, "-a12b34")
                self.assertEqual(set(sdk._RUNTIMES), set(sdk.get_runtime_libraries()))
                for path in self.root.glob("lib*"):
                    path.unlink()

    def test_ambiguous_or_missing_provider_is_rejected(self):
        with self.assertRaises(RuntimeError):
            sdk.get_runtime_libraries()
        self.make_libraries()
        (self.root / "libnaja_nl-second.so").touch()
        with self.assertRaisesRegex(RuntimeError, "Expected one naja_nl"):
            sdk.get_runtime_libraries()

    def test_repair_sibling_directory_and_cmake_paths_with_spaces(self):
        self.make_libraries()
        sibling = self.root.parent / "najaeda.libs"
        sibling.mkdir()
        (self.root / "libnaja_nl.so").rename(sibling / "libnaja_nl-ab12.so")
        text = sdk._cmake_config()
        self.assertIn("NajaEDA_BUILD_ID", text)
        self.assertIn("naja sdk spaces ", text)
        self.assertIn("najaeda.libs/libnaja_nl-ab12.so", text)

    def test_cmake_injection_is_rejected(self):
        for value in ("x;y", "x\ny", "x]==]y"):
            with self.subTest(value=value), self.assertRaises(RuntimeError):
                sdk._cmake_value(value)

    def test_macos_consumer_fixup_relocates_provider_only_and_is_idempotent(self):
        self.make_libraries(".dylib")
        allocator = self.root / ".dylibs/libtbbmalloc.2.18.dylib"
        allocator.parent.mkdir()
        allocator.touch()
        core = self.root / "libnaja_nl.dylib"
        consumer = self.root.parent / "consumer binary"
        consumer.touch()
        references = ["/DLC/najaeda/.dylibs/libtbbmalloc.2.18.dylib",
                      "@rpath/libnaja_nl.dylib", "/usr/lib/libSystem.B.dylib"]

        def linkage():
            return str(consumer) + ":\n" + "".join(
                f"\t{name} (compatibility version 1.0.0, current version 1.0.0)\n"
                for name in references)

        with mock.patch.object(sdk.sys, "platform", "darwin"), \
             mock.patch.object(sdk.subprocess, "check_output", side_effect=lambda *a, **k: linkage()), \
             mock.patch.object(sdk.subprocess, "run") as run:
            sdk.fixup_consumer(consumer)
            self.assertEqual([
                mock.call(["install_name_tool", "-change", references[0], str(allocator.resolve()),
                           str(consumer.resolve())], check=True),
                mock.call(["install_name_tool", "-change", references[1], str(core.resolve()),
                           str(consumer.resolve())], check=True),
                mock.call(["codesign", "--force", "--sign", "-", str(consumer.resolve())], check=True),
            ], run.call_args_list)
            run.reset_mock()
            references[:2] = [str(allocator.resolve()), str(core.resolve())]
            sdk.fixup_consumer(consumer)
            run.assert_not_called()
            with self.assertRaisesRegex(RuntimeError, "must not modify.*provider"):
                sdk.fixup_consumer(core)

    def test_bundled_tbb_names_exclude_proxy_and_preserve_repair_names(self):
        dependencies = self.root / ".dylibs"
        dependencies.mkdir()
        for tbb, allocator in (("libtbb.12.18.dylib", "libtbbmalloc.2.18.dylib"),
                               ("libtbb-ab12.so.12", "libtbbmalloc-cd34.so.2"),
                               ("tbb12-ab12.dll", "tbbmalloc-cd34.dll")):
            with self.subTest(tbb=tbb):
                (dependencies / tbb).touch()
                (dependencies / allocator).touch()
                (dependencies / "tbbmalloc_proxy.dll").touch()
                result = sdk.get_dependency_libraries()
                self.assertEqual({"tbb", "tbbmalloc"}, set(result))
                self.assertEqual((dependencies / tbb).resolve(), Path(result["tbb"]))
                for path in dependencies.iterdir():
                    path.unlink()

    def test_pe_exports_preserve_data_symbols(self):
        library = self.root / "naja_nl-hash.dll"
        for zero_initialized in (False, True):
            with self.subTest(zero_initialized=zero_initialized):
                contents = pe_fixture()
                if zero_initialized:
                    struct.pack_into("<I", contents, 0x188 + 2 * 40 + 16, 0)
                library.write_bytes(contents)
                self.assertEqual([("function", False), ("state", True)], sdk._pe_exports(library))
        library.write_bytes(b"MZ")
        with self.assertRaises(RuntimeError):
            sdk._pe_exports(library)

    @unittest.skipUnless(shutil.which("cmake"), "CMake is required")
    def test_cmake_uses_provider_tbb_for_all_configurations(self):
        # Configure the real package config without compiling or relying on a
        # host TBB installation. The dependency finder fixtures supply headers
        # and targets; the SDK must replace their original library locations.
        fixture = self.root.parent / "consumer source"
        fixture.mkdir()
        cmake_dir = self.root / "sdk/cmake"
        cmake_dir.mkdir()
        shutil.copyfile(_SOURCE, self.root / "sdk.py")
        shutil.copyfile(_SOURCE.parents[3] / "cmake/NajaEDAConfig.cmake.in",
                        cmake_dir / "NajaEDAConfig.cmake")
        info = sdk.get_build_info()
        info["python_soabi"] = ""
        (self.root / "sdk/build.json").write_text(json.dumps(info))
        boost = fixture / "boost"
        (boost / "boost/intrusive").mkdir(parents=True)
        (boost / "boost/intrusive/set.hpp").touch()
        (boost / "boost/version.hpp").write_text("#define BOOST_VERSION 109000\n")
        suffix = ".dll" if sys.platform == "win32" else ".so"
        (self.root / "sdk/lib").mkdir()
        for name in (*sdk._RUNTIMES, "tbb", "tbbmalloc"):
            (self.root / f"{name}{suffix}").touch()
            (self.root / "sdk/lib" / f"{name}.lib").touch()
        (fixture / "FindPython3.cmake").write_text(
            f"set(Python3_EXECUTABLE {sdk._cmake_value(sys.executable)})\n"
            "set(Python3_FOUND TRUE)\nadd_library(Python3::Module INTERFACE IMPORTED)\n")
        (fixture / "FindTBB.cmake").write_text("""
set(TBB_FOUND TRUE)
set(TBB_VERSION 2022.3.0)
foreach(name IN ITEMS tbb tbbmalloc)
  add_library(TBB::${name} SHARED IMPORTED)
  foreach(suffix IN ITEMS "" _RELEASE _DEBUG _RELWITHDEBINFO _MINSIZEREL)
    set_property(TARGET TBB::${name} PROPERTY IMPORTED_LOCATION${suffix} stale)
    set_property(TARGET TBB::${name} PROPERTY IMPORTED_IMPLIB${suffix} stale)
  endforeach()
endforeach()
""")
        (fixture / "CMakeLists.txt").write_text("""
cmake_minimum_required(VERSION 3.24)
project(NajaEDASDKFixture NONE)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_SIZEOF_VOID_P 8)
set(CMAKE_CXX_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_VERSION 20.1.0)
find_package(NajaEDA CONFIG REQUIRED)
foreach(name IN ITEMS tbb tbbmalloc)
  foreach(suffix IN ITEMS "" _RELEASE _DEBUG _RELWITHDEBINFO _MINSIZEREL)
    get_target_property(actual TBB::${name} IMPORTED_LOCATION${suffix})
    if(NOT actual STREQUAL NajaEDA_${name}_LIBRARY)
      message(FATAL_ERROR "Wrong provider library for ${name}${suffix}: ${actual}")
    endif()
    if(WIN32)
      get_target_property(actual TBB::${name} IMPORTED_IMPLIB${suffix})
      if(NOT actual STREQUAL NajaEDA_${name}_IMPLIB)
        message(FATAL_ERROR "Wrong provider import library for ${name}${suffix}")
      endif()
    endif()
  endforeach()
endforeach()
get_target_property(system naja_nl SYSTEM)
if(system)
  message(FATAL_ERROR "Version-matched provider headers must precede CPATH")
endif()
""")
        result = subprocess.run([
            shutil.which("cmake"), "-S", str(fixture), "-B", str(fixture / "build"),
            f"-DNajaEDA_DIR={cmake_dir}", f"-DNajaEDA_BOOST_INCLUDE_DIR={boost}",
        ], capture_output=True, text=True)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_repaired_windows_import_library_is_generated_and_cached(self):
        library = self.root / "naja_nl-hash.dll"
        library.write_bytes(pe_fixture())
        commands = []

        def generate(command, **kwargs):
            commands.append(command)
            definition = Path(command[command.index("-d") + 1]).read_text()
            self.assertIn('LIBRARY "naja_nl-hash.dll"', definition)
            self.assertIn('"state" DATA', definition)
            Path(command[command.index("-l") + 1]).write_bytes(b"import library")

        with mock.patch.object(sdk.shutil, "which", return_value="llvm-dlltool"), \
             mock.patch.object(sdk.subprocess, "run", side_effect=generate):
            destination = sdk._import_library("naja_nl", library, self.root.parent / "consumer build")
            self.assertTrue(destination.is_file())
            self.assertEqual(destination, sdk._import_library("naja_nl", library, destination.parent))
        self.assertEqual(1, len(commands))


if __name__ == "__main__":
    unittest.main()
