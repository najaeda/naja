# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = 'Stop'

Write-Host "PowerShell test command started"

$env:PYTHONPATH = ""

python "$PSScriptRoot\check_wheel_import.py"

$env:VERILOG_BENCHMARKS_PATH = "$PSScriptRoot\..\test\nl\formats\verilog\benchmarks"
$env:LIBERTY_BENCHMARKS_PATH = "$PSScriptRoot\..\test\nl\formats\liberty\benchmarks"
$env:NAJAEDA_TEST_PATH = "$PSScriptRoot\..\test\najaeda"
$env:NAJAEDA_SOURCE_TEST_PATH = "$PSScriptRoot\..\test\najaeda"

python -m pytest "$PSScriptRoot\..\test\najaeda"

Set-Location "$PSScriptRoot\..\src\najaeda\examples"
python run_regress.py

Set-Location "$PSScriptRoot\..\src\najaeda\native_examples"
python run_regress.py