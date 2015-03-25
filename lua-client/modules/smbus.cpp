#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/II2C.h"

#include <map>

namespace fr { namespace lua { namespace m { namespace smbus {

namespace {

    using namespace objects;
    namespace iiface = fr::client::interfaces::i2c;

    typedef std::shared_ptr<iiface::iface> smbus_sptr;
    typedef std::map<size_t, smbus_sptr>   dev_map;

    const std::string     module_name("smbus");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".smbus.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    struct module: public iface {

        client::general_info  &info_;
        dev_map                buses_;

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

        }

        void deinit( )
        {

        }

//        iface_sptr new_bus( )
//        {

//        }

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
            return true;
        }
    };

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

