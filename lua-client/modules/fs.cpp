#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "interfaces/IFilesystem.h"

namespace fr { namespace lua { namespace m { namespace fs {

namespace {

    using namespace objects;
    namespace fsiface = fr::client::interfaces::filesystem;

    const std::string     module_name("fs");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".fs.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_pwd( lua_State *L );
    int lcall_cd( lua_State *L );

    struct module: public iface {

        client::general_info            &info_;
        std::unique_ptr<fsiface::iface>  iface_;

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
            iface_.reset( fsiface::create( *info_.client_core_, "" ) );
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

            res->add( "pwd", new_function( &lcall_pwd ) );
            res->add( "cd",  new_function( &lcall_cd ) );

            return res;
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_pwd( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        ls.push( m->iface_->pwd( ) );
        return 1;
    }

    int lcall_cd( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {
            m->iface_->cd( path );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
        }
        return 0;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

