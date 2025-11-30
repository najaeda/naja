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
$msysRoot = Get-ChildItem "$env:USERPROFILE/vcpkg/downloads/tools/msys2" |
    Where-Object { $_.PSIsContainer } |
    Select-Object -First 1

$msysBin = "$($msysRoot.FullName)/usr/bin"
$tools = "$env:USERPROFILE/vcpkg/installed/x64-windows/tools/winflexbison"

Write-Host "Copying MSYS2 DLLs to winflexbison folder..."

Copy-Item "$msysBin/msys-2.0.dll" $tools -Force
Copy-Item "$msysBin/msys-intl-8.dll" $tools -Force
Copy-Item "$msysBin/msys-iconv-2.dll" $tools -Force

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"