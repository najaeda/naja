if(DOXYGEN_FOUND)
  # set input and output files
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
  set(DOXYGEN_OUT ${DOXYGEN_OUTPUT_DIR}/Doxyfile.out)
  set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)


  # request to configure the file
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message("Doxygen build started")

  add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   MAIN_DEPENDENCY ${DOXYGEN_OUT}
                   COMMENT "Generating API documentation with Doxygen")

  # Note: do not put "ALL" - this builds docs together with application EVERY TIME!
  add_custom_target(docs DEPENDS ${DOXYGEN_INDEX_FILE})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc OPTIONAL) 
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

if(BUILD_SPHINX_DOCUMENTATION)
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
  COMMAND 
  ${SPHINX_EXECUTABLE} -b html
  # Tell Breathe where to find the Doxygen output
  -Dbreathe_projects.naja=${DOXYGEN_OUTPUT_DIR}/xml
  ${SPHINX_SOURCE} ${SPHINX_BUILD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
  # Other docs files you want to track should go here (or in some variable)
  ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
  ${DOXYGEN_INDEX_FILE}
  MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
  COMMENT "Generating documentation with Sphinx")

add_custom_target(sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

endif(BUILD_SPHINX_DOCUMENTATION)