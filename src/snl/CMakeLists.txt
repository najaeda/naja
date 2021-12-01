set(SOURCES
  SNLObject.cpp
  SNLUniverse.cpp SNLDB.cpp SNLDB0.cpp
  SNLLibrary.cpp SNLDesign.cpp
  SNLDesignObject.cpp
  SNLNet.cpp SNLBusNet.cpp
  SNLBitNet.cpp SNLScalarNet.cpp SNLBusNetBit.cpp
  SNLNetComponent.cpp
  SNLTerm.cpp SNLBitTerm.cpp 
  SNLBusTerm.cpp SNLScalarTerm.cpp
  SNLBusTermBit.cpp
  SNLInstance.cpp SNLInstTerm.cpp
  SNLSharedPath.cpp SNLPath.cpp
  #SNLLevelize.cpp SNLDump.cpp
)

add_library(naja_snl SHARED ${SOURCES})
target_link_libraries(naja_snl naja_core coverage_config)

target_compile_options(naja_snl PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_snl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
