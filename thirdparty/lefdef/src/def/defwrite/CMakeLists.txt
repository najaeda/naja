# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/defwrite>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/def/def
                             )

                         set ( cpps     defwrite.cpp
                             )

              add_executable ( defwrite ${cpps} )
       target_link_libraries ( defwrite def )
      
                     install ( TARGETS  defwrite  DESTINATION bin )
