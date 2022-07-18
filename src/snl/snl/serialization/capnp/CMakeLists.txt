capnp_generate_cpp(snlInterfaceSources snlInterfaceHeaders snl_interface.capnp)

add_library(naja_snl_capnp ${snlInterfaceSources})

target_link_libraries(naja_snl_capnp PRIVATE CapnProto::capnp)
