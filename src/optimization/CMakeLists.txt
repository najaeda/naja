# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Add subdirectories - each will build their own libraries
add_subdirectory(constant_propagation)
add_subdirectory(loadless_logic)
add_subdirectory(reduction)

# Main optimization library that links all sub-modules
add_library(naja_opt INTERFACE)
target_link_libraries(naja_opt 
  INTERFACE 
    naja_constant_propagation
    naja_loadless_logic
    naja_reduction
    naja_bne 
    naja_dnl 
    coverage_config 
    sanitizers_config
)

target_include_directories(naja_opt 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SPDLOG_DIR}
)

target_include_directories(naja_opt SYSTEM 
  INTERFACE 
    ${Boost_INCLUDE_DIR}
)

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_opt LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda)
else(BUILD_NAJA_PYTHON)
install(TARGETS naja_opt
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)
