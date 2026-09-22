#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

naja_source_dir="${GITHUB_WORKSPACE:?GITHUB_WORKSPACE must name the checked-out Naja tree}"
naja_build_dir="${naja_source_dir}/build"
naja_install_prefix="${naja_source_dir}/naja-install"
naja_python="$(command -v python3)"

if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo "Canonical Release builds require Linux x86-64." >&2
  exit 1
fi

naja_os_identity="$(. /etc/os-release && printf '%s:%s' "${ID}" "${VERSION_ID}")"
if [[ "${naja_os_identity}" != "ubuntu:24.04" ]]; then
  echo "Canonical Release builds require Ubuntu 24.04, found ${naja_os_identity}." >&2
  exit 1
fi

"${naja_python}" -c \
  'import sys; assert sys.version_info[:2] == (3, 12), sys.version'

cmake -S "${naja_source_dir}" -B "${naja_build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_INSTALL_PREFIX="${naja_install_prefix}" \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DPython3_EXECUTABLE="${naja_python}" \
  -DBUILD_NAJA_PYTHON=OFF \
  -DBUILD_BENCHMARKS=OFF \
  -DCODE_COVERAGE=OFF \
  -DENABLE_SANITIZERS=OFF \
  -DENABLE_THREAD_SANITIZER=OFF \
  -DENABLE_SV_CONSTRUCTOR_PERF_REPORT=OFF \
  -DLONG_TESTS=OFF
