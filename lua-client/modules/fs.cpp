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
    int lcall_exists( lua_State *L );
    int lcall_mkdir( lua_State *L );
    int lcall_del( lua_State *L );
    int lcall_rename( lua_State *L );

    int lcall_read( lua_State *L );
    int lcall_write( lua_State *L );

    int lcall_info( lua_State *L );
    int lcall_stat( lua_State *L );

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

            res->add( "pwd",        new_function( &lcall_pwd ) );
            res->add( "cd",         new_function( &lcall_cd ) );
            res->add( "exists",     new_function( &lcall_exists ) );
            res->add( "mkdir",      new_function( &lcall_mkdir ) );
            res->add( "del",        new_function( &lcall_del ) );
            res->add( "rename",     new_function( &lcall_rename ) );
            res->add( "info",       new_function( &lcall_info ) );
            res->add( "stat",       new_function( &lcall_stat ) );

            res->add( "read",       new_function( &lcall_read ) );
            res->add( "write",      new_function( &lcall_write ) );

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
            ls.push( ex.what( ) );
            return 1;
        }
        return 0;
    }

    int lcall_exists( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {
            ls.push( m->iface_->exists( path ) );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_mkdir( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {
            m->iface_->mkdir( path );
        } catch( const std::exception &ex ) {
            ls.push( ex.what( ) );
            return 1;
        }
        return 0;
    }

    int lcall_del( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {
            m->iface_->del( path );
        } catch( const std::exception &ex ) {
            ls.push( ex.what( ) );
            return 1;
        }
        return 0;
    }

    int lcall_rename( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string from( ls.get_opt<std::string>( 1 ) );
        std::string to( ls.get_opt<std::string>( 2 ) );
        try {
            m->iface_->rename( from, to );
        } catch( const std::exception &ex ) {
            ls.push( ex.what( ) );
            return 1;
        }
        return 0;
    }


    int lcall_read( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        std::string from( ls.get_opt<std::string>( 1 ) );
        unsigned    max(  ls.get_opt<unsigned>( 2, 44000 ) );

        if( max > 44000 ) {
            max = 44000;
        }
        try {
            std::vector<char> data( max + 1 );
            size_t res = m->iface_->read_file( from, &data[0], max );
            ls.push( &data[0], res );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_write( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        std::string from( ls.get_opt<std::string>( 1 ) );
        std::string data( ls.get_opt<std::string>( 2 ) );

        try {
            size_t res = m->iface_->write_file( from,
                                                data.c_str( ), data.size( ) );
            ls.push( res );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

#define ADD_TABLE_STAT_FIELD( sd, name ) \
    #name, objects::new_integer( sd.name )

    int lcall_info( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {

            fsiface::info_data sd;
            m->iface_->info( path, sd );

            table_sptr t(new_table( ));
            t->add( ADD_TABLE_STAT_FIELD( sd, is_exist      ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, is_directory  ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, is_empty      ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, is_regular    ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, is_symlink    ) );

            t->push( L );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;

    }

    int lcall_stat( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        std::string path( ls.get_opt<std::string>( 1 ) );
        try {
            fsiface::stat_data sd;
            m->iface_->stat( path, sd );

            table_sptr t(new_table( ));
            t->add( ADD_TABLE_STAT_FIELD( sd, dev     ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, ino     ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, mode    ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, nlink   ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, uid     ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, gid     ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, rdev    ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, size    ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, blksize ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, blocks  ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, atime   ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, mtime   ) );
            t->add( ADD_TABLE_STAT_FIELD( sd, ctime   ) );

            t->push( L );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

#undef ADD_TABLE_STAT_FIELD

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

