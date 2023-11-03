#[=======================================================================[.rst:
# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
FindNaja
-----------

Find the naja libraries.

``Naja::SNL``
  Naja SNL library

^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Naja_FOUND``
  true if Naja headers and libraries were found
``NAJA_INCLUDE_DIRS``
  list of the include directories needed to use Naja
``NAJA_LIBRARIES``
  Naja libraries to be linked

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``NAJA_INCLUDE_DIR``
  the directory containing Naja headers
``NAJA_SNL_LIBRARY``
  path to the Naja library
``NAJA_SNL_VERILOG_LIBRARY``
  path to the Naja verilog library
#]=======================================================================]

find_path(NAJA_INCLUDE_DIR NAMES SNLUniverse.h
   HINTS
   $ENV{NAJA_INSTALL}/include
)

find_library(NAJA_SNL_LIBRARY NAMES naja_snl
   HINTS
   $ENV{NAJA_INSTALL}/lib
)

find_library(NAJA_SNL_VERILOG_LIBRARY NAMES naja_snl_verilog
   HINTS
   $ENV{NAJA_INSTALL}/lib
)

find_library(NAJA_SNL_PYLOADER_LIBRARY NAMES naja_snl_pyloader
   HINTS
   $ENV{NAJA_INSTALL}/lib
)

set(NAJA_INCLUDE_DIRS ${NAJA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Naja
  REQUIRED_VARS NAJA_INCLUDE_DIR
  NAJA_SNL_LIBRARY NAJA_SNL_VERILOG_LIBRARY NAJA_SNL_PYLOADER_LIBRARY)

mark_as_advanced(NAJA_INCLUDE_DIR
  NAJA_SNL_LIBRARY NAJA_SNL_VERILOG_LIBRARY NAJA_SNL_PYLOADER_LIBRARY)

if(Naja_FOUND AND NOT TARGET Naja::SNL)
  add_library(Naja::SNL UNKNOWN IMPORTED)
  set_target_properties(Naja::SNL PROPERTIES
    IMPORTED_LOCATION ${NAJA_SNL_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${NAJA_INCLUDE_DIRS})

  add_library(Naja::SNLPyLoader UNKNOWN IMPORTED)
  set_target_properties(Naja::SNLPyLoader PROPERTIES
    IMPORTED_LOCATION ${NAJA_SNL_PYLOADER_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${NAJA_INCLUDE_DIRS}
    IMPORTED_LINK_INTERFACE_LIBRARIES Naja::SNL)

  add_library(Naja::SNLVerilog UNKNOWN IMPORTED)
  set_target_properties(Naja::SNLVerilog PROPERTIES
    IMPORTED_LOCATION ${NAJA_SNL_VERILOG_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${NAJA_INCLUDE_DIRS}
    IMPORTED_LINK_INTERFACE_LIBRARIES Naja::SNLPyLoader)
endif()
