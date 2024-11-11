# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  PySNLUniverse.cpp
  PySNLDB.cpp PySNLLibrary.cpp PySNLDesign.cpp
  PySNLParameter.cpp
  PySNLDesignObject.cpp
  PySNLNet.cpp PySNLBusNet.cpp PySNLBitNet.cpp PySNLScalarNet.cpp PySNLBusNetBit.cpp
  PySNLNetType.cpp
  PySNLNetComponent.cpp
  PySNLTerm.cpp
  PySNLTermDirection.cpp 
  PySNLBusTerm.cpp PySNLBitTerm.cpp PySNLScalarTerm.cpp PySNLBusTermBit.cpp
  PySNLInstance.cpp PySNLInstParameter.cpp PySNLInstTerm.cpp
  PySNLLibraries.cpp PySNLDesigns.cpp
  PySNLParameters.cpp PySNLInstParameters.cpp
  PySNLNetComponents.cpp
  PySNLTerms.cpp PySNLBitTerms.cpp PySNLScalarTerms.cpp PySNLBusTerms.cpp
  PySNLNets.cpp PySNLBitNets.cpp PySNLScalarNets.cpp PySNLBusNets.cpp
  PySNLInstances.cpp PySNLInstTerms.cpp PySNLPath.cpp PySNLOccurrence.cpp
  PySNLNetComponentOccurrence.cpp PySNLInstTermOccurrence.cpp PySNLInstTermOccurrences.cpp
  PySNLEquipotential.cpp PySNLUniquifier.cpp
)

Python3_add_library(naja_snl_python SHARED ${SOURCES})
target_link_libraries(naja_snl_python PUBLIC naja_snl_verilog naja_bne naja_snl_dump)
target_include_directories(naja_snl_python PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

Python3_add_library(snl MODULE PySNL.cpp)

target_link_libraries(snl PRIVATE naja_snl_python)

install(TARGETS naja_snl_python LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}) 
install(TARGETS snl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python/naja)
