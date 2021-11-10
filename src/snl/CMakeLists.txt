SET(SOURCES
  SNLObject.cpp
  SNLUniverse.cpp SNLDB.cpp SNLDB0.cpp
  SNLLibrary.cpp SNLDesign.cpp
  SNLDesignObject.cpp
  SNLNet.cpp SNLBusNet.cpp SNLBitNet.cpp SNLScalarNet.cpp
  SNLNetComponent.cpp
  SNLTerm.cpp SNLBitTerm.cpp 
  SNLBusTerm.cpp SNLScalarTerm.cpp
  SNLBusTermBit.cpp
  SNLInstance.cpp SNLInstTerm.cpp
  #SNLLevelize.cpp SNLDump.cpp
)

ADD_LIBRARY(naja_snl SHARED ${SOURCES})
TARGET_LINK_LIBRARIES(naja_snl naja_core)

TARGET_INCLUDE_DIRECTORIES(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
TARGET_INCLUDE_DIRECTORIES(naja_snl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#ADD_SUBDIRECTORY(swig)
