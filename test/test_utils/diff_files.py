#!/usr/bin/env python3 

# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys

def diff_files(file1, file2):
    line_number = 0
    with open(file1, 'r') as f1:
        with open(file2, 'r') as f2:
            for line1, line2 in zip(f1, f2):
                line_number += 1
                if (line1.startswith('#IGNORE#') or line2.startswith('#IGNORE#')):
                    continue
                if line1 != line2:
                    print(f'Files {file1} and {file2} differ at line {line_number}: {line1} vs {line2}',
                          file=sys.stderr)
                    return False
    return True

def parse_args():
    parser = argparse.ArgumentParser(description='Compare two files.')
    parser.add_argument('file1', type=str, help='First file to compare')
    parser.add_argument('file2', type=str, help='Second file to compare')
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    result = diff_files(args.file1, args.file2)
    print("Files are the same" if result else "Files are different")
    sys.exit(0 if result else 1)