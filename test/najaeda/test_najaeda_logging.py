# SPDX-FileCopyrightText: 2026 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import io
import logging
import os
import re
import unittest

from najaeda import naja


class NajaedaLoggingTest(unittest.TestCase):
    def setUp(self):
        test_dir = os.environ["NAJAEDA_TEST_PATH"]
        self.log_path = os.path.join(test_dir, "najaeda_native_logging.log")
        if os.path.exists(self.log_path):
            os.remove(self.log_path)
        naja.clearLogSinks()
        naja.addLogFile(self.log_path)
        naja.setLogLevel("debug")

    def tearDown(self):
        naja.clearLogSinks()
        naja.setLogLevel("info")

    def test_package_records_use_native_logger_without_root_duplication(self):
        root_output = io.StringIO()
        root_handler = logging.StreamHandler(root_output)
        root_logger = logging.getLogger()
        root_logger.addHandler(root_handler)
        try:
            logger = logging.getLogger("najaeda.test.forwarding")
            logger.info("python info through native logger")
            logger.warning("python warning through native logger")
            logger.error("python error through native logger")
        finally:
            root_logger.removeHandler(root_handler)

        # Closing the file sink flushes it before inspection.
        naja.clearLogSinks()
        with open(self.log_path, "r", encoding="utf-8") as log_file:
            contents = log_file.read()

        self.assertEqual("", root_output.getvalue())
        self.assertRegex(
            contents,
            re.compile(
                r"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3} "
                r"\[naja\] \[info\] python info through native logger"
            ),
        )
        self.assertIn("[warning] python warning through native logger", contents)
        self.assertIn("[error] python error through native logger", contents)


if __name__ == "__main__":
    unittest.main()
