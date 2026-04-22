#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/coverage_svconstructor.sh full [options]
  scripts/coverage_svconstructor.sh focus (--gtest-filter FILTER | --ctest-filter REGEX) [options]

Options:
  --build-dir DIR         Coverage build directory.
                          Default: <repo>/build-coverage-svconstructor
  --build-type TYPE       CMake build type. Default: Debug
  --jobs N                Parallel build jobs. Default: 8
  --file PATH             Source file to annotate with gcov.
                          Default: src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp
  --gcno-path PATH        Explicit .gcno file to use instead of auto-discovery.
  --ctest-filter REGEX    ctest -R filter.
  --gtest-filter FILTER   GoogleTest filter for snlSVConstructorTests.
  --no-configure          Reuse existing configure step.
  --no-build              Reuse existing build artifacts.
  --no-clean              Keep existing .gcda counters.
  --publish-gcov          Copy the generated .gcov next to the target source file.
  --help                  Show this message.

Examples:
  scripts/coverage_svconstructor.sh full
  scripts/coverage_svconstructor.sh focus --gtest-filter \
    'SNLSVConstructorTestSimple.parseUnsupportedModuleBodyCoverageScopeLabelReported'
  scripts/coverage_svconstructor.sh focus --ctest-filter 'SNLSVConstructorTestSimple.parseUnsupported.*'

Reports:
  <build-dir>/reports/full/<timestamp>-.../
  <build-dir>/reports/focus/<timestamp>-.../

Each report directory contains:
  metadata.txt
  test.log
  gcov.log
  summary.txt
  uncovered.txt
  partial_branches.txt
  <target>.gcov
EOF
}

die() {
  echo "ERROR: $*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

sanitize_label() {
  printf '%s' "$1" | tr -cs '[:alnum:]._-' '_'
}

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-coverage-svconstructor"
BUILD_TYPE="Debug"
JOBS="8"
TARGET_FILE="src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp"
GCNO_PATH=""
CTEST_FILTER=""
GTEST_FILTER=""
RUN_CONFIGURE=1
RUN_BUILD=1
CLEAN_COUNTERS=1
PUBLISH_GCOV=0
MODE=""

if [[ $# -eq 0 ]]; then
  usage
  exit 1
fi

MODE="$1"
shift

case "$MODE" in
  full|focus)
    ;;
  -h|--help|help)
    usage
    exit 0
    ;;
  *)
    die "Unknown mode: $MODE"
    ;;
esac

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --build-type)
      BUILD_TYPE="$2"
      shift 2
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --file)
      TARGET_FILE="$2"
      shift 2
      ;;
    --gcno-path)
      GCNO_PATH="$2"
      shift 2
      ;;
    --ctest-filter)
      CTEST_FILTER="$2"
      shift 2
      ;;
    --gtest-filter)
      GTEST_FILTER="$2"
      shift 2
      ;;
    --no-configure)
      RUN_CONFIGURE=0
      shift
      ;;
    --no-build)
      RUN_BUILD=0
      shift
      ;;
    --no-clean)
      CLEAN_COUNTERS=0
      shift
      ;;
    --publish-gcov)
      PUBLISH_GCOV=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown argument: $1"
      ;;
  esac
done

need_cmd cmake
need_cmd ctest
need_cmd gcov
need_cmd python3

TARGET_PATH="${ROOT_DIR}/${TARGET_FILE}"
[[ -f "${TARGET_PATH}" ]] || die "Target file not found: ${TARGET_PATH}"

TEST_BINARY="${BUILD_DIR}/test/nl/formats/systemverilog/frontend/snlSVConstructorTests"
REPORT_KIND="$MODE"

if [[ "$MODE" == "full" ]]; then
  if [[ -n "${GTEST_FILTER}" ]]; then
    die "full mode does not accept --gtest-filter; use --ctest-filter or focus mode"
  fi
  if [[ -z "${CTEST_FILTER}" ]]; then
    CTEST_FILTER="SNLSVConstructorTest"
  fi
  RUN_DESCRIPTOR="ctest-${CTEST_FILTER}"
else
  if [[ -z "${GTEST_FILTER}" && -z "${CTEST_FILTER}" ]]; then
    die "focus mode requires --gtest-filter or --ctest-filter"
  fi
  if [[ -n "${GTEST_FILTER}" && -n "${CTEST_FILTER}" ]]; then
    die "focus mode accepts only one of --gtest-filter or --ctest-filter"
  fi
  if [[ -n "${GTEST_FILTER}" ]]; then
    RUN_DESCRIPTOR="gtest-${GTEST_FILTER}"
  else
    RUN_DESCRIPTOR="ctest-${CTEST_FILTER}"
  fi
fi

if [[ "${RUN_CONFIGURE}" == "1" ]]; then
  echo "[1/5] Configure ${BUILD_DIR}"
  cmake \
    -S "${ROOT_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCODE_COVERAGE=ON \
    -DLONG_TESTS=ON
else
  echo "[1/5] Configure skipped"
fi

if [[ "${RUN_BUILD}" == "1" ]]; then
  echo "[2/5] Build snlSVConstructorTests"
  cmake --build "${BUILD_DIR}" --target snlSVConstructorTests -j "${JOBS}" --config "${BUILD_TYPE}"
else
  echo "[2/5] Build skipped"
fi

[[ -x "${TEST_BINARY}" ]] || die "Missing test binary: ${TEST_BINARY}"

if [[ "${CLEAN_COUNTERS}" == "1" ]]; then
  echo "[3/5] Reset coverage counters"
  find "${BUILD_DIR}" -name '*.gcda' -delete
else
  echo "[3/5] Counter reset skipped"
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
RUN_LABEL="$(sanitize_label "${RUN_DESCRIPTOR}")"
REPORT_DIR="${BUILD_DIR}/reports/${REPORT_KIND}/${STAMP}-${RUN_LABEL}"
LATEST_LINK="${BUILD_DIR}/reports/${REPORT_KIND}/latest"
mkdir -p "${REPORT_DIR}"

TEST_LOG="${REPORT_DIR}/test.log"
GCOV_LOG="${REPORT_DIR}/gcov.log"
SUMMARY_FILE="${REPORT_DIR}/summary.txt"
UNCOVERED_FILE="${REPORT_DIR}/uncovered.txt"
PARTIAL_BRANCHES_FILE="${REPORT_DIR}/partial_branches.txt"
METADATA_FILE="${REPORT_DIR}/metadata.txt"

echo "[4/5] Run tests"
if [[ "$MODE" == "full" || -n "${CTEST_FILTER}" ]]; then
  ctest --test-dir "${BUILD_DIR}" -R "${CTEST_FILTER}" --output-on-failure -C "${BUILD_TYPE}" \
    >"${TEST_LOG}" 2>&1
else
  "${TEST_BINARY}" --gtest_filter="${GTEST_FILTER}" --gtest_color=no \
    >"${TEST_LOG}" 2>&1
fi

if [[ -z "${GCNO_PATH}" ]]; then
  mapfile -t gcno_candidates < <(find "${BUILD_DIR}" -name "$(basename "${TARGET_FILE}").gcno" | sort)
  if [[ "${#gcno_candidates[@]}" -eq 0 ]]; then
    die "Could not find $(basename "${TARGET_FILE}").gcno under ${BUILD_DIR}"
  fi
  if [[ "${#gcno_candidates[@]}" -gt 1 ]]; then
    printf 'Found multiple .gcno candidates:\n' >&2
    printf '  %s\n' "${gcno_candidates[@]}" >&2
    die "Please pass --gcno-path explicitly"
  fi
  GCNO_PATH="${gcno_candidates[0]}"
fi

[[ -f "${GCNO_PATH}" ]] || die "Missing gcno file: ${GCNO_PATH}"

echo "[5/5] Generate gcov and readable reports"
(
  cd "${REPORT_DIR}"
  gcov -b -c "${GCNO_PATH}" >"${GCOV_LOG}" 2>&1
)

mapfile -t gcov_candidates < <(find "${REPORT_DIR}" -maxdepth 1 -name "*$(basename "${TARGET_FILE}").gcov" | sort)
if [[ "${#gcov_candidates[@]}" -eq 0 ]]; then
  die "gcov did not produce a report for $(basename "${TARGET_FILE}")"
fi
TARGET_GCOV="${gcov_candidates[0]}"

python3 "${ROOT_DIR}/scripts/coverage_uncovered.py" \
  --gcov "${TARGET_GCOV}" \
  --summary-out "${SUMMARY_FILE}" \
  --uncovered-out "${UNCOVERED_FILE}" \
  --partial-branches-out "${PARTIAL_BRANCHES_FILE}"

{
  echo "mode=${MODE}"
  echo "timestamp=${STAMP}"
  echo "target_file=${TARGET_FILE}"
  echo "build_dir=${BUILD_DIR}"
  echo "build_type=${BUILD_TYPE}"
  echo "run_descriptor=${RUN_DESCRIPTOR}"
  echo "gcno_path=${GCNO_PATH}"
  echo "gcov_file=${TARGET_GCOV}"
  echo "git_commit=$(git -C "${ROOT_DIR}" rev-parse HEAD)"
} >"${METADATA_FILE}"

mkdir -p "$(dirname "${LATEST_LINK}")"
ln -sfn "${REPORT_DIR}" "${LATEST_LINK}"

if [[ "${PUBLISH_GCOV}" == "1" ]]; then
  cp "${TARGET_GCOV}" "${TARGET_PATH}.gcov"
fi

echo
echo "Coverage run complete."
echo "Report directory: ${REPORT_DIR}"
echo "Latest symlink:   ${LATEST_LINK}"
echo "Summary:          ${SUMMARY_FILE}"
echo "Uncovered:        ${UNCOVERED_FILE}"
echo "Partial branches: ${PARTIAL_BRANCHES_FILE}"
if [[ "${PUBLISH_GCOV}" == "1" ]]; then
  echo "Published gcov:   ${TARGET_PATH}.gcov"
fi
