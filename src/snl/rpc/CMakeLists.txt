# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

capnp_generate_cpp(snlRPCSources snlRPCHeaders snl_rpc.capnp)

add_library(naja_snl_rpc
  ${snlRPCSources}
  SNLDBImpl.cpp
  SNLLibraryImpl.cpp
  SNLDesignImpl.cpp
  SNLTermImpl.cpp
  SNLBusTermImpl.cpp
  SNLNetComponentImpl.cpp
  SNLScalarTermImpl.cpp
  SNLBusTermBitImpl.cpp
  SNLInstTermImpl.cpp
  SNLBitNetImpl.cpp
  SNLBusNetBitImpl.cpp
  SNLNetImpl.cpp
  SNLBusNetImpl.cpp
  SNLInstanceImpl.cpp
  SNLIDConversion.cpp
  SNLServer.cpp
)

set(SNL_RPC_HEADERS ${snlRPCHeaders})
  
target_link_libraries(naja_snl_rpc PUBLIC CapnProto::capnp-rpc naja_snl)
target_include_directories(naja_snl_rpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(naja_snl_rpc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(naja_snl_rpc PROPERTIES PUBLIC_HEADER "${SNL_RPC_HEADERS}")
install(TARGETS naja_snl_rpc LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 