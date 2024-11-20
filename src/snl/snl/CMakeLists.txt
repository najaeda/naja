# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(NOT BUILD_NAJA_PYTHON)
  add_subdirectory(serialization)
  add_subdirectory(visual)
endif()

set(SNL_KERNEL_SOURCES
  kernel/SNLObject.cpp
  kernel/SNLAttributes.cpp
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
  kernel/SNLParameter.cpp kernel/SNLInstParameter.cpp
  kernel/SNLUtils.cpp
  kernel/SNLOccurrence.cpp kernel/SNLBitNetOccurrence.cpp
  kernel/SNLNetComponentOccurrence.cpp 
  kernel/SNLBitTermOccurrence.cpp kernel/SNLInstTermOccurrence.cpp
  kernel/SNLEquipotential.cpp
  kernel/SNLUniquifier.cpp
)
set(SNL_DECORATOR_SOURCES
  decorators/SNLDesignModeling.cpp
  decorators/SNLLibraryTruthTables.cpp
  decorators/SNLDesignTruthTable.cpp
)

set(SNL_HEADERS
  kernel/SNLName.h
  kernel/SNLID.h
  kernel/SNLObject.h
  kernel/SNLUniverse.h kernel/SNLDB.h
  kernel/SNLLibrary.h kernel/SNLDesign.h
  kernel/SNLDesignObject.h
  kernel/SNLNet.h kernel/SNLBusNet.h
  kernel/SNLBitNet.h kernel/SNLScalarNet.h kernel/SNLBusNetBit.h
  kernel/SNLNetComponent.h
  kernel/SNLTerm.h kernel/SNLBitTerm.h 
  kernel/SNLBusTerm.h kernel/SNLScalarTerm.h
  kernel/SNLBusTermBit.h
  kernel/SNLInstance.h kernel/SNLInstTerm.h
  kernel/SNLSharedPath.h kernel/SNLPath.h
  kernel/SNLParameter.h kernel/SNLInstParameter.h
  kernel/SNLException.h
  kernel/SNLUtils.h
  kernel/SNLOccurrence.h
  kernel/SNLBitNetOccurrence.h
  kernel/SNLNetComponentOccurrence.h
  kernel/SNLBitTermOccurrence.h
  kernel/SNLInstTermOccurrence.h
  kernel/SNLEquipotential.h
  decorators/SNLDesignModeling.h
  decorators/SNLTruthTable.h
  decorators/SNLLibraryTruthTables.h
)

add_library(naja_snl STATIC ${SNL_KERNEL_SOURCES} ${SNL_DECORATOR_SOURCES})
set_property(TARGET naja_snl PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl PUBLIC naja_core)
target_compile_options(naja_snl PRIVATE ${NAJA_CXX_WARNINGS}) 

#target_compile_definitions(naja_snl PRIVATE SNL_DESTROY_DEBUG)

target_include_directories(naja_snl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_snl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/kernel
  ${CMAKE_CURRENT_SOURCE_DIR}/decorators
)
target_include_directories(naja_snl PUBLIC ${SPDLOG_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl PROPERTIES PUBLIC_HEADER "${SNL_HEADERS}")
  install(TARGETS naja_snl
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(NOT BUILD_NAJA_PYTHON)