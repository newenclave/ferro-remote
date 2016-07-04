
list( APPEND src_dirs . )

foreach( dir ${src_dirs} )
    aux_source_directory( ${dir} src_dir )
endforeach(dir)

set( exe_name frfuse_client )

add_executable(${exe_name} ${src_dir})

include( ../cxx11_check.cmake )
get_compiler_thread_local( THREAD_LOCAL_VALUE )
add_definitions( -DTHREAD_LOCAL=${THREAD_LOCAL_VALUE} )

####################### FUSE ############################
find_package(Fuse REQUIRED)

add_definitions(${FUSE_DEFINITIONS})
add_definitions( -DFUSE_USE_VERSION=22 )

message( "FUSE_VERSION: " ${FUSE_VERSION})

include_directories(${FUSE_INCLUDE_DIRS})
target_link_libraries(${exe_name} ${FUSE_LIBRARIES})
target_link_libraries(${exe_name} dl)

#########################################################

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

#include_directories( ${LUA_INCLUDE_DIR} )
#target_link_libraries( ${exe_name} ${LUA_LIBRARIES} )
#add_dependencies( ${exe_name} lua_lib )

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

#if(MINGW)
#    add_definitions( -DBOOST_THREAD_USE_LIB=1 )
#endif(MINGW)

set_target_properties(${exe_name}
                      PROPERTIES COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")

