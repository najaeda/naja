# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0


$ErrorActionPreference = 'Stop'
Set-PSDebug -Trace 1
Write-Host "=== ENTER test_windows.ps1 ==="
Write-Host "Script root: $PSScriptRoot"
Write-Host "PWD: $(Get-Location)"

Write-Host "PowerShell test command started"

$env:PYTHONPATH = ""

python "$PSScriptRoot\check_wheel_import.py"

$env:VERILOG_BENCHMARKS_PATH = "$PSScriptRoot\..\test\nl\formats\verilog\benchmarks"
$env:SYSTEMVERILOG_BENCHMARKS_PATH = "$PSScriptRoot\..\test\nl\formats\systemverilog\benchmarks"
$env:LIBERTY_BENCHMARKS_PATH = "$PSScriptRoot\..\test\nl\formats\liberty\benchmarks"
$env:NAJAEDA_TEST_PATH = "$PSScriptRoot\..\test\najaeda"
$env:NAJAEDA_SOURCE_TEST_PATH = "$PSScriptRoot\..\test\najaeda"
$env:NAJA_DB0_AUDIT_BINARY = (Get-Content -Raw "$PSScriptRoot\..\build\wheel\db0_audit_binary_Release.txt").Trim()
if (-not (Test-Path -PathType Leaf $env:NAJA_DB0_AUDIT_BINARY)) {
    throw "Native DB0 audit executable not found: $env:NAJA_DB0_AUDIT_BINARY"
}

python -m pytest -vv "$PSScriptRoot\..\test\najaeda"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Set-Location "$PSScriptRoot\..\src\najaeda\examples"
python run_regress.py

Set-Location "$PSScriptRoot\..\src\najaeda\native_examples"
python run_regress.py
