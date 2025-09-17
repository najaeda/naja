BISON_TARGET(DefParser def.y
  ${CMAKE_CURRENT_BINARY_DIR}/def.cpp
  COMPILE_FLAGS "-Wconflicts-sr -d -v -p defyy")

set(includes
  defiAlias.hpp defiAssertion.hpp defiBlockage.hpp 
  defiComponent.hpp defiDebug.hpp defiDefs.hpp 
  defiFPC.hpp defiFill.hpp defiGroup.hpp 
  defiIOTiming.hpp defiKRDefs.hpp defiMisc.hpp 
  defiNet.hpp defiNonDefault.hpp defiPartition.hpp 
  defiPath.hpp defiPinCap.hpp defiPinProp.hpp 
  defiProp.hpp defiPropType.hpp defiRegion.hpp 
  defiRowTrack.hpp defiScanchain.hpp defiSite.hpp 
  defiSlot.hpp defiTimingDisable.hpp defiUser.hpp 
  defiUtil.hpp defiVia.hpp defrCallBacks.hpp 
  defrData.hpp defrReader.hpp defrSettings.hpp 
  defwWriter.hpp defwWriterCalls.hpp
)

set(cpps
  ${BISON_DefParser_OUTPUTS}
  def_keywords.cpp defiAlias.cpp defiAssertion.cpp 
  defiBlockage.cpp defiComponent.cpp defiDebug.cpp 
  defiFPC.cpp defiFill.cpp defiGroup.cpp 
  defiIOTiming.cpp defiMisc.cpp defiNet.cpp 
  defiNonDefault.cpp defiPartition.cpp defiPath.cpp 
  defiPinCap.cpp defiPinProp.cpp defiProp.cpp 
  defiPropType.cpp defiRegion.cpp defiRowTrack.cpp 
  defiScanchain.cpp defiSite.cpp defiSlot.cpp 
  defiTimingDisable.cpp defiUtil.cpp defiVia.cpp 
  defrCallbacks.cpp defrData.cpp defrReader.cpp 
  defrSettings.cpp defwWriter.cpp defwWriterCalls.cpp
)
      
add_library(def ${cpps} )
set_target_properties(def PROPERTIES VERSION 5.8 SOVERSION 5)
target_include_directories(def
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
      
install(TARGETS def DESTINATION lib${LIB_SUFFIX})
install(FILES  ${includes} DESTINATION include/lefdef) 