# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(snl_snippet ${PROJECT_SOURCE_DIR}/src/snl/snippets/app/src/SNLSnippet.cpp)
target_link_libraries(snl_snippet naja_snl)