# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

find_path(TBB_INCLUDE_DIR
  NAMES tbb/tbb.h
)

find_library(TBB_LIBRARY
  NAMES tbb
)

find_library(TBBMALLOC_LIBRARY
  NAMES tbbmalloc
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TBB
  REQUIRED_VARS
    TBB_INCLUDE_DIR
    TBB_LIBRARY
    TBBMALLOC_LIBRARY
)

if(TBB_FOUND)
  set(TBB_INCLUDE_DIRS "${TBB_INCLUDE_DIR}")
  set(TBB_LIBRARIES "${TBB_LIBRARY}" "${TBBMALLOC_LIBRARY}")

  if(NOT TARGET TBB::tbb)
    add_library(TBB::tbb UNKNOWN IMPORTED)
    set_target_properties(TBB::tbb PROPERTIES
      IMPORTED_LOCATION "${TBB_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIR}"
    )
  endif()

  if(NOT TARGET TBB::tbbmalloc)
    add_library(TBB::tbbmalloc UNKNOWN IMPORTED)
    set_target_properties(TBB::tbbmalloc PROPERTIES
      IMPORTED_LOCATION "${TBBMALLOC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(
  TBB_INCLUDE_DIR
  TBB_LIBRARY
  TBBMALLOC_LIBRARY
)
