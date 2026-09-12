#!/bin/bash
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
#
# Bazel sh_test equivalent of najaeda-examples.yml: sets up the same
# scratch najaeda-package-plus-naja.so layout as
# test/najaeda/run_najaeda_tests.sh (own copy, not shared with that
# target -- unlike the CMake CI job, which reuses ctest's fixture
# directory, a Bazel test shouldn't depend on another target's side
# effects), then runs run_regress.py from src/najaeda/examples exactly
# as the workflow does. run_regress.py shells out to a bare "python3" per
# example script, so PATH (not just PYTHONPATH) must resolve to our
# located interpreter.
set -euo pipefail

najaeda_src_dir="$1"
naja_so="$2"
naja_runtime="$3"
python3_bin="$4"
examples_dir="$5"

runfiles_root="$PWD"
abspath() { echo "${runfiles_root}/$1"; }

scratch="$(mktemp -d)"
trap 'rm -rf "${scratch}"' EXIT

cp -R "$(abspath "${najaeda_src_dir}")" "${scratch}/najaeda"
cp "$(abspath "${naja_so}")" "${scratch}/najaeda/naja.so"

python3_abs="$(abspath "${python3_bin}")"
runtime_dir="$(dirname "$(abspath "${naja_runtime}")")"

export DYLD_LIBRARY_PATH="${runtime_dir}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"
export LD_LIBRARY_PATH="${runtime_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PYTHONPATH="${scratch}"
export PATH="$(dirname "${python3_abs}"):${PATH:-}"

cd "$(abspath "${examples_dir}")"
exec "${python3_abs}" run_regress.py
