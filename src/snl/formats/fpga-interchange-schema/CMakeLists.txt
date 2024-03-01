# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(FPGA_INTERCHANGE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/fpga-interchange-schema/interchange)
set(CAPNPC_SRC_PREFIX ${FPGA_INTERCHANGE_DIR})
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})


capnp_generate_cpp(snlFIFReferencesSources snlFIFReferencesHeaders ${FPGA_INTERCHANGE_DIR}/References.capnp)
capnp_generate_cpp(snlFIFNetlistSources snlFIFNetlistHeaders ${FPGA_INTERCHANGE_DIR}/LogicalNetlist.capnp)

add_library(naja_snl_fpga_interchange_format
    ${snlFIFReferencesSources}
    ${snlFIFNetlistSources}
    SNLFIFNetlist.cpp
    #SNLDump.cpp SNLDumpManifest.cpp
    #SNLCapnP.cpp
    #SNLCapnPInterface.cpp SNLCapnPImplementation.cpp)
)

target_link_libraries(naja_snl_fpga_interchange_format PRIVATE CapnProto::capnp )
target_link_libraries(naja_snl_fpga_interchange_format PUBLIC naja_snl)
target_include_directories(naja_snl_fpga_interchange_format PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(naja_snl_fpga_interchange_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})