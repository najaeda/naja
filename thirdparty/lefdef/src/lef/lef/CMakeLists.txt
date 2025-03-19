BISON_TARGET(LefParser lef.y ${CMAKE_CURRENT_BINARY_DIR}/lef.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lef.tab.h
  COMPILE_FLAGS "-Wconflicts-sr -d -v -p lefyy"
)

set(SOURCES
  ${BISON_LefParser_OUTPUTS}
  crypt.cpp
  lef_keywords.cpp
  lefiArray.cpp
  lefiCrossTalk.cpp
  lefiDebug.cpp
  lefiEncryptInt.cpp
  lefiLayer.cpp
  lefiMacro.cpp
  lefiMisc.cpp
  lefiNonDefault.cpp
  lefiProp.cpp
  lefiPropType.cpp
  lefiTBExt.cpp
  lefiUnits.cpp
  lefiVia.cpp
  lefiViaRule.cpp
  lefrCallbacks.cpp
  lefrData.cpp
  lefrReader.cpp
  lefrSettings.cpp
  lefwWriter.cpp
  lefwWriterCalls.cpp
)

add_library(lef ${SOURCES})
set_target_properties(lef PROPERTIES VERSION 5.8 SOVERSION 5)
target_include_directories(lef
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(includes lefiArray.hpp
               lefiCrossTalk.hpp
               lefiDebug.hpp
               lefiDefs.hpp
               lefiEncryptInt.hpp
               lefiKRDefs.hpp
               lefiLayer.hpp
               lefiMacro.hpp
               lefiMisc.hpp
               lefiNonDefault.hpp
               lefiProp.hpp
               lefiPropType.hpp
               lefiUnits.hpp
               lefiUser.hpp
               lefiUtil.hpp
               lefiVia.hpp
               lefiViaRule.hpp
               lefrCallBacks.hpp
               lefrData.hpp
               lefrReader.hpp
               lefrSettings.hpp
               lefwWriter.hpp
               lefwWriterCalls.hpp
)
      
#install ( TARGETS  lef DESTINATION lib${LIB_SUFFIX} )
#install ( FILES    ${includes} DESTINATION include/lefdef ) 
