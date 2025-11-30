# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

Write-Host "Installing vcpkg (latest)"

git clone https://github.com/microsoft/vcpkg.git $env:USERPROFILE/vcpkg
& "$env:USERPROFILE/vcpkg/bootstrap-vcpkg.bat"

# Install dependencies
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install capnproto:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install tbb:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install boost-intrusive:x64-windows

Write-Host "Installing winflexbison manually (official binary release)"

$wfbZip = "$env:TEMP\winflexbison-win64.zip"
Invoke-WebRequest `
  -Uri "https://github.com/lexxmark/winflexbison/releases/download/v2.5.28/winflexbison-win64.zip" `
  -OutFile $wfbZip

$tools = "$env:USERPROFILE/vcpkg/installed/x64-windows/tools/winflexbison"
New-Item -ItemType Directory -Force -Path $tools | Out-Null

Expand-Archive -Path $wfbZip -DestinationPath $tools -Force

$inner = Get-ChildItem $tools | Where-Object { $_.PSIsContainer } | Select-Object -First 1
Copy-Item "$($inner.FullName)\*" $tools -Recurse -Force

# Rename executables to GNU-standard names
Rename-Item "$tools\win_bison.exe" "$tools\bison.exe" -Force
Rename-Item "$tools\win_flex.exe" "$tools\flex.exe" -Force

# Create wrapper scripts to fix version output (CMake requires stdout)
Set-Content "$tools\bison.cmd" '@echo off
"%~dp0\bison.exe" %* 2>&1
'
Set-Content "$tools\flex.cmd" '@echo off
"%~dp0\flex.exe" %* 2>&1
'

Write-Host "Testing bison.exe..."
& "$tools\bison.exe" --version
if ($LASTEXITCODE -ne 0) {
    throw "bison.exe failed to run after rename"
}

Write-Host "Testing flex.exe..."
& "$tools\flex.exe" --version
if ($LASTEXITCODE -ne 0) {
    throw "flex.exe failed to run after rename"
}

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"