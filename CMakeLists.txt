
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

check_cxx11( FR_CXX11_ENABLED )
if( NOT FR_CXX11_ENABLED )
    message( FATAL_ERROR "Compiler has no C++11 support" )
endif( )

message( "static link: " ${VTRC_LINK_TYPE_STATIC} )

project( ${INTERNAL_PROJECT_NAME} )

#################### BOOST ###########################
if( ${VTRC_LINK_TYPE} MATCHES ${VTRC_LINK_TYPE_STATIC} )
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_STATIC_RUNTIME  ON)
endif( )

set(Boost_USE_MULTITHREADED   ON)

if(MSVC)

    find_package( Boost 1.50 COMPONENTS
                    system
                    thread
                    program_options
                    filesystem
                    date_time
                    chrono
                    regex
                  REQUIRED)
else( MSVC )

    find_package( Boost 1.50 COMPONENTS
                    system
                    thread
                    program_options
                    filesystem
                    regex
                    #context
                  REQUIRED)
endif( MSVC )
###############################################

find_package(Fuse)

get_cmd_cxx11( VTRC_CXX_CMD )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTRC_CXX_CMD}")

find_package( Protobuf REQUIRED)

include_directories( ${VTRC_INCLUDE_DIRS} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${FR_INCLUDE_DIRS} )

set( src_dirs )
set( src )

set( LUA_FOUND 0 )

if( DISABLE_LUA )
    message( "DISABLE_LUA " ${DISABLE_LUA} )
else( )
    set( DISABLE_LUA 0 )
endif( )

if( NOT DISABLE_LUA )

    if( LUA_SRC )

        add_subdirectory( lua-build )
        set( LUA_FOUND 1 )

    else( )

        find_file( LUA_SM_PATH
                    NAMES "lua.h"
                    PATHS "${CMAKE_CURRENT_SOURCE_DIR}lua/src/" )
        if( LUA_SM_PATH )
            set( LUA_FOUND 1 )
            set( LUA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lua/src" )
            add_subdirectory( lua-build )
            message( "Lua SRC: "  ${LUA_SRC} )
        endif( LUA_SM_PATH )

    endif( )
endif( )

if(MSVC)
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string( REGEX REPLACE "/W[0-4]" "/W4"
                CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else( )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif( )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
    # Update if necessary

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

elseif(  CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )

    add_definitions( -DNULL=0UL )
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

endif( )

if( STATIC_GLIBC )
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -static-libgcc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    ADD_DEFINITIONS(-static-libgcc)
    SET( CMAKE_CXX_LINK_EXECUTABLE
         "${CMAKE_CXX_LINK_EXECUTABLE} -static-libgcc -static-libstdc++")
endif( STATIC_GLIBC )

configure_file ("${PROJECT_SOURCE_DIR}/ferro-remote-config.h.in"
                "${FR_INCLUDE_DIRS}/ferro-remote-config.h" )

add_subdirectory( protocol )
add_subdirectory( client-core )
add_subdirectory( hub-core )


###############################################
## make some stuff for gui client
###############################################

# LIBS +=
set( GUI_LIBRARIES ${${INTERNAL_PROJECT_NAME}_client_core_LIBRARIES}
                   ${${INTERNAL_PROJECT_NAME}_protocol_LIBRARIES}
                   ${VTRC_CLIENT_LIBRARIES}
                   ${VTRC_COMMON_LIBRARIES}
                   ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} )

set( GUI_LIBS_RES "" )

foreach( gui_lib ${GUI_LIBRARIES} ) # Qt :(
    set( GUI_LIBS_RES "${GUI_LIBS_RES} \\\n\t${gui_lib}" )
endforeach( )

set( GUI_LIBRARIES ${GUI_LIBS_RES} )

# INCLUDEPATH +=

set( GUI_INCLUDE_RES "" )

foreach( gui_inc ${VTRC_INCLUDE_DIRS}
                 ${Boost_INCLUDE_DIRS}
                 ${PROTOBUF_INCLUDE_DIRS}
                 ${FR_INCLUDE_DIRS})
    set( GUI_INCLUDE_RES "${GUI_INCLUDE_RES} \\\n\t${gui_inc}" )
endforeach( )

set( GUI_INCLUDES ${GUI_INCLUDE_RES} )

configure_file ("${PROJECT_SOURCE_DIR}/gui-client/ferro-remote.pri.in"
                "${PROJECT_SOURCE_DIR}/gui-client/ferro-remote-templ.pri" )

###############################################

# add_subdirectory( console-client )

if(LUA_FOUND)
    add_subdirectory( lua-client )
endif(LUA_FOUND)

if(FUSE_FOUND)
    add_subdirectory( fuse-client )
endif(FUSE_FOUND)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
    add_subdirectory( agent )
endif( )

