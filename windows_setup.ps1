# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

Write-Host "Installing vcpkg (latest)"

git clone https://github.com/microsoft/vcpkg.git $env:USERPROFILE/vcpkg
& "$env:USERPROFILE/vcpkg/bootstrap-vcpkg.bat"

# Install dependencies
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install capnproto:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install tbb:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install flex:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install bison:x64-windows
& "$env:USERPROFILE/vcpkg/vcpkg.exe" install boost:x64-windows

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"