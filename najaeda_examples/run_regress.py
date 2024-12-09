# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

root_dir = os.getcwd()

# Iterate only through the immediate subdirectories
for dir_name in next(os.walk(root_dir))[1]:  # [1] gives the list of subdirectories
    subdir_path = os.path.join(root_dir, dir_name)
    print(f"Processing directory: {subdir_path}")
    
    # Iterate through the files in this subdirectory
    for file in os.listdir(subdir_path):
        if file.endswith(".py"):
            test_path = os.path.join(subdir_path, file)
            print(f"Running {test_path}...")
            # Run the test using subprocess
            subprocess.run(["python", file], cwd=subdir_path)
