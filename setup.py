from skbuild import setup

setup(
    name="naja",
    version="0.1.0",
    description="A Python package with a C++ backend built using CMake",
    author="Your Name",
    author_email="your.email@example.com",
    license="MIT",
    packages=["naja"],
    cmake_install_dir="lib/python/naja",
    include_package_data=True,
)
