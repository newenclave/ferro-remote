
#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/IGPIO.h"

namespace fr { namespace lua { namespace m { namespace gpio {

namespace {

    using namespace objects;
    namespace giface = fr::client::interfaces::gpio;
    typedef   giface::iface siface_type;

    typedef std::shared_ptr<giface::iface> dev_sptr;
    typedef std::map<size_t, dev_sptr>     dev_map;

    const std::string     module_name("gpio");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".gpio.__i";

    struct module;

    int lcall_available( lua_State *L );

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_export   ( lua_State *L );
    int lcall_info     ( lua_State *L );
    int lcall_unexport ( lua_State *L );

    struct module: public iface {

        client::general_info &info_;
        bool                  available_;
        dev_map               devs_;

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
            available_ = giface::available( *info_.client_core_ );
        }

        void deinit( )
        {
            devs_.clear( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "available", new_boolean( available_ ) );

            return res;
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_available( lua_State *L )
    {
        lua::state ls( L );
        module *m = get_module( L );
        ls.push( giface::available( *m->info_.client_core_ ) );
        return 1;
    }

    int lcall_export( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }

        return 0;
    }

    int lcall_info( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }

        return 0;

    }

    int lcall_unexpor( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 0;
    }


}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

