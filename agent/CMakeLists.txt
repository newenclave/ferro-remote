
list( APPEND src_dirs
        .
        ./poll-reactor
    )

foreach( dir ${src_dirs} )
    aux_source_directory( ${dir} src_dir )
endforeach(dir)

set( exe_name ${INTERNAL_PROJECT_NAME}_agent )

add_executable(${exe_name} ${src_dir})

list( APPEND internal
    ${INTERNAL_PROJECT_NAME}_protocol
    VTRC_SERVER
    VTRC_COMMON
    )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${${INTERNAL_PROJECT_NAME}_protocol_INCLUDE_DIRS}  )

if( LUA_FOUND )
    include_directories( ${LUA_INCLUDE_DIR} )
    target_link_libraries( ${exe_name} ${LUA_LIBRARIES} )
    add_dependencies( ${exe_name} lua_lib )
endif( )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)

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

set_target_properties(${exe_name}
                      PROPERTIES COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")

