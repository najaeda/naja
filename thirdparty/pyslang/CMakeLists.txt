# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

find_package(pybind11 3.0.4 EXACT CONFIG REQUIRED)

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c
          "import importlib.metadata as m; assert m.version('pybind11-stubgen') == '2.5.5'"
  RESULT_VARIABLE NAJA_PYSLANG_STUBGEN_RESULT
  OUTPUT_QUIET
  ERROR_QUIET
)
if(NOT NAJA_PYSLANG_STUBGEN_RESULT EQUAL 0)
  message(FATAL_ERROR
    "BUILD_NAJA_PYSLANG requires pybind11-stubgen==2.5.5 for reproducible stubs")
endif()

set(NAJA_PYSLANG_BINDINGS_DIR
  "${PROJECT_SOURCE_DIR}/thirdparty/slang/bindings/python")
set(NAJA_PYSLANG_SCRIPTS_DIR
  "${PROJECT_SOURCE_DIR}/thirdparty/slang/scripts")

add_custom_command(
  COMMAND
    ${Python3_EXECUTABLE} ${NAJA_PYSLANG_SCRIPTS_DIR}/syntax_gen.py --dir
    ${CMAKE_CURRENT_BINARY_DIR} --python-bindings --syntax
    ${NAJA_PYSLANG_SCRIPTS_DIR}/syntax.txt
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings0.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings1.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings2.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings3.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxFactory.cpp
  DEPENDS ${NAJA_PYSLANG_SCRIPTS_DIR}/syntax_gen.py
          ${NAJA_PYSLANG_SCRIPTS_DIR}/syntax.txt
  COMMENT "Generating bundled pyslang syntax bindings"
)

set(NAJA_PYSLANG_SOURCES
  ${NAJA_PYSLANG_BINDINGS_DIR}/AnalysisBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/ASTBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/CompBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/ExpressionBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/NumericBindings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NajaPyslangNumericShim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NajaPyslang.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/StatementBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/SymbolBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/SyntaxBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/TypeBindings.cpp
  ${NAJA_PYSLANG_BINDINGS_DIR}/UtilBindings.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings0.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings1.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings2.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxBindings3.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/PySyntaxFactory.cpp
)

pybind11_add_module(naja_pyslang MODULE ${NAJA_PYSLANG_SOURCES})
set_target_properties(naja_pyslang PROPERTIES OUTPUT_NAME pyslang)

# literalBaseFromChar is the one function referenced by upstream's Python
# bindings that is not exported from a shared slang build. Redirect just that
# binding to an equivalent local shim without altering the slang submodule.
set_source_files_properties(
  ${NAJA_PYSLANG_BINDINGS_DIR}/NumericBindings.cpp
  PROPERTIES COMPILE_DEFINITIONS
    "literalBaseFromChar=NajaPyslang_literalBaseFromChar"
)

# Upstream slang bindings contain a few literal `pybind11` namespace uses.
# Rename that token as well as pybind11's configurable namespace, and give the
# bundled module a private internals / conduit key. This prevents an external
# top-level pyslang from sharing class or native-enum registries with us.
target_compile_definitions(naja_pyslang PRIVATE
  pybind11=naja_pybind11
  PYBIND11_NAMESPACE=naja_pybind11
  PYBIND11_COMPILER_TYPE="naja_pyslang"
)

target_include_directories(naja_pyslang PRIVATE
  ${NAJA_PYSLANG_BINDINGS_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(naja_pyslang PRIVATE
  slang::slang
  naja_python_native_api
)

install(TARGETS naja_pyslang
  LIBRARY DESTINATION najaeda
  RUNTIME DESTINATION najaeda
)

# Mark the distribution as typed and generate stubs from the extension that was
# just installed. Running against the installed image also exercises its final
# loader paths during wheel construction.
install(
  FILES ${PROJECT_SOURCE_DIR}/thirdparty/slang/pyslang/pyslang/py.typed
  DESTINATION najaeda
)
install(CODE "
  set(NAJA_PYSLANG_STUBGEN_PYTHON \"${Python3_EXECUTABLE}\")
  include(\"${CMAKE_CURRENT_SOURCE_DIR}/GeneratePyslangStubs.cmake\")
")
