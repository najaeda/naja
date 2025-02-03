# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/lefdiff>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/lef/lef
                             )

                         set ( cpps     diffLefRW.cpp
                                        differLef.cpp
                             )

              add_executable ( lefdiff  ${cpps} )
       target_link_libraries ( lefdiff  lef )
      
                     install ( TARGETS  lefdiff DESTINATION bin )
