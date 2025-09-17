# -*- mode: CMAKE explicit-buffer-name: "CMakeLists.txt<lefdef/src/defzlib>" -*-

         include_directories ( ${LEFDEF_SOURCE_DIR}/src/def/def
                               ${LEFDEF_SOURCE_DIR}/src/def/defzlib
                             )

                         set ( includes defzlib.hpp
                             )
      
                         set ( cpps     defzlib.cpp
                             )

                 add_library ( defzlib  ${cpps} )
       target_link_libraries ( defzlib  def z )
       set_target_properties ( defzlib  PROPERTIES VERSION 5.8 SOVERSION 5 )
      
                     install ( TARGETS  defzlib DESTINATION lib${LIB_SUFFIX} )
                     install ( FILES    ${includes} DESTINATION include/lefdef ) 
