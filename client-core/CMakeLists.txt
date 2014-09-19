
set( lib_name ${INTERNAL_PROJECT_NAME}_client_core )

list( APPEND src_dirs
        .
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(   ${src_dir} lib_src)
    file(GLOB   headers     ${src_dir}/*.h)
    list(APPEND lib_src     ${headers})
endforeach( )

add_library( ${lib_name} STATIC ${lib_src} )

list( APPEND internal
    ${INTERNAL_PROJECT_NAME}_protocol
    VTRC_CLIENT
    VTRC_COMMON
    )

include_directories( ${${INTERNAL_PROJECT_NAME}_protocol_INCLUDE_DIRS}  )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${lib_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )

endforeach(dep)

