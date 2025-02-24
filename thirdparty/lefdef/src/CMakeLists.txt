# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src>" -*-

# Check if the compiler is GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wno-format-truncation -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object -Wno-other -Wno-format-overflow)
endif()

# Add these flags for Bison as well
if(DEFINED BISON_EXECUTABLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object")
endif()

# Ensure Ninja generator is used
if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object")
endif()

 add_subdirectory(lef)
 add_subdirectory(def)
