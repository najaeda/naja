# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/defrw>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/def/def
                             )

                         set ( cpps     defrw.cpp
                             )

              add_executable ( defrw    ${cpps} )
       target_link_libraries ( defrw    def )
      
                     install ( TARGETS  defrw   DESTINATION bin )
