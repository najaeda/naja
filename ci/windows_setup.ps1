# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

# The wheel cache key includes this script, so dependency changes refresh it.
$vcpkgRevision = '10541e317a660f4165ba4ac2851ab54a8d4577b1'
$vcpkgRoot = "$env:USERPROFILE/vcpkg"
Write-Host "Installing vcpkg ($vcpkgRevision)"

git init $vcpkgRoot
git -C $vcpkgRoot fetch --depth 1 https://github.com/microsoft/vcpkg.git $vcpkgRevision
git -C $vcpkgRoot checkout --detach FETCH_HEAD
& "$vcpkgRoot/bootstrap-vcpkg.bat" -disableMetrics

# vcpkg validates each binary's ABI before reusing it from the restored cache.
$packages = @(
    'capnproto:x64-windows'
    'tbb:x64-windows'
    'boost-intrusive:x64-windows'
    'boost-dynamic-bitset:x64-windows'
    'boost-multiprecision:x64-windows'
    'boost-unordered:x64-windows'
    'boost-regex:x64-windows'
)
& "$vcpkgRoot/vcpkg.exe" install @packages

# Export for CMake
Add-Content $env:GITHUB_ENV "CMAKE_TOOLCHAIN_FILE=$env:USERPROFILE/vcpkg/scripts/buildsystems/vcpkg.cmake"
Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$env:USERPROFILE/vcpkg"
