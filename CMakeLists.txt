
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

set( INTERNAL_PROJECT_NAME ferro_remote )

set( VTRC_IMPORT_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/../vtrc/vtrc-import.cmake")
set( FR_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

if( VTRC_IMPORT )
    set( VTRC_IMPORT_CMAKE ${VTRC_IMPORT} )
endif( )

message( "VTRC import file path: " ${VTRC_IMPORT_CMAKE} )

include( ${VTRC_IMPORT_CMAKE} )
include( cxx11_check.cmake )

message( "static link: " ${VTRC_LINK_TYPE_STATIC} )

project( ${INTERNAL_PROJECT_NAME} )

if( ${VTRC_LINK_TYPE} MATCHES ${VTRC_LINK_TYPE_STATIC} )
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_STATIC_RUNTIME  ON)
endif( )

set(Boost_USE_MULTITHREADED   ON)

if( VTRC_CXX11_ENABLED )

    find_package( Boost 1.50 COMPONENTS
                    system
                    thread
                    program_options
                    filesystem
                  REQUIRED)

    get_cmd_cxx11( VTRC_CXX_CMD )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTRC_CXX_CMD}")

else( )
    find_package( Boost 1.53 COMPONENTS
                    system
                    chrono
                    thread
                    atomic
                    random
                    program_options
                    filesystem
                  REQUIRED)
endif( )

find_package( Protobuf REQUIRED)

include_directories( ${VTRC_INCLUDE_DIRS} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${FR_INCLUDE_DIRS} )

set( src_dirs )
set( src )

set( LUA_FOUND 0 )

if( LUA_SRC )

    add_subdirectory( lua-build )

#    include_directories( ${LUA_INCLUDE_DIR} )
#    target_link_libraries( ${PROJECT_NAME} ${LUA_LIBRARIES} )
#    add_dependencies( ${PROJECT_NAME} lua_lib )

    set( LUA_FOUND 1 )

else( )

    find_package( Lua51 )

    if( ${LUA51_FOUND} )

        set( LUA_FOUND 1 )
    else( )
        find_package( Lua50 )
        if( ${LUA50_FOUND} )
            set( LUA_FOUND 1 )
        endif( )

    endif( )

endif( )

configure_file ("${PROJECT_SOURCE_DIR}/ferro-remote-config.h.in"
                "${FR_INCLUDE_DIRS}/ferro-remote-config.h" )

add_subdirectory( protocol )
add_subdirectory( client-core )

add_subdirectory( console-client )

if( NOT WIN32 )
    add_subdirectory( agent )
endif( )

