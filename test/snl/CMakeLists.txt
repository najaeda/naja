
add_subdirectory(kernel)
add_subdirectory(netlists)
add_subdirectory(flattener)
add_subdirectory(formats)

if(Python3_Development_FOUND)
  #add_subdirectory(python)
endif(Python3_Development_FOUND)
