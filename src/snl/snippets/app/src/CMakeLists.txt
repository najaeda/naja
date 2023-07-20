# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(snl_app SNLSnippet.cpp)
target_include_directories(snl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_app Naja::SNL)