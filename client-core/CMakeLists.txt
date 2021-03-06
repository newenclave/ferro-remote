

set( lib_postfix client_core )
set( lib_dirname client-core )


set( lib_name ${INTERNAL_PROJECT_NAME}_${lib_postfix} )

list( APPEND src_dirs
        .
        ./impl
        ${FR_INCLUDE_DIRS}/${lib_dirname}
        ${FR_INCLUDE_DIRS}/${lib_dirname}/interfaces
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(   ${src_dir} lib_src)
    file(GLOB   headers     ${src_dir}/*.h)
    list(APPEND lib_src     ${headers})
endforeach( )

add_library( ${lib_name} STATIC ${lib_src} )

set( CORE_INCLUDE_DIRS "${FR_INCLUDE_DIRS}/${lib_dirname}" )

list( APPEND internal
    ${INTERNAL_PROJECT_NAME}_protocol
    VTRC_CLIENT
    VTRC_COMMON
    )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${${INTERNAL_PROJECT_NAME}_protocol_INCLUDE_DIRS}  )
include_directories( ${CORE_INCLUDE_DIRS} )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${lib_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    #target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )

endforeach(dep)

#if( LUA_FOUND )
#    include_directories( ${LUA_INCLUDE_DIR} )
#    target_link_libraries( ${lib_name} ${LUA_LIBRARIES} )
#    #add_dependencies( ${lib_name} lua_lib )
#endif( )

list(APPEND dependencies
    Boost
    PROTOBUF
    )

message( "-- Seting up dependencies for " ${lib_name} )
foreach( dep ${dependencies} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${lib_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    #target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )

endforeach(dep)

set_target_properties(${lib_name}
                      PROPERTIES
                        COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")

get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-D${VTRC_LINK_TYPE}" PARENT_SCOPE)

set(${lib_name}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${${lib_name}_PROTOBUF_FILES_DIR}
    ${${lib_name}_PROTOBUF_FILES_OUTPUT}
    ${CONFIG_FILES_DIRECTORY}
    ${CORE_INCLUDE_DIRS}
    PARENT_SCOPE
)

set(${lib_name}_LIBRARIES ${${lib_name}_LOCATION} PARENT_SCOPE)
