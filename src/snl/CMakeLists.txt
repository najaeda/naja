add_subdirectory(kernel)
add_subdirectory(flattener)
add_subdirectory(formats)
add_subdirectory(snippet)

if(Python3_Development_FOUND)
  add_subdirectory(python)
endif(Python3_Development_FOUND)
