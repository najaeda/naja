# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

#import platform
from skbuild import setup

#library_names = []

# Determine library based on OS
#if platform.system() == "Darwin":
#    libraries = [f"lib{x}.dylib" for x in library_names]
#else:  # Assume Linux
#    libraries = [f"lib{x}.so" for x in library_names]
#
#libraries.append("install/lib/python/naja/snl.so")
#libraries.append("snl.so")

setup(
    name="naja",
    version="0.1.0",
    description="Naja Python package",
    author="Naja Authors",
    author_email="contact@keplertech.io",
    license="Apache License 2.0",
    packages=["naja"],
    #package_dir={"naja": "src/naja_python"},
    #cmake_install_dir="install",
    #cmake_install_dir="src/naja_python",
    #include_package_data=True,
    #package_data={"naja": libraries},
    cmake_args=[
        "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON",      # Enable verbose output
        "-DCMAKE_BUILD_TYPE=Release",            # Set the build type
        "-DBUILD_NAJA_PYTHON=ON",                # Enable the Python package
    ],
)
