set(SOURCES
  DNLInstance.cpp
  DNLEqui.cpp
  DNLDB.cpp
  DNLSNLConstructor.cpp
)

add_library(naja_dnl SHARED ${SOURCES})
target_link_libraries(naja_dnl coverage_config)

target_compile_options(naja_dnl PRIVATE ${NAJA_CXX_WARNINGS}) 

#target_include_directories(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
#target_include_directories(naja_snl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#set_target_properties(naja_snl PROPERTIES PUBLIC_HEADER "${HEADERS}")

#install(TARGETS naja_snl LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 