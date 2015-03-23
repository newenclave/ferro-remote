#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/IOS.h"

namespace fr { namespace lua { namespace m { namespace os {

namespace {

    using namespace objects;
    namespace ifos = fr::client::interfaces::os;

    const std::string     module_name("os");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".os.__i";

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
        std::unique_ptr<ifos::iface>     iface_;

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
            iface_.reset( ifos::create( *info_.client_core_ ) );
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

            res->add( "execute", new_function( &lcall_os_exec ) );
            res->add( "system",  new_function( &lcall_os_exec ) );

            return res;
        }
    };

    int lcall_os_exec( lua_State *L )
    {
        lua::state ls( L );
        module *m = get_module( L );
        std::string command( ls.get_opt<std::string>(1) );
        if( !command.empty( ) ) {
            try {
                ls.push( m->iface_->execute( command ) );
                return 1;
            } catch( const std::exception &ex ) {
                ls.push( );
                ls.push( ex.what( ) );
                return 2;
            }
        }
        return 0;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

