set(SNL_KERNEL_SOURCES
  kernel/SNLObject.cpp kernel/SNLID.cpp
  kernel/SNLUniverse.cpp kernel/SNLDB.cpp kernel/SNLDB0.cpp
  kernel/SNLLibrary.cpp
  kernel/SNLDesignObject.cpp
  kernel/SNLNet.cpp kernel/SNLBusNet.cpp
  kernel/SNLBitNet.cpp kernel/SNLScalarNet.cpp kernel/SNLBusNetBit.cpp
  kernel/SNLNetComponent.cpp
  kernel/SNLTerm.cpp kernel/SNLBitTerm.cpp 
  kernel/SNLBusTerm.cpp kernel/SNLScalarTerm.cpp
  kernel/SNLBusTermBit.cpp
  kernel/SNLInstance.cpp kernel/SNLInstTerm.cpp
  kernel/SNLDesign.cpp
  kernel/SNLSharedPath.cpp kernel/SNLPath.cpp
  kernel/SNLParameter.cpp
)

set(SNL_KERNEL_HEADERS
  kernel/SNLName.h
  kernel/SNLObject.h kernel/SNLID.h
  kernel/SNLUniverse.h kernel/SNLDB.h
  kernel/SNLLibrary.h kernel/SNLDesign.h
  kernel/SNLDesignObject.h
  kernel/SNLNet.h kernel/SNLBusNet.h
  kernel/SNLBitNet.h kernel/SNLScalarNet.h kernel/SNLBusNetBit.cpp
  kernel/SNLNetComponent.h
  kernel/SNLTerm.h kernel/SNLBitTerm.h 
  kernel/SNLBusTerm.h kernel/SNLScalarTerm.h
  kernel/SNLBusTermBit.h
  kernel/SNLInstance.h kernel/SNLInstTerm.h
  kernel/SNLSharedPath.h kernel/SNLPath.h
  kernel/SNLParameter.h
)

set(SNL_DUMP_SOURCES
  dump/SNLDumpManifest.cpp
  dump/SNLDumper.cpp
  dump/SNLLoader.cpp
)

set(SNL_FLATTENER_SOURCES
  flattener/SNLFlattener.cpp
  flattener/SNLFlattenerInstanceTreeNode.cpp 
  flattener/SNLFlattenerInstanceTree.cpp
)

add_library(naja_snl SHARED ${SNL_KERNEL_SOURCES} ${SNL_DUMP_SOURCES})
target_link_libraries(naja_snl naja_core coverage_config)

target_compile_options(naja_snl PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_snl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/kernel
  ${CMAKE_CURRENT_SOURCE_DIR}/dump
  ${CMAKE_CURRENT_SOURCE_DIR}/flattener
)

set_target_properties(naja_snl PROPERTIES PUBLIC_HEADER "${SNL_KERNEL_HEADERS}")

install(TARGETS naja_snl LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 