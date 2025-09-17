# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(naja_app NajaSnippet.cpp NLUniverseSnippet.cpp)
target_include_directories(naja_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(naja_app Naja::SNL)