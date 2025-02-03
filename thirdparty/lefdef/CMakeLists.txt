# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef>" -*-

 set(CMAKE_LEGACY_CYGWIN_WIN32 0)
 project(LEFDEF)

 cmake_minimum_required(VERSION 3.16)
 set(ignoreVariables "${BUILD_DOC} ${CMAKE_INSTALL_DIR}")
 option(USE_LIBBFD     "Link with BFD libraries to print stack traces" OFF)

 find_package(BISON              REQUIRED)
 find_package(FLEX               REQUIRED)
 
 add_subdirectory(src)
 #add_subdirectory(doc)
