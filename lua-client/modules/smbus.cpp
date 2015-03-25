#include <map>

#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/II2C.h"

#include "../utils.h"

namespace fr { namespace lua { namespace m { namespace smbus {

namespace {

    using namespace objects;
    namespace iiface = fr::client::interfaces::i2c;

    typedef std::shared_ptr<iiface::iface> smbus_sptr;
    typedef std::map<size_t, smbus_sptr>   dev_map;

    static const unsigned slave_invalid = iiface::I2C_SLAVE_INVALID_ADDRESS;

    const std::string     module_name("smbus");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".smbus.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_bus_avail( lua_State *L );
    int lcall_bus_open ( lua_State *L );
    int lcall_bus_close( lua_State *L );

    struct module: public iface {

        client::general_info  &info_;
        dev_map                buses_;

        module( client::general_info &info )
            :info_(info)
        { }

        size_t next_handle(  )
        {
            return info_.eventor_->next_index( );
        }

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
            buses_.clear( );
        }

        utils::handle new_bus( unsigned bus_id,
                               unsigned slave = slave_invalid,
                               bool slave_force = false)
        {
            size_t nh = next_handle( );
            smbus_sptr r(iiface::open( *info_.client_core_,
                                       bus_id, slave, slave_force ));
            buses_[nh] = r;
            return utils::to_handle( nh );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "available", new_function( &lcall_bus_avail ) );

            return res;
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_bus_avail( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        try {
            ls.push( iiface::bus_available(
                         *m->info_.client_core_,
                         ls.get_opt<unsigned>( 1, 0xFFFFFFFF )) );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_open ( lua_State *L )
    {
        return 0;
    }

    int lcall_bus_close( lua_State *L )
    {
        return 0;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

