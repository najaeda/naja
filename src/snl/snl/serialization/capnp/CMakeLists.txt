capnp_generate_cpp(snlInterfaceSources snlInterfaceHeaders snl_interface.capnp)
capnp_generate_cpp(snlImplementationSources snlImplementationHeaders snl_implementation.capnp)

add_library(naja_snl_capnp
    ${snlInterfaceSources} ${snlImplementationSources}
    SNLCapnP.cpp
    SNLCapnPInterface.cpp SNLCapnPImplementation.cpp)

target_link_libraries(naja_snl_capnp PRIVATE CapnProto::capnp )
target_link_libraries(naja_snl_capnp PUBLIC naja_snl)
target_include_directories(naja_snl_capnp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(naja_snl_capnp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#set(SNL_CAPN_HEADERS SNLCapNp.h)
