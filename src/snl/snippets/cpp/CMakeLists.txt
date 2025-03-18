# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(snl_snippet
  ${PROJECT_SOURCE_DIR}/src/app_snippet/src/NLUniverseSnippet.cpp
  SNLSnippet.cpp
)
#add_executable(snl_receiver
#  ${PROJECT_SOURCE_DIR}/src/app_snippet/src/SNLUniverseSnippet.cpp
#  SNLReceiver.cpp
#)
#add_executable(snl_sender
#  ${PROJECT_SOURCE_DIR}/src/app_snippet/src/SNLUniverseSnippet.cpp
#  SNLSender.cpp
#)

target_link_libraries(snl_snippet naja_snl)
target_include_directories(snl_snippet PRIVATE ${PROJECT_SOURCE_DIR}/src/app_snippet/src)
#target_link_libraries(snl_receiver naja_snl_dump)
#target_include_directories(snl_receiver PRIVATE ${PROJECT_SOURCE_DIR}/src/app_snippet/src)
#target_link_libraries(snl_sender naja_snl_dump)
#target_include_directories(snl_sender PRIVATE ${PROJECT_SOURCE_DIR}/src/app_snippet/src)