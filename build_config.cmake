option(BUILD_NAJA_PYTHON "Build Naja Python package" OFF)
option(BUILD_ONLY_DOC "Only configure to build Doxygen documentation" OFF)
option(BUILD_EMSCRIPTEN "Build with Emscripten" OFF)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers in compilation" OFF)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.30")
  cmake_policy(SET CMP0167 OLD)
  cmake_policy(SET CMP0144 NEW)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24" AND NOT BUILD_EMSCRIPTEN)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

#RPATH settings
if(BUILD_NAJA_PYTHON)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
  endif()
  #set(CMAKE_INSTALL_RPATH "$ORIGIN/lib:$ORIGIN")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
  set(CMAKE_MACOSX_RPATH TRUE)
  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  ######
endif()

if(PROJECT_IS_TOP_LEVEL AND NOT BUILD_NAJA_PYTHON)
  option(LONG_TESTS "Enable long tests" OFF)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

if(BUILD_EMSCRIPTEN)
  add_compile_options(-fexceptions)
  add_link_options(
  "-sEXIT_RUNTIME=1"
  "-sASSERTIONS=0"
  "-sVERBOSE=0"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sDISABLE_EXCEPTION_CATCHING=0"
  "--verbose"
)
endif()

if(CODE_COVERAGE)
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  target_link_options(coverage_config INTERFACE --coverage)
endif(CODE_COVERAGE)

if(ENABLE_SANITIZERS)
  target_compile_options(sanitizers_config INTERFACE
    -fsanitize=address
    -fno-omit-frame-pointer
    #-fsanitize=thread
  )
  target_link_options(sanitizers_config INTERFACE
    -fsanitize=address
    #-fsanitize=thread
  )
endif(ENABLE_SANITIZERS)

if(BUILD_ONLY_DOC)
  option(BUILD_SPHINX_DOCUMENTATION "Build Sphinx documentation" OFF)
  
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
  # Doxygen
  # look for Doxygen package
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    add_subdirectory(docs)
  endif(DOXYGEN_FOUND)
  
  if(BUILD_SPHINX_DOCUMENTATION)
    #Sphinx
    #look for Sphinx package
    find_package(Sphinx)
    if (NOT (DOXYGEN_FOUND AND SPHINX_FOUND))
      message(SEND_ERROR "Doxygen and Sphinx must be available to build Sphinx documentation")
    endif(NOT (DOXYGEN_FOUND AND SPHINX_FOUND))
  endif(BUILD_SPHINX_DOCUMENTATION)
else()
  find_package(Boost REQUIRED)
  if(NOT BUILD_EMSCRIPTEN)
    find_package(TBB REQUIRED)
    find_package(CapnProto REQUIRED)
    if (NOT ${BUILD_NAJA_PYTHON})
      find_package(Python3 3.9...<3.99 COMPONENTS Interpreter Development.Embed Development.Module REQUIRED)
    else()
      find_package(Python3 3.9...<3.99 COMPONENTS Interpreter Development.Module REQUIRED)
    endif()
  endif(NOT BUILD_EMSCRIPTEN)

  add_subdirectory(thirdparty)
  add_subdirectory(src)
  if(NOT BUILD_NAJA_PYTHON)
    add_subdirectory(cmake)
    add_subdirectory(primitives)
  endif(NOT BUILD_NAJA_PYTHON)
endif()
