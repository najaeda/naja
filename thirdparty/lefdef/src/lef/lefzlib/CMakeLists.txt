# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/lefzlib>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/lef/lef
                               ${LEFDEF_SOURCE_DIR}/src/lef/lefzlib
                             )

                         set ( includes lefzlib.hpp
                             )
      
                         set ( cpps     lefzlib.cpp
                             )

                 add_library ( lefzlib  ${cpps} )
       target_link_libraries ( lefzlib  lef z )
       set_target_properties ( lefzlib  PROPERTIES VERSION 5.8 SOVERSION 5 )
      
                     install ( TARGETS  lefzlib DESTINATION lib${LIB_SUFFIX} )
                     install ( FILES    ${includes} DESTINATION include/lefdef ) 
