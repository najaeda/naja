#[=======================================================================[.rst:
FindNaja
-----------

Find the naja libraries.

``Naja::Naja``
  Naja libraries

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
``NAJA_LIBRARY``
  path to the Naja library
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

set(NAJA_INCLUDE_DIRS ${NAJA_INCLUDE_DIR})
list(APPEND NAJA_INCLUDE_DIRS "${NAJA_INCLUDE_DIR}")

set(NAJA_LIBRARIES ${NAJA_SNL_LIBRARY} ${NAJA_SNL_VERILOG_LIBRARY})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Naja
                                  REQUIRED_VARS NAJA_SNL_LIBRARY NAJA_INCLUDE_DIR)

mark_as_advanced(NAJA_INCLUDE_DIR NAJA_SNL_LIBRARY)

if(Naja_FOUND AND NOT TARGET Naja::Naja)
  add_library(Naja::Naja UNKNOWN IMPORTED)
  set_target_properties(Naja::Naja PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NAJA_INCLUDE_DIRS}")
  set_property(TARGET Naja::Naja APPEND PROPERTY IMPORTED_LOCATION "${NAJA_LIBRARIES}")
endif()
