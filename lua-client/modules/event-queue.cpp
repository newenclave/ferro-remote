#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

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

            return res;
        }

        bool connection_required( ) const
        {
            return false;
        }
    };

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


