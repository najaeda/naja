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

# Create symlinks so CMake FindBISON/FIND_FLEX can detect them
$tools = "$env:USERPROFILE/vcpkg/installed/x64-windows/tools/winflexbison"
New-Item -ItemType SymbolicLink -Path "$tools/bison.exe" -Target "$tools/win_bison.exe" -Force
New-Item -ItemType SymbolicLink -Path "$tools/flex.exe" -Target "$tools/win_flex.exe" -Force

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"