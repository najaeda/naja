set(SOURCES
  PySNLUniverse.cpp
  PySNLDB.cpp PySNLLibrary.cpp PySNLDesign.cpp
  PySNLDesignObject.cpp
  PySNLNet.cpp PySNLBusNet.cpp PySNLBitNet.cpp PySNLScalarNet.cpp PySNLBusNetBit.cpp
  PySNLNetComponent.cpp
  PySNLTerm.cpp PySNLBusTerm.cpp PySNLBitTerm.cpp PySNLScalarTerm.cpp PySNLBusTermBit.cpp
  PySNLInstance.cpp
  PySNL.cpp
)

Python3_add_library(snl MODULE ${SOURCES})

target_link_libraries(snl PRIVATE naja_snl)
