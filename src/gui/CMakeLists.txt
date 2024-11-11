# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES Schematic.cpp Naja.cpp)

add_executable(naja_gui ${SOURCES}
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_opengl3.cpp
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
)
target_include_directories(naja_gui PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
target_include_directories(naja_gui PRIVATE ${ARGPARSE_DIR})
target_link_libraries(naja_gui 
    naja_snl_verilog naja_snl_pyloader naja_snl_liberty
    naja_snl_dump
    GLEW::GLEW glfw)
