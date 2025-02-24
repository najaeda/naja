# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/def>" -*-

# Check if the compiler is GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wno-format-truncation -Wno-deprecated -Wno-nonnull -Wno-free-nonheap-object -Wno-other)
endif()

 add_subdirectory(def)
 add_subdirectory(defzlib)
 add_subdirectory(defdiff)
 add_subdirectory(defrw)
 add_subdirectory(defwrite)
