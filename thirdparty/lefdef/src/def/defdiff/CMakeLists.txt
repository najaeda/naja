# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/defdiff>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/def/def
                             )

                         set ( cpps     diffDefRW.cpp
                                        differDef.cpp
                             )

              add_executable ( defdiff  ${cpps} )
       target_link_libraries ( defdiff  def )
      
                     install ( TARGETS  defdiff DESTINATION bin )
