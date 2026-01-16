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

  def test_add_log_file_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.addLogFile()

  def test_add_log_file_invalid_type(self):
    with self.assertRaises(RuntimeError):
      naja.addLogFile(123)

  def test_add_log_file_invalid_level(self):
    log_file = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(log_file)
    log_path = os.path.join(log_file, 'NajaLoggingTest_invalid_level.log')
    with self.assertRaises(RuntimeError):
      naja.addLogFile(log_path, "notalevel")

  def test_add_log_file_unwritable_path(self):
    log_file = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(log_file)
    dir_path = os.path.join(log_file, 'NajaLoggingTest_unwritable')
    os.makedirs(dir_path, exist_ok=True)
    naja.addLogFile(dir_path)
    self.assertTrue(os.path.isdir(dir_path))

  def test_set_log_level_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.setLogLevel()

  def test_set_log_level_invalid_level(self):
    with self.assertRaises(RuntimeError):
      naja.setLogLevel("notalevel")

  def test_set_log_level_filters(self):
    log_file = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(log_file)
    log_path = os.path.join(log_file, 'NajaLoggingTest_set_level.log')
    naja.clearLogSinks()
    naja.addLogFile(log_path)
    naja.setLogLevel("warn")
    naja.logInfo("info suppressed")
    naja.logWarn("warn visible")
    naja.clearLogSinks()
    with open(log_path, "r", encoding="utf-8") as log_handle:
      contents = log_handle.read()
    self.assertNotIn("info suppressed", contents)
    self.assertIn("warn visible", contents)

  def test_log_info_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.logInfo()

  def test_log_warn_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.logWarn()

  def test_log_critical_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.logCritical()
