
list( APPEND src_dirs
        .
        ./modules
    )

foreach( dir ${src_dirs} )
    aux_source_directory( ${dir} src_dir )
endforeach(dir)

set( exe_name lua_client )

add_executable(${exe_name} ${src_dir})

list( APPEND internal
    ${INTERNAL_PROJECT_NAME}_client_core
    ${INTERNAL_PROJECT_NAME}_protocol
    VTRC_CLIENT
    VTRC_COMMON
    )

include_directories( ${${INTERNAL_PROJECT_NAME}_protocol_INCLUDE_DIRS}  )
include_directories( ${${INTERNAL_PROJECT_NAME}_client_core_INCLUDE_DIRS}  )

message( "Core " ${${INTERNAL_PROJECT_NAME}_client_core_INCLUDE_DIRS} )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)

include_directories( ${LUA_INCLUDE_DIR} )
target_link_libraries( ${exe_name} ${LUA_LIBRARIES} )
add_dependencies( ${exe_name} lua_lib )

list(APPEND dependencies
    Boost
    PROTOBUF
    )

message( "-- Seting up dependencies for " ${exe_name} )
foreach( dep ${dependencies} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)


if(WIN32)
    target_link_libraries(${exe_name} ws2_32.lib)
endif(WIN32)

set_target_properties(${exe_name}
                      PROPERTIES COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")

