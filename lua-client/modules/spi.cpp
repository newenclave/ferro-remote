#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/ISPI.h"

namespace fr { namespace lua { namespace m { namespace spi {

namespace {

    using namespace objects;
    namespace ispi = fr::client::interfaces::spi;

    const std::string     module_name("spi");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".spi.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_os_exec( lua_State *L );

    struct module: public iface {

        client::general_info            &info_;
        std::unique_ptr<ispi::iface>     iface_;

        module( client::general_info &info )
            :info_(info)
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void reinit( )
        {
            iface_.reset( ispi::create( *info_.client_core_ ) );
        }

        void deinit( )
        {
            iface_.reset( );
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
    };

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


