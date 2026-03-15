#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  perf_sv_constructor.sh [options] -- <command> [args...]

Purpose:
  Run a command multiple times, extract NajaPerf scope time for
  </SNLSVConstructor::construct>, and report aggregate stats.

Options:
  --label <name>               Label for output folder (default: perf)
  --runs <n>                   Number of measured runs (default: 7)
  --warmup <n>                 Number of warmup runs (default: 1)
  --run-dir <path>             Directory where command is executed (default: cwd)
  --out-dir <path>             Directory for logs/results (default: /tmp/naja_perf_sv)
  --summary-out <path>         Write summary env file here
  --baseline-summary <path>    Compare against baseline summary env file
  --threshold-pct <pct>        Fail if abs(median delta %) > threshold
  --keep-stdio                 Keep per-run stdout/stderr logs
  -h, --help                   Show this help

Notes:
  - Script sets NAJA_PERF per run automatically.
  - If command does not use NAJA_PERF (e.g. naja_edit), script falls back to
    <run-dir>/naja_stats.log if present.

Examples:
  ci/perf_sv_constructor.sh --label base --runs 9 --warmup 2 --run-dir /path/to/oss_eda \
    --summary-out /tmp/base.env -- \
    /opt/homebrew/bin/python3 naja.py --flist /path/to/Flist.cva6

  ci/perf_sv_constructor.sh --label cand --runs 9 --warmup 2 --run-dir /path/to/oss_eda \
    --baseline-summary /tmp/base.env --threshold-pct 5 -- \
    /opt/homebrew/bin/python3 naja.py --flist /path/to/Flist.cva6
EOF
}

require_integer() {
  local value="$1"
  local name="$2"
  if ! [[ "$value" =~ ^[0-9]+$ ]]; then
    echo "Invalid ${name}: ${value}" >&2
    exit 2
  fi
}

calc_mean() {
  local file="$1"
  awk '
    { sum += $1; n += 1 }
    END {
      if (n == 0) { print "nan"; exit }
      printf "%.6f", sum / n
    }' "$file"
}

calc_min() {
  local file="$1"
  awk '
    {
      if (NR == 1 || $1 < min) min = $1
    }
    END {
      if (NR == 0) { print "nan"; exit }
      printf "%.6f", min
    }' "$file"
}

calc_max() {
  local file="$1"
  awk '
    {
      if (NR == 1 || $1 > max) max = $1
    }
    END {
      if (NR == 0) { print "nan"; exit }
      printf "%.6f", max
    }' "$file"
}

calc_median() {
  local file="$1"
  sort -n "$file" | awk '
    { a[NR] = $1 }
    END {
      if (NR == 0) { print "nan"; exit }
      if (NR % 2 == 1) {
        printf "%.6f", a[(NR + 1) / 2]
      } else {
        printf "%.6f", (a[NR / 2] + a[NR / 2 + 1]) / 2.0
      }
    }'
}

extract_scope_s() {
  local perf_log="$1"
  awk '
    /<\/SNLSVConstructor::construct>/ {
      for (i = 1; i <= NF; ++i) {
        if ($i == "scope:") {
          v = $(i + 1)
          gsub(/s/, "", v)
          print v
          exit
        }
      }
    }' "$perf_log"
}

extract_drss_mb() {
  local perf_log="$1"
  awk '
    /<\/SNLSVConstructor::construct>/ {
      for (i = 1; i <= NF; ++i) {
        if ($i == "drss:") {
          v = $(i + 1)
          gsub(/Mb/, "", v)
          gsub(/\+/, "", v)
          print v
          exit
        }
      }
    }' "$perf_log"
}

label="perf"
runs=7
warmup=1
run_dir="$(pwd)"
out_dir="/tmp/naja_perf_sv"
summary_out=""
baseline_summary=""
threshold_pct=""
keep_stdio=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --label)
      label="${2:-}"
      shift 2
      ;;
    --runs)
      runs="${2:-}"
      shift 2
      ;;
    --warmup)
      warmup="${2:-}"
      shift 2
      ;;
    --run-dir)
      run_dir="${2:-}"
      shift 2
      ;;
    --out-dir)
      out_dir="${2:-}"
      shift 2
      ;;
    --summary-out)
      summary_out="${2:-}"
      shift 2
      ;;
    --baseline-summary)
      baseline_summary="${2:-}"
      shift 2
      ;;
    --threshold-pct)
      threshold_pct="${2:-}"
      shift 2
      ;;
    --keep-stdio)
      keep_stdio=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ $# -eq 0 ]]; then
  echo "Missing command. Use -- <command> [args...]" >&2
  usage
  exit 2
fi

require_integer "$runs" "runs"
require_integer "$warmup" "warmup"

if (( runs < 1 )); then
  echo "--runs must be >= 1" >&2
  exit 2
fi
if (( warmup < 0 )); then
  echo "--warmup must be >= 0" >&2
  exit 2
fi

if [[ ! -d "$run_dir" ]]; then
  echo "Run directory does not exist: $run_dir" >&2
  exit 2
fi

if [[ -n "$baseline_summary" ]] && [[ ! -f "$baseline_summary" ]]; then
  echo "Baseline summary not found: $baseline_summary" >&2
  exit 2
fi

if [[ -n "$threshold_pct" ]] && ! [[ "$threshold_pct" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
  echo "Invalid --threshold-pct: $threshold_pct" >&2
  exit 2
fi

cmd=( "$@" )
timestamp="$(date +%Y%m%d_%H%M%S)"
session_dir="${out_dir}/${timestamp}_${label}"
mkdir -p "$session_dir"

results_tsv="${session_dir}/results.tsv"
printf "idx\tkind\tscope_s\tdrss_mb\tperf_log\n" > "$results_tsv"

echo "Session: $session_dir"
echo "Run dir: $run_dir"
echo "Command: ${cmd[*]}"
echo "Warmup: $warmup, measured: $runs"

total=$((warmup + runs))
for ((i = 1; i <= total; ++i)); do
  kind="measure"
  idx=$((i - warmup))
  if (( i <= warmup )); then
    kind="warmup"
    idx=$i
  fi

  tag="$(printf "%s_%02d" "$kind" "$idx")"
  perf_log="${session_dir}/${tag}.naja_perf.log"
  stdout_log="${session_dir}/${tag}.stdout.log"
  stderr_log="${session_dir}/${tag}.stderr.log"
  fallback_stats="${run_dir}/naja_stats.log"

  rm -f "$perf_log" "$fallback_stats"

  set +e
  (
    cd "$run_dir"
    NAJA_PERF="$perf_log" "${cmd[@]}"
  ) >"$stdout_log" 2>"$stderr_log"
  rc=$?
  set -e

  if (( rc != 0 )); then
    echo "Run failed (${tag}), rc=${rc}" >&2
    echo "stderr: $stderr_log" >&2
    exit $rc
  fi

  if [[ ! -s "$perf_log" ]]; then
    if [[ -s "$fallback_stats" ]]; then
      cp "$fallback_stats" "$perf_log"
    else
      echo "No perf log found for ${tag} (neither $perf_log nor $fallback_stats)" >&2
      exit 3
    fi
  fi

  scope_s="$(extract_scope_s "$perf_log")"
  if [[ -z "$scope_s" ]]; then
    echo "Could not extract scope time from $perf_log" >&2
    exit 3
  fi
  drss_mb="$(extract_drss_mb "$perf_log")"
  if [[ -z "$drss_mb" ]]; then
    drss_mb="nan"
  fi

  printf "%d\t%s\t%s\t%s\t%s\n" "$idx" "$kind" "$scope_s" "$drss_mb" "$perf_log" >> "$results_tsv"
  echo "  ${tag}: scope=${scope_s}s drss=${drss_mb}Mb"

  if (( keep_stdio == 0 )); then
    rm -f "$stdout_log" "$stderr_log"
  fi
done

scope_values="${session_dir}/scope_values.txt"
drss_values="${session_dir}/drss_values.txt"
awk -F '\t' 'NR > 1 && $2 == "measure" { print $3 }' "$results_tsv" > "$scope_values"
awk -F '\t' 'NR > 1 && $2 == "measure" && $4 ~ /^[-+]?[0-9]*\.?[0-9]+$/ { print $4 }' \
  "$results_tsv" > "$drss_values"

scope_mean="$(calc_mean "$scope_values")"
scope_median="$(calc_median "$scope_values")"
scope_min="$(calc_min "$scope_values")"
scope_max="$(calc_max "$scope_values")"

drss_mean="nan"
drss_median="nan"
if [[ -s "$drss_values" ]]; then
  drss_mean="$(calc_mean "$drss_values")"
  drss_median="$(calc_median "$drss_values")"
fi

if [[ -z "$summary_out" ]]; then
  summary_out="${session_dir}/summary.env"
fi

cmd_quoted="$(printf '%q ' "${cmd[@]}")"
cmd_quoted="${cmd_quoted% }"
{
  echo "label=$label"
  echo "runs=$runs"
  echo "warmup=$warmup"
  echo "run_dir=$run_dir"
  echo "session_dir=$session_dir"
  echo "results_tsv=$results_tsv"
  echo "mean_scope_s=$scope_mean"
  echo "median_scope_s=$scope_median"
  echo "min_scope_s=$scope_min"
  echo "max_scope_s=$scope_max"
  echo "mean_drss_mb=$drss_mean"
  echo "median_drss_mb=$drss_median"
  printf "command_shell=%q\n" "$cmd_quoted"
} > "$summary_out"

echo ""
echo "Measured scope stats (s): mean=$scope_mean median=$scope_median min=$scope_min max=$scope_max"
echo "Measured drss stats (Mb): mean=$drss_mean median=$drss_median"
echo "Summary file: $summary_out"

if [[ -n "$baseline_summary" ]]; then
  # shellcheck disable=SC1090
  source "$baseline_summary"
  if [[ -z "${median_scope_s:-}" ]]; then
    echo "Baseline summary missing median_scope_s: $baseline_summary" >&2
    exit 4
  fi
  baseline_median="$median_scope_s"
  current_median="$scope_median"
  delta_pct="$(
    awk -v b="$baseline_median" -v c="$current_median" '
      BEGIN {
        if (b == 0) { print "nan"; exit }
        printf "%.3f", ((c - b) / b) * 100.0
      }'
  )"
  abs_delta_pct="$(
    awk -v d="$delta_pct" '
      BEGIN {
        v = d + 0.0
        if (v < 0) v = -v
        printf "%.3f", v
      }'
  )"
  echo "Baseline median scope: ${baseline_median}s"
  echo "Current  median scope: ${current_median}s"
  echo "Delta: ${delta_pct}%"

  if [[ -n "$threshold_pct" ]]; then
    over="$(
      awk -v a="$abs_delta_pct" -v t="$threshold_pct" 'BEGIN { print (a > t) ? 1 : 0 }'
    )"
    if [[ "$over" == "1" ]]; then
      echo "Threshold exceeded: |${delta_pct}%| > ${threshold_pct}%" >&2
      exit 5
    fi
  fi
fi
