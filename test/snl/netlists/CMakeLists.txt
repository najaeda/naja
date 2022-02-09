set(SOURCES SNLNetlists.cpp)

add_library(snl_netlists SHARED ${SOURCES})

TARGET_LINK_LIBRARIES(snl_netlists naja_snl)