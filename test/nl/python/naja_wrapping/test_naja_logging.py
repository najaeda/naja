# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import logging
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
    naja.log("error", "generic error")
    naja.clearLogSinks()
    with open(log_path, "r", encoding="utf-8") as log_handle:
      contents = log_handle.read()
    self.assertIn("warning", contents)
    self.assertIn("test", contents)
    self.assertIn("generic error", contents)

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

  def test_log_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.log("info")

  def test_log_invalid_level(self):
    with self.assertRaises(RuntimeError):
      naja.log("notalevel", "message")

  def test_install_logging_handler_is_idempotent(self):
    root_logger = logging.getLogger()
    original_handlers = list(root_logger.handlers)
    original_level = root_logger.level
    original_basic_config = logging.basicConfig
    try:
      naja.installLoggingHandler()
      naja.installLoggingHandler()
      native_handlers = [
        handler for handler in root_logger.handlers
        if getattr(handler, "_naja_native_handler", False)
      ]
      self.assertEqual(1, len(native_handlers))
      self.assertTrue(
        getattr(logging.basicConfig, "_naja_preserves_native_handler", False)
      )
    finally:
      for handler in root_logger.handlers:
        if handler not in original_handlers:
          handler.close()
      root_logger.handlers[:] = original_handlers
      root_logger.setLevel(original_level)
      logging.basicConfig = original_basic_config

  def test_log_warn_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.logWarn()

  def test_log_critical_invalid_args(self):
    with self.assertRaises(RuntimeError):
      naja.logCritical()
