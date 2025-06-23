# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(DNL_SOURCES
  DNL.cpp
)
set(DNL_HEADERS
  DNL.h DNL_impl.h
)

add_library(naja_dnl SHARED ${DNL_SOURCES})

target_link_libraries(naja_dnl PUBLIC naja_nl TBB::tbb TBB::tbbmalloc)
if(APPLE)
  #xtof: 23/04/2025
  # avoid on macOS (Ventura) the following error:
  # /usr/local/include/oneapi/tbb/detail/_utils.h:174:31: error: implicit conversion changes signedness: 'int' to 'unsigned long' [-Werror,-Wsign-conversion]
  #   return 0 == (arg & (arg - divisor));
  # /usr/local/include/tbb/../oneapi/tbb/concurrent_vector.h:734:16: note: in instantiation of function template specialization
  # 'tbb::detail::is_power_of_two_at_least<unsigned long, int>' requested here
  # return is_power_of_two_at_least(index, 2);
  target_compile_options(naja_dnl PRIVATE -Wno-sign-conversion)
endif()
target_compile_options(naja_dnl BEFORE PRIVATE ${NAJA_CXX_WARNINGS}) 
#target_compile_definitions(naja_dnl PRIVATE SNL_DESTROY_DEBUG)

target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${TBB_INCLUDE_DIR})
target_include_directories(naja_dnl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_dnl LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda)
else(BUILD_NAJA_PYTHON)
  set_target_properties(naja_dnl PROPERTIES PUBLIC_HEADER "${DNL_HEADERS}")
  install(TARGETS naja_dnl
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)