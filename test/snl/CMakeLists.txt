add_subdirectory(netlists)
add_subdirectory(formats)
add_subdirectory(snl)

if(PYTHON_INTERFACE AND Python3_Development_FOUND)
  add_subdirectory(python)
endif(PYTHON_INTERFACE AND Python3_Development_FOUND)