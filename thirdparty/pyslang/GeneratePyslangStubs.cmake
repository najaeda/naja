# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED NAJA_PYSLANG_STUBGEN_PYTHON)
  message(FATAL_ERROR "NAJA_PYSLANG_STUBGEN_PYTHON is not set")
endif()

if(DEFINED ENV{DESTDIR})
  set(NAJA_PYSLANG_INSTALL_ROOT "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
else()
  set(NAJA_PYSLANG_INSTALL_ROOT "${CMAKE_INSTALL_PREFIX}")
endif()

set(NAJA_PYSLANG_PACKAGE_DIR "${NAJA_PYSLANG_INSTALL_ROOT}/najaeda")
set(NAJA_PYSLANG_STUB_OUT "${NAJA_PYSLANG_PACKAGE_DIR}/_pyslang_stubgen")

file(GLOB NAJA_PYSLANG_EXTENSIONS
  "${NAJA_PYSLANG_PACKAGE_DIR}/pyslang*.so"
  "${NAJA_PYSLANG_PACKAGE_DIR}/pyslang*.pyd"
  "${NAJA_PYSLANG_PACKAGE_DIR}/pyslang*.dylib"
)
if(NOT NAJA_PYSLANG_EXTENSIONS)
  message(FATAL_ERROR
    "Cannot generate bundled pyslang stubs: no installed extension in "
    "${NAJA_PYSLANG_PACKAGE_DIR}")
endif()

file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env
          "PYTHONPATH=${NAJA_PYSLANG_INSTALL_ROOT}"
          ${NAJA_PYSLANG_STUBGEN_PYTHON} -m pybind11_stubgen
          najaeda.pyslang
          -o "${NAJA_PYSLANG_STUB_OUT}"
          --root-suffix ""
          --ignore-invalid-expressions=all
  RESULT_VARIABLE NAJA_PYSLANG_STUBGEN_RESULT
  OUTPUT_VARIABLE NAJA_PYSLANG_STUBGEN_OUTPUT
  ERROR_VARIABLE NAJA_PYSLANG_STUBGEN_ERROR
)
if(NOT NAJA_PYSLANG_STUBGEN_RESULT EQUAL 0)
  file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")
  message(FATAL_ERROR
    "Bundled pyslang stub generation failed:\n"
    "${NAJA_PYSLANG_STUBGEN_OUTPUT}\n${NAJA_PYSLANG_STUBGEN_ERROR}")
endif()

set(NAJA_PYSLANG_GENERATED_DIR
  "${NAJA_PYSLANG_STUB_OUT}/najaeda/pyslang")
if(NOT EXISTS "${NAJA_PYSLANG_GENERATED_DIR}/__init__.pyi")
  file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")
  message(FATAL_ERROR "Bundled pyslang stub generator produced no root stub")
endif()

set(NAJA_PYSLANG_STUB_DIR "${NAJA_PYSLANG_PACKAGE_DIR}/pyslang")
file(MAKE_DIRECTORY "${NAJA_PYSLANG_STUB_DIR}")
foreach(NAJA_PYSLANG_STUB IN ITEMS
    __init__.pyi analysis.pyi ast.pyi driver.pyi parsing.pyi syntax.pyi)
  file(REMOVE "${NAJA_PYSLANG_STUB_DIR}/${NAJA_PYSLANG_STUB}")
endforeach()
file(COPY "${NAJA_PYSLANG_GENERATED_DIR}/"
     DESTINATION "${NAJA_PYSLANG_STUB_DIR}"
     FILES_MATCHING PATTERN "*.pyi")

# Normalize C++-derived signatures that are not legal Python, then validate
# every emitted stub before packaging it.
file(GLOB NAJA_PYSLANG_GENERATED_STUBS
     "${NAJA_PYSLANG_STUB_DIR}/*.pyi")
execute_process(
  COMMAND ${NAJA_PYSLANG_STUBGEN_PYTHON}
          "${CMAKE_CURRENT_LIST_DIR}/NormalizePyslangStubs.py"
          ${NAJA_PYSLANG_GENERATED_STUBS}
  RESULT_VARIABLE NAJA_PYSLANG_STUB_NORMALIZE_RESULT
  ERROR_VARIABLE NAJA_PYSLANG_STUB_NORMALIZE_ERROR
)
if(NOT NAJA_PYSLANG_STUB_NORMALIZE_RESULT EQUAL 0)
  file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")
  message(FATAL_ERROR
    "Bundled pyslang stub normalization failed:\n"
    "${NAJA_PYSLANG_STUB_NORMALIZE_ERROR}")
endif()

foreach(NAJA_PYSLANG_GENERATED_STUB IN LISTS NAJA_PYSLANG_GENERATED_STUBS)
  execute_process(
    COMMAND ${NAJA_PYSLANG_STUBGEN_PYTHON} -c
            "import ast,pathlib,sys; p=pathlib.Path(sys.argv[1]); ast.parse(p.read_text(encoding='utf-8'), filename=str(p))"
            "${NAJA_PYSLANG_GENERATED_STUB}"
    RESULT_VARIABLE NAJA_PYSLANG_STUB_PARSE_RESULT
    ERROR_VARIABLE NAJA_PYSLANG_STUB_PARSE_ERROR
  )
  if(NOT NAJA_PYSLANG_STUB_PARSE_RESULT EQUAL 0)
    file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")
    message(FATAL_ERROR
      "Generated bundled pyslang stub is invalid: "
      "${NAJA_PYSLANG_GENERATED_STUB}\n${NAJA_PYSLANG_STUB_PARSE_ERROR}")
  endif()
endforeach()

file(REMOVE_RECURSE "${NAJA_PYSLANG_STUB_OUT}")

message(STATUS "Generated bundled pyslang type stubs")
