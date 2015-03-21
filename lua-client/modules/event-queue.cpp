#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "../utils.h"

namespace fr { namespace lua { namespace m { namespace event_queue {

namespace {

    using namespace objects;

    const std::string     module_name("event_queue");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".event_queue.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_post( lua_State *L );

    struct module: public iface {

        client::general_info &info_;

        module( client::general_info &info )
            :info_(info)
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void deinit( )
        {

        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));
            res->add( "post", new_function( &lcall_post ) );
            return res;
        }

        bool connection_required( ) const
        {
            return false;
        }
    };

    int lcall_post( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        int n = ls.get_top( );

        base_sptr call(new_reference( L, 1 ));

        std::vector<objects::base_sptr> params;

        if( n > 1 ) {
            params.reserve( n - 1 );
            for( int i=2; i<=n; ++i ) {
                params.push_back( base_sptr( new_reference( L, i ) ) );
            }
        }
        m->info_.eventor_->push_call( call, params );
        return 0;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


