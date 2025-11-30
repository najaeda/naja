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

# Install winflexbison (provides win_flex and win_bison)
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install winflexbison:x64-windows

# Copy MSYS2 runtime DLLs next to win_bison.exe and win_flex.exe
$msysRoot = Get-ChildItem "$env:USERPROFILE/vcpkg/downloads/tools/msys2" -Directory |
    Select-Object -First 1

if (-not $msysRoot) {
    Write-Host "ERROR: MSYS2 directory not found under vcpkg downloads." -ForegroundColor Red
    Write-Host "Listing vcpkg downloads/tools/msys2 contents:"
    Get-ChildItem "$env:USERPROFILE/vcpkg/downloads/tools/msys2" -Recurse
    throw "MSYS2 runtimes missing — winflexbison cannot run without msys DLLs."
}

$msysBin = Join-Path $msysRoot.FullName "usr/bin"
$tools = "$env:USERPROFILE/vcpkg/installed/x64-windows/tools/winflexbison"

Write-Host "Found MSYS2 at $msysRoot"
Write-Host "Copying all MSYS2 runtime DLLs from $msysBin to $tools..."

$msysDlls = Get-ChildItem -Path $msysBin -Filter "msys-*.dll" -File

if (-not $msysDlls) {
    Write-Host "WARNING: no msys-*.dll files found in $msysBin" -ForegroundColor Yellow
} else {
    foreach ($dll in $msysDlls) {
        Copy-Item $dll.FullName $tools -Force
        Write-Host "Copied $($dll.Name)"
    }
}

Write-Host "Testing win_bison.exe --version..."
& "$tools/win_bison.exe" --version
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: win_bison.exe --version failed after copying DLLs" -ForegroundColor Red
    exit 1
}

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"