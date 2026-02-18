#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

# Local coverage helper (CI parity with .github/workflows/coverage.yml):
#   ./ci/coverage_local.sh [build-dir]
# Useful env vars:
#   BUILD_DIR=build-coverage BUILD_TYPE=Debug JOBS=8 ./ci/coverage_local.sh
# Fast rerun (reuse existing configure/build/tests, regenerate coverage only):
#   RUN_CONFIGURE=0 RUN_BUILD=0 RUN_TESTS=0 ./ci/coverage_local.sh
# Output:
#   <build-dir>/coverage.info

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-coverage}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-4}"
RUN_CONFIGURE="${RUN_CONFIGURE:-1}"
RUN_BUILD="${RUN_BUILD:-1}"
RUN_TESTS="${RUN_TESTS:-1}"

if [[ $# -gt 0 ]]; then
  BUILD_DIR="$1"
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: cmake is required." >&2
  exit 1
fi

if ! command -v lcov >/dev/null 2>&1; then
  echo "ERROR: lcov is required (on macOS: brew install lcov)." >&2
  exit 1
fi

if [[ "${RUN_CONFIGURE}" == "1" ]]; then
  echo "[1/4] Configure (${BUILD_DIR})"
  cmake \
    -S "${ROOT_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCODE_COVERAGE=ON \
    -DLONG_TESTS=ON
else
  echo "[1/4] Configure skipped (RUN_CONFIGURE=${RUN_CONFIGURE})"
fi

if [[ "${RUN_BUILD}" == "1" ]]; then
  echo "[2/4] Build"
  if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
    CC=clang CXX=clang++ cmake --build "${BUILD_DIR}" -j "${JOBS}" --config "${BUILD_TYPE}"
  else
    cmake --build "${BUILD_DIR}" -j "${JOBS}" --config "${BUILD_TYPE}"
  fi
else
  echo "[2/4] Build skipped (RUN_BUILD=${RUN_BUILD})"
fi

if [[ "${RUN_TESTS}" == "1" ]]; then
  echo "[3/4] Test"
  ctest --test-dir "${BUILD_DIR}" -VV -C "${BUILD_TYPE}"
else
  echo "[3/4] Test skipped (RUN_TESTS=${RUN_TESTS})"
fi

echo "[4/4] Coverage"
pushd "${BUILD_DIR}" >/dev/null

if [[ "$(uname -s)" == "Darwin" ]]; then
  lcov --ignore-errors inconsistent,unsupported,mismatch,count,format,corrupt \
    --directory . --capture --output-file coverage.info
  lcov --ignore-errors inconsistent,unsupported,mismatch,count,format,unused,corrupt \
    --remove coverage.info \
    --output-file coverage.info \
    '/usr/*' \
    '/Applications/*' \
    '/opt/homebrew/*' \
    '*/test/*' \
    '*/thirdparty/*' \
    '*/naja/thirdparty/*'
  lcov --ignore-errors inconsistent,unsupported,mismatch,count,format,unused,corrupt \
    --summary coverage.info
else
  lcov --directory . --capture --output-file coverage.info
  lcov --remove coverage.info --output-file coverage.info \
    '/usr/*' \
    '*/test/*' \
    '*/naja/thirdparty/*'
  lcov --summary coverage.info
fi

echo "Coverage report generated at: ${BUILD_DIR}/coverage.info"

popd >/dev/null
