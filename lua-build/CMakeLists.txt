cmake_minimum_required( VERSION 2.8 )

set( lib_name lua_lib )

project( ${lib_name} )

list( APPEND lib_dirs
        .
        ${LUA_SRC}
    )

foreach( src_dir ${lib_dirs} )
    aux_source_directory(   ${src_dir} lib_src)
    file(GLOB   headers     ${src_dir}/*.h)
    list(APPEND lib_src     ${headers})
endforeach( )

add_library( ${lib_name} STATIC ${lib_src} )

set( LUA_INCLUDE_DIR
     ${LUA_SRC}
     PARENT_SCOPE
)

get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)
set( LUA_LIBRARIES ${${lib_name}_LOCATION} PARENT_SCOPE)

