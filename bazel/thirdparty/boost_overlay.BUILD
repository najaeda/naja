# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Overlay BUILD file for the vendored Boost source (fetched via
# http_archive in MODULE.bazel from the official archives.boost.io
# release tarball, pinned to 1.89.0 -- superseding the earlier
# pkg-config/find_path-style bazel/thirdparty/boost/repo.bzl, which
# wrapped whatever Boost happened to be installed on the local machine.
# That approach meant the exact Boost version (and even which of several
# installed copies got picked, by search-order priority) silently varied
# by host, and a too-broad repo_ctx.symlink of the whole discovered
# include prefix (e.g. /usr/local/include) was found to leak unrelated
# system headers -- including an old libgtest-dev gtest.h that shadowed
# Bazel's own fetched @googletest headers on one machine (see git log).
# Vendoring a pinned release tarball removes both problems: identical
# Boost bits on every machine, and only boost/** ends up on the include
# path.
#
# naja only ever consumes Boost as headers (no compiled Boost::
# component library is linked anywhere in src/), matching
# CMakeLists.txt's own `find_path(Boost_INCLUDE_DIR NAMES
# boost/version.hpp)` header-only usage -- so, same as fmt/tomlplusplus
# above, only a glob of the headers is needed, no srcs to compile.

cc_library(
    name = "headers",
    hdrs = glob(["boost/**"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
