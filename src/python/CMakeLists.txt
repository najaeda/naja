set(SOURCES
  PySNLUniverse.cpp
  PySNLDB.cpp PySNLLibrary.cpp
  PySNLDesign.cpp
  PySNLScalarTerm.cpp
  PySNLInstance.cpp
  PySNL.cpp
)

Python3_add_library(snl MODULE ${SOURCES})

target_link_libraries(snl PRIVATE naja_snl)
