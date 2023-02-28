set(SOURCES
  PySNLUniverse.cpp
  PySNLDB.cpp PySNLLibrary.cpp PySNLDesign.cpp
  PySNLDesignObject.cpp
  PySNLNet.cpp PySNLBusNet.cpp PySNLBitNet.cpp PySNLScalarNet.cpp PySNLBusNetBit.cpp
  PySNLNetComponent.cpp
  PySNLTerm.cpp PySNLBusTerm.cpp PySNLBitTerm.cpp PySNLScalarTerm.cpp PySNLBusTermBit.cpp
  PySNLInstance.cpp
)

Python3_add_library(naja_snl_python SHARED ${SOURCES})
target_link_libraries(naja_snl_python PUBLIC naja_snl)
target_include_directories(naja_snl_python PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

Python3_add_library(snl MODULE PySNL.cpp)

target_link_libraries(snl PRIVATE naja_snl_python)