# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

find_package(benchmark REQUIRED)

add_executable(snl_bench
  SNLNetlistBench.cpp
  ${CMAKE_SOURCE_DIR}/test/nl/netlists/SNLNetlist0.cpp
)

target_include_directories(snl_bench PRIVATE
  ${CMAKE_SOURCE_DIR}/test/nl/netlists
)

target_link_libraries(snl_bench PRIVATE
  benchmark::benchmark
  naja_nl
)
