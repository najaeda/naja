# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

capnp_generate_cpp(snlCommonSources snlCommonHeaders snl_common.capnp)
capnp_generate_cpp(snlInterfaceSources snlInterfaceHeaders snl_interface.capnp)
capnp_generate_cpp(snlImplementationSources snlImplementationHeaders snl_implementation.capnp)

add_library(naja_nl_dump STATIC
    ${snlCommonSources} ${snlInterfaceSources} ${snlImplementationSources}
    SNLDump.cpp SNLDumpManifest.cpp
    SNLCapnP.cpp
    SNLCapnPInterface.cpp SNLCapnPImplementation.cpp)

set_target_properties(naja_nl_dump PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_nl_dump PRIVATE CapnProto::capnp )
target_link_libraries(naja_nl_dump PUBLIC naja_nl)
target_include_directories(naja_nl_dump PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(naja_nl_dump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
install(
    FILES snl_common.capnp snl_interface.capnp snl_implementation.capnp snl_common.capnp
    DESTINATION ${CMAKE_INSTALL_DATADIR}/naja/snlif)
endif(NOT BUILD_NAJA_PYTHON)