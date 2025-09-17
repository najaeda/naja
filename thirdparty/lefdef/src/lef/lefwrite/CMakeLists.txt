# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/lefwrite>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/lef/lef
                             )

                         set ( cpps     lefwrite.cpp
                             )

              add_executable ( lefwrite ${cpps} )
       target_link_libraries ( lefwrite lef )
      
                     install ( TARGETS  lefwrite  DESTINATION bin )
