# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import gzip
import shutil
import tempfile
import unittest
import zipfile

from najaeda import netlist

liberty_benchmarks = os.environ.get('LIBERTY_BENCHMARKS_PATH')


class NajaNetlistLibertyGzTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def _assert_can_instantiate_and2(self):
        top = netlist.create_top("top")
        and2_ins = top.create_child_instance("and2", "and2_ins")
        self.assertIsNotNone(and2_ins)

    def test_load_liberty_gz(self):
        primitives = [os.path.join(liberty_benchmarks, "tests", "small.lib.gz")]
        netlist.load_liberty(primitives)
        self._assert_can_instantiate_and2()

    def test_load_liberty_renamed_files(self):
        source = os.path.join(liberty_benchmarks, "tests", "small.lib")
        self.assertTrue(os.path.exists(source))

        with tempfile.TemporaryDirectory() as tempdir:
            lib_suffix_path = os.path.join(tempdir, "small.lib_tt")
            gzip_path = os.path.join(tempdir, "small.memory")
            zip_path = os.path.join(tempdir, "small.bundle")

            shutil.copyfile(source, lib_suffix_path)
            with open(source, "rb") as input_file:
                with gzip.open(gzip_path, "wb") as gzip_file:
                    shutil.copyfileobj(input_file, gzip_file)
            with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                archive.write(source, "memories/small.lib_tt")

            for path in (lib_suffix_path, gzip_path, zip_path):
                with self.subTest(path=path):
                    netlist.load_liberty(path)
                    self._assert_can_instantiate_and2()
                    netlist.reset()
