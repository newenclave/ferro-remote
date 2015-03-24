#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"

#include "../utils.h"

namespace fr { namespace lua { namespace m { namespace fs {

namespace {

    using namespace objects;
    namespace fsiface = fr::client::interfaces::filesystem;
    namespace fiface  = fr::client::interfaces::file;

    typedef std::shared_ptr<fsiface::directory_iterator_impl> iterator_sptr;
    typedef std::shared_ptr<fiface::iface>                    file_sptr;

    const std::string     module_name("fs");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".fs.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_pwd   ( lua_State *L );
    int lcall_cd    ( lua_State *L );
    int lcall_exists( lua_State *L );
    int lcall_mkdir ( lua_State *L );
    int lcall_del   ( lua_State *L );
    int lcall_rename( lua_State *L );

    int lcall_read  ( lua_State *L );
    int lcall_write ( lua_State *L );

    int lcall_info  ( lua_State *L );
    int lcall_stat  ( lua_State *L );

    int lcall_close  ( lua_State *L );

    int lcall_fs_iter_begin ( lua_State *L );
    int lcall_fs_iter_next  ( lua_State *L );
    int lcall_fs_iter_get   ( lua_State *L );
    int lcall_fs_iter_clone ( lua_State *L );
    int lcall_fs_iter_end   ( lua_State *L );

    struct module: public iface {

        client::general_info                &info_;
        std::unique_ptr<fsiface::iface>      iface_;

        std::map<size_t, iterator_sptr>      iterators_;
        std::map<size_t, file_sptr>          files_;

        module( client::general_info &info )
            :info_(info)
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        size_t next_handle(  )
        {
            return info_.eventor_->next_index( );
        }

        void close( size_t id )
        {
            files_.erase( id );
            iterators_.erase( id );
        }

        iterator_sptr get_fs_iter( size_t id )
        {
            static const iterator_sptr empty;
            auto f(iterators_.find( id ));
            return (f != iterators_.end( )) ? f->second : empty;
        }

        utils::handle new_fs_iter( const std::string &path )
        {
            size_t nh = next_handle( );
            iterator_sptr ni( iface_->begin_iterate( path ) );
            iterators_[nh] = ni;
            return utils::to_handle( nh );
        }

        file_sptr get_file_iter( size_t id )
        {
            static const file_sptr empty;
            auto f(files_.find( id ));
            return (f != files_.end( )) ? f->second : empty;
        }

        utils::handle new_file_iter( const std::string &path,
                                     const std::string &mode,
                                     bool device = false )
        {
            size_t nh = next_handle( );
            file_sptr nf(fiface::create( *info_.client_core_,
                                         path, mode, device ) );
            files_[nh] = nf;
            return utils::to_handle( nh );
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
            res->add( "close",      new_function( &lcall_close ) );

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

#define LUA_CALL_TRUE_FALSE_PROLOGUE        \
    module *m = get_module( L );            \
    lua::state ls(L);                       \
    try {

#define LUA_CALL_TRUE_FALSE_EPILOGUE        \
        ls.push( true );                    \
    } catch( const std::exception &ex ) {   \
        ls.push( false );                   \
        ls.push( ex.what( ) );              \
        return 2;                           \
    }                                       \
    return 1


    int lcall_cd( lua_State *L )
    {
        LUA_CALL_TRUE_FALSE_PROLOGUE
            std::string path( ls.get_opt<std::string>( 1 ) );
            m->iface_->cd( path );
        LUA_CALL_TRUE_FALSE_EPILOGUE;
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
        LUA_CALL_TRUE_FALSE_PROLOGUE
            std::string path( ls.get_opt<std::string>( 1 ) );
            m->iface_->mkdir( path );
        LUA_CALL_TRUE_FALSE_EPILOGUE;
    }

    int lcall_del( lua_State *L )
    {
        LUA_CALL_TRUE_FALSE_PROLOGUE
            std::string path( ls.get_opt<std::string>( 1 ) );
            m->iface_->del( path );
        LUA_CALL_TRUE_FALSE_EPILOGUE;
    }

    int lcall_rename( lua_State *L )
    {
        LUA_CALL_TRUE_FALSE_PROLOGUE
            std::string from( ls.get_opt<std::string>( 1 ) );
            std::string to( ls.get_opt<std::string>( 2 ) );
            m->iface_->rename( from, to );
        LUA_CALL_TRUE_FALSE_EPILOGUE;
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

    int lcall_close( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        size_t id( utils::from_handle<size_t>(ls.get_opt<void *>( 1 )) );
        m->close( id );
        ls.push( true );
        return 1;
    }

    int lcall_fs_iter_begin ( lua_State *L )
    {

    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

