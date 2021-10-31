SET(SOURCES
  SNLObject.cpp
  SNLUniverse.cpp SNLDB.cpp SNLLibrary.cpp SNLDesign.cpp SNLDesignObject.cpp
  SNLNet.cpp SNLBusNet.cpp SNLBitNet.cpp SNLScalarNet.cpp
  SNLTerm.cpp SNLBusTerm.cpp SNLBitTerm.cpp SNLScalarTerm.cpp
  SNLInstance.cpp
  #SNLLevelize.cpp SNLDump.cpp
)

ADD_LIBRARY(naja_snl SHARED ${SOURCES})
TARGET_LINK_LIBRARIES(naja_snl naja_core)

TARGET_INCLUDE_DIRECTORIES(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
TARGET_INCLUDE_DIRECTORIES(naja_snl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#ADD_SUBDIRECTORY(swig)
