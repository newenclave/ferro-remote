#ifndef LUA_INTERFACE_H
#define LUA_INTERFACE_H

#include "ferro-remote-config.h"

#if FR_WITH_LUA

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

namespace fr { namespace client { namespace core {
    class client_core;
}}}

namespace fr { namespace lua {

    namespace names {
        static const char * main_table    = "fr" ;
        static const char * client_table  = "client";
        static const char * core_field    = "core";
    }

    inline
    static client::core::client_core *get_core( lua_State *L )
    {
        lua::state lv( L );
        void *p =
                lv.get_from_global<void *>( names::main_table,
                                            names::core_field );
        return reinterpret_cast<client::core::client_core *>( p );
    }

    inline
    static void set_core( lua_State *L, client::core::client_core *cc )
    {
        lua::state lv( L );
        lv.set_in_global( names::main_table, names::core_field,
                          reinterpret_cast<void *>( cc ) );
    }

    struct base_data {
        virtual ~base_data( ) { }
        virtual lua::objects::table_sptr get_table( )
        {
            return lua::objects::table_sptr( );
        }
    };

    typedef std::shared_ptr<base_data> data_sptr;

    namespace os {
        static const char *table_name = "os";
        data_sptr init( lua_State *ls, client::core::client_core &cc );
    }
}}


#endif

#endif // LUAINTERFACE_H
