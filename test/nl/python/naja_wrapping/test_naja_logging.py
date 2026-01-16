# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import naja

class NajaLoggingTest(unittest.TestCase):
  def test0(self):
    log_file = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(log_file)
    log_path = os.path.join(log_file, 'NajaLoggingTest_test0.log')
    naja.addLogFile(log_path)
    self.assertTrue(os.path.isfile(log_path))
    naja.logWarn("warning")
    naja.logInfo("test")
    naja.clearLogSinks()
    with open(log_path, "r", encoding="utf-8") as log_handle:
      contents = log_handle.read()
    self.assertIn("warning", contents)
    self.assertIn("test", contents)
