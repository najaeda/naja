# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import subprocess

root_dir = os.getcwd()
timeout_seconds = 3 * 60  # Set timeout duration in seconds

# Iterate only through the immediate subdirectories
for dir_name in next(os.walk(root_dir))[1]:  # [1] gives the list of subdirectories
    subdir_path = os.path.join(root_dir, dir_name)
    print(f"Processing directory: {subdir_path}")

    failure = False
    # Iterate through the files in this subdirectory
    for file in os.listdir(subdir_path):
        if file.endswith(".py"):
            test_path = os.path.join(subdir_path, file)
            print(f"Running {test_path}...")

            try:
                # Run the test using subprocess with timeout
                process_return = subprocess.run(
                    ["python3", file],
                    cwd=subdir_path,
                    timeout=timeout_seconds
                )

                if process_return.returncode != 0:
                    print(f"Test {test_path} failed with return code {process_return.returncode}!")
                    failure = True

            except subprocess.TimeoutExpired:
                print(f"Test {test_path} timed out after {timeout_seconds} seconds!")
                failure = True
            except Exception as e:
                print(f"An error occurred while running {test_path}: {e}")
                failure = True

    if failure:
        sys.exit(1)

sys.exit(0)
