add_subdirectory(snl)
add_subdirectory(formats)
add_subdirectory(snippets)

if(Python3_Development_FOUND)
  add_subdirectory(python)
endif(Python3_Development_FOUND)
