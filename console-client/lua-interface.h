#ifndef LUA_INTERFACE_H
#define LUA_INTERFACE_H

#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include <map>
#include <string>

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

namespace fr { namespace client { namespace core {
    class client_core;
}}}

namespace fr { namespace lua {

    namespace names {
        extern const char * main_table;
        extern const char * client_table;
        extern const char * connect_table;
        extern const char * disconnect_table;
        extern const char * core_path;
        extern const char * server_path;
        extern const char * inst_field;
    }

    struct core_data;

    inline
    static core_data *get_core( lua_State *L )
    {
        lua::state lv( L );
        void *p = lv.get<void *>( names::core_path );
        return reinterpret_cast<core_data *>( p );
    }

    inline
    static void set_core( lua_State *L, core_data *cc )
    {
        lua::state lv( L );
        lv.set( names::core_path, reinterpret_cast<void *>( cc ) );
    }

    inline
    static void *get_component_iface( lua_State *L, const char *table_path )
    {
        lua::state lv( L );

        int level = lv.get_table( table_path );
        void *ptr = NULL;

        if( level ) {
            ptr = lv.get_field<void *>( names::inst_field );
            lv.pop( level );
        }
        return ptr;
    }

    struct base_data {
        virtual ~base_data( ) { }
        virtual lua::objects::table_sptr init_table( )
        {
            return lua::objects::table_sptr( );
        }
    };

    typedef std::shared_ptr<base_data> data_sptr;

    struct core_data {

        typedef std::map<std::string, data_sptr> table_map;

        client::core::client_core  *core_;
        table_map                   tables_;
        std::shared_ptr<lua::state> state_;
    };


#define FR_DEFINE_NAMESPACE_FOR_LUA( ns )                                      \
    namespace ns {                                                             \
        const char *table_name( );                                             \
        const char *table_path( );                                             \
        data_sptr init( std::shared_ptr<lua::state> &ls,                       \
                        client::core::client_core &cc );                       \
    }

    FR_DEFINE_NAMESPACE_FOR_LUA( os )
    FR_DEFINE_NAMESPACE_FOR_LUA( fs )
    FR_DEFINE_NAMESPACE_FOR_LUA( gpio )
    FR_DEFINE_NAMESPACE_FOR_LUA( i2c )

#undef FR_DEFINE_NAMESPACE_FOR_LUA

}}


#endif

#endif // LUAINTERFACE_H
