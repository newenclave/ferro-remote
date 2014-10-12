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
        static const char * client_table  = "fr.client";
        static const char * core_path     = "fr.client.core";
        static const char * inst_field    = "__i";
    }

    inline
    static client::core::client_core *get_core( lua_State *L )
    {
        lua::state lv( L );
        void *p = lv.get<void *>( names::core_path );
        return reinterpret_cast<client::core::client_core *>( p );
    }

    inline
    static void set_core( lua_State *L, client::core::client_core *cc )
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

#define FR_DEFINE_NAMESPACE_FOR_LUA( ns )                               \
    namespace ns {                                                      \
        static const char *table_name = #ns;                            \
        data_sptr init( lua_State *ls, client::core::client_core &cc ); \
    }

    FR_DEFINE_NAMESPACE_FOR_LUA( os )
    FR_DEFINE_NAMESPACE_FOR_LUA( fs )
    FR_DEFINE_NAMESPACE_FOR_LUA( gpio )

#undef FR_DEFINE_NAMESPACE_FOR_LUA

}}


#endif

#endif // LUAINTERFACE_H
