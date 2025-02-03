# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/lefrw>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/lef/lef
                             )

                         set ( cpps     lefrw.cpp
                             )

              add_executable ( lefrw    ${cpps} )
       target_link_libraries ( lefrw    lef )
      
                     install ( TARGETS  lefrw   DESTINATION bin )
