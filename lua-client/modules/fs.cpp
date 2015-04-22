#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "../event-container.h"

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
    typedef std::shared_ptr<lua::event_container>             eventor_sptr;
    typedef std::weak_ptr<lua::event_container>               eventor_wptr;

    const std::string       module_name("fs");
    const char *id_path     = FR_CLIENT_LUA_HIDE_TABLE ".fs.__i";
    const char *file_meta   = FR_CLIENT_LUA_HIDE_TABLE ".fs.file.meta";
    const char *iter_meta   = FR_CLIENT_LUA_HIDE_TABLE ".fs.iterator.meta";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    struct meta_object {
        utils::handle hdl_;
    };

    int lcall_meta_file_string( lua_State *L )
    {
        lua::state ls(L);
        void *ud = luaL_testudata( L, 1, file_meta );
        if( ud ) {
            std::ostringstream oss;
            oss << "file@"
                << std::hex
                << static_cast<meta_object *>(ud)->hdl_;
            ls.push( oss.str( ) );
        } else {
            ls.push( "Unknown object." );
        }
        return 1;
    }

    int lcall_meta_iter_string( lua_State *L )
    {
        lua::state ls(L);
        void *ud = luaL_testudata( L, 1, iter_meta );
        if( ud ) {
            std::ostringstream oss;
            oss << "fs_iterator@"
                << std::hex
                << static_cast<meta_object *>(ud)->hdl_;
            ls.push( oss.str( ) );
        } else {
            ls.push( "Unknown object." );
        }
        return 1;
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

    int lcall_close ( lua_State *L );

    int lcall_fs_iter_begin     ( lua_State *L );
    int lcall_fs_iter_next      ( lua_State *L );
    int lcall_fs_iter_get       ( lua_State *L );
    int lcall_fs_iter_clone     ( lua_State *L );
    int lcall_fs_iter_end       ( lua_State *L );
    int lcall_fs_iter_has_next  ( lua_State *L );

    int lcall_file_open  ( lua_State *L );
    int lcall_file_ioctl ( lua_State *L );
    int lcall_file_seek  ( lua_State *L );
    int lcall_file_tell  ( lua_State *L );
    int lcall_file_flush ( lua_State *L );

    int lcall_file_read  ( lua_State *L );
    int lcall_file_write ( lua_State *L );

    int lcall_file_events   ( lua_State *L );
    int lcall_file_subscribe( lua_State *L );

    const struct luaL_Reg file_lib[ ] = {

         { "ioctl",      &lcall_file_ioctl   }
        ,{ "seek",       &lcall_file_seek    }
        ,{ "tell",       &lcall_file_tell    }
        ,{ "flush",      &lcall_file_flush   }
        ,{ "read",       &lcall_file_read    }
        ,{ "write",      &lcall_file_write   }
        ,{ "events",     &lcall_file_events  }
        ,{ "subscribe",  &lcall_file_subscribe }
        ,{ "close",      &lcall_close        }
        ,{ "__gc",       &lcall_close        }
        ,{ "__tostring", &lcall_meta_file_string }

        ,{ nullptr,      nullptr }
    };

    const struct luaL_Reg iter_lib[ ] = {

         { "end",        &lcall_fs_iter_end      }
        ,{ "has_next",   &lcall_fs_iter_has_next }
        ,{ "next",       &lcall_fs_iter_next     }
        ,{ "get",        &lcall_fs_iter_get      }
        ,{ "clone",      &lcall_fs_iter_clone    }
        ,{ "close",      &lcall_close            }
        ,{ "__gc",       &lcall_close            }
        ,{ "__tostring", &lcall_meta_iter_string }

        ,{ nullptr,      nullptr }
    };

    std::vector<std::string> file_events( )
    {
        std::vector<std::string> res;

        res.push_back( "on_pollin" );

        return res;
    }

    int lcall_register_file_meta( lua_State *L )
    {
        metatable mt( file_meta, file_lib );
        mt.push( L );
        return 1;
    }

    int lcall_register_iter_meta( lua_State *L )
    {
        metatable mt( iter_meta, iter_lib );
        mt.push( L );
        return 1;
    }

    void register_meta_tables( lua_State *L )
    {
        lcall_register_file_meta( L );
        lcall_register_iter_meta( L );
    }

    struct module: public iface {

        client::general_info            &info_;
        std::unique_ptr<fsiface::iface>  iface_;

        std::map<size_t, iterator_sptr>  iterators_;
        std::map<size_t, file_sptr>      files_;
        std::map<size_t, eventor_sptr>   file_events_;
        std::vector<std::string>         events_name_;

        module( client::general_info &info )
            :info_(info)
            ,events_name_(file_events( ))
        {
            register_meta_tables( info_.main_ );
        }

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
            if( files_.find( id ) != files_.end( ) ) {
                files_.erase( id );
                file_events_.erase( id );
            } else if( iterators_.find( id ) != iterators_.end( ) ) {
                iterators_.erase( id );
            }
        }

        void file_event( unsigned err, const std::string &data,
                         eventor_wptr evtr )
        {
            eventor_sptr le(evtr.lock( ));
            if( le ) {
                FR_LUA_EVENT_PROLOGUE( "on_pollin", *le );
                if( err ) {
                    result->add( "error", new_integer( err ) );
                } else {
                    result->add( "data", new_string( data ) );
                }
                FR_LUA_EVENT_EPILOGUE;
            }
        }

        void register_file( file_sptr f, eventor_sptr e )
        {
            namespace ph = std::placeholders;
            f->register_for_events( std::bind( &module::file_event, this,
                                    ph::_1, ph::_2, eventor_wptr(e) ) );
        }

        void unregister_file( file_sptr f )
        {
            f->unregister( );
        }

        iterator_sptr get_fs_iter( utils::handle id )
        {
            static const iterator_sptr empty;
            auto f(iterators_.find( utils::from_handle<size_t>(id) ));
            return (f != iterators_.end( )) ? f->second : empty;
        }

        utils::handle new_fs_iter( const std::string &path )
        {
            size_t nh = next_handle( );
            iterator_sptr ni( iface_->begin_iterate( path ) );
            iterators_[nh]   = ni;
            return utils::to_handle( nh );
        }

        eventor_sptr new_eventor( )
        {
            eventor_sptr res = std::make_shared<
                                    lua::event_container
                               >( info_, events_name_ );
            return res;
        }

        eventor_sptr get_eventor( utils::handle id )
        {
            static const eventor_sptr empty;
            auto f(file_events_.find( utils::from_handle<size_t>(id) ));
            return (f != file_events_.end( )) ? f->second : empty;
        }

        utils::handle clone_fs_iter( utils::handle hdl )
        {
            auto f(iterators_.find( utils::from_handle<size_t>( hdl ) ));
            if( f != iterators_.end( ) ) {
                size_t ni = next_handle( );
                iterator_sptr nh(f->second->clone( ));
                iterators_[ni] = nh;
                return utils::to_handle( ni );
            } else {
                return nullptr;
            }
        }

        file_sptr get_file_hdl( utils::handle id )
        {
            static const file_sptr empty;
            auto f(files_.find( utils::from_handle<size_t>(id) ));
            return (f != files_.end( )) ? f->second : empty;
        }

        utils::handle new_file( const std::string &path,
                                const std::string &mode,
                                bool device = false )
        {
            size_t nh = next_handle( );
            file_sptr nf(fiface::create( *info_.client_core_,
                                         path, mode, device ) );
            files_[nh] = nf;
            file_events_[nh] = new_eventor( );
            return utils::to_handle( nh );
        }

        meta_object *push_file_object( lua_State *L,
                                       utils::handle hdl, const char *name )
        {
            void *ud = lua_newuserdata( L, sizeof(meta_object) );
            meta_object *nfo = static_cast<meta_object *>(ud);
            if( nfo ) {
                luaL_getmetatable( L, name );
                lua_setmetatable(L, -2);
                nfo->hdl_ = hdl;
            }
            return nfo;
        }

        utils::handle get_meta_object( lua_State *L, int id, const char *name )
        {
            void *ud = luaL_testudata( L, id, name );
            if( ud ) {
                return static_cast<meta_object *>(ud)->hdl_;
            } else {
                return utils::handle( );
            }
        }

        utils::handle get_file_object( lua_State *L, int id = 1 )
        {
            return get_meta_object( L, id, file_meta );
        }

        utils::handle get_iter_object( lua_State *L, int id = 1 )
        {
            return get_meta_object( L, id, iter_meta );
        }

        utils::handle new_file( const std::string &path,
                                unsigned flags, unsigned mode,
                                bool device = false )
        {
            size_t nh = next_handle( );
            file_sptr nf( device
                        ? fiface::create_simple_device( *info_.client_core_,
                                                        path, flags, mode )
                        : fiface::create( *info_.client_core_,
                                           path, flags, mode ) );
            files_[nh] = nf;
            file_events_[nh] = new_eventor( );
            return utils::to_handle( nh );
        }

        void reinit( )
        {
            iface_.reset( fsiface::create( *info_.client_core_, "" ) );
        }

        void deinit( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, (void *)nullptr );

            file_events_.clear( );
            iterators_.clear( );
            files_.clear( );
            iface_.reset( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        objects::table_sptr iter_table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "begin",      new_function( &lcall_fs_iter_begin ) );
            res->add( "end",        new_function( &lcall_fs_iter_end ) );
            res->add( "has_next",   new_function( &lcall_fs_iter_has_next ) );

            res->add( "next",       new_function( &lcall_fs_iter_next ) );
            res->add( "get",        new_function( &lcall_fs_iter_get ) );
            res->add( "clone",      new_function( &lcall_fs_iter_clone ) );

            return res;
        }

        objects::table_sptr file_table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));
            objects::table_sptr f(std::make_shared<objects::table>( ));
            objects::table_sptr m(std::make_shared<objects::table>( ));

#define ADD_FILE_TABLE_FLAG( f ) \
    objects::new_string( #f ), objects::new_integer( fiface::flags::f )

#define ADD_FILE_TABLE_MODE( m ) \
    objects::new_string( #m ), objects::new_integer( fiface::mode::m )

                f->add( ADD_FILE_TABLE_FLAG( RDONLY ) );
                f->add( ADD_FILE_TABLE_FLAG( WRONLY ) );
                f->add( ADD_FILE_TABLE_FLAG( RDWR ) );
                f->add( ADD_FILE_TABLE_FLAG( CREAT ) );
                f->add( ADD_FILE_TABLE_FLAG( EXCL ) );
                f->add( ADD_FILE_TABLE_FLAG( APPEND ) );
                f->add( ADD_FILE_TABLE_FLAG( NONBLOCK ) );
                f->add( ADD_FILE_TABLE_FLAG( ASYNC ) );
                f->add( ADD_FILE_TABLE_FLAG( SYNC ) );
                f->add( ADD_FILE_TABLE_FLAG( TRUNC ) );

                m->add( ADD_FILE_TABLE_MODE( IRWXU ) );
                m->add( ADD_FILE_TABLE_MODE( IRUSR ) );
                m->add( ADD_FILE_TABLE_MODE( IWUSR ) );
                m->add( ADD_FILE_TABLE_MODE( IXUSR ) );
                m->add( ADD_FILE_TABLE_MODE( IRWXG ) );
                m->add( ADD_FILE_TABLE_MODE( IRGRP ) );
                m->add( ADD_FILE_TABLE_MODE( IWGRP ) );
                m->add( ADD_FILE_TABLE_MODE( IXGRP ) );
                m->add( ADD_FILE_TABLE_MODE( IRWXO ) );
                m->add( ADD_FILE_TABLE_MODE( IROTH ) );
                m->add( ADD_FILE_TABLE_MODE( IWOTH ) );
                m->add( ADD_FILE_TABLE_MODE( IXOTH ) );

#undef ADD_FILE_TABLE_MODE
#undef ADD_FILE_TABLE_FLAG

            res->add( "flag", f );
            res->add( "mode", m );
            res->add( "seek_pos", new_table( )
                      ->add( "set", new_integer( fiface::POS_SEEK_SET ) )
                      ->add( "cur", new_integer( fiface::POS_SEEK_CUR ) )
                      ->add( "end", new_integer( fiface::POS_SEEK_END ) ) );

            res->add( "open",   new_function( &lcall_file_open ) );
            res->add( "ioctl",  new_function( &lcall_file_ioctl ) );
            res->add( "flush",  new_function( &lcall_file_flush ) );
            res->add( "tell",   new_function( &lcall_file_tell ) );
            res->add( "seek",   new_function( &lcall_file_seek ) );
            res->add( "read",   new_function( &lcall_file_read ) );
            res->add( "write",  new_function( &lcall_file_write ) );

            res->add( "events",    new_function( &lcall_file_events ) );
            res->add( "subscribe", new_function( &lcall_file_subscribe ) );

            return res;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "pwd",      new_function( &lcall_pwd ) );
            res->add( "cd",       new_function( &lcall_cd ) );
            res->add( "exists",   new_function( &lcall_exists ) );
            res->add( "mkdir",    new_function( &lcall_mkdir ) );
            res->add( "del",      new_function( &lcall_del ) );
            res->add( "rename",   new_function( &lcall_rename ) );
            res->add( "info",     new_function( &lcall_info ) );
            res->add( "stat",     new_function( &lcall_stat ) );
            res->add( "close",    new_function( &lcall_close ) );

            res->add( "read",     new_function( &lcall_read ) );
            res->add( "write",    new_function( &lcall_write ) );

            res->add( "iterator", iter_table( ) );
            res->add( "file",     file_table( ) );

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
        if( !m ) {
            return 0;
        }
        lua::state ls(L);
        int n = ls.get_top( );
        for( int i=1; i<=n; i++ ) {
            const utils::handle fhdl = m->get_file_object( L, i );
            if( fhdl ) {
                m->close( utils::from_handle<size_t>( fhdl ) );
            } else {
                const utils::handle ihdl = m->get_iter_object( L, 1 );
                m->close( utils::from_handle<size_t>( ihdl ) );
            }
        }
        ls.push( true );
        return 1;
    }

    int lcall_fs_iter_begin ( lua_State *L )
    {
        module *m = get_module( L );

        lua::state ls(L);

        try {
            std::string path;
            if( ls.get_top( ) && ls.get_type( 1 ) == base::TYPE_STRING ) {
                path = ls.get<std::string>( 1 );
            }
            m->push_file_object( L, m->new_fs_iter( path ), iter_meta );
        } catch ( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_fs_iter_end( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {
            utils::handle h = m->get_iter_object( L, 1 );
            auto hi( m->get_fs_iter( h ) );
            if( hi ) {
                ls.push( hi->end( ) );
            } else {
                ls.push(  );
                ls.push( "Bad handle." );
                return 2;
            }
        } catch ( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_fs_iter_has_next( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {
            utils::handle h = m->get_iter_object( L, 1 );
            auto hi( m->get_fs_iter( h ) );
            if( hi ) {
                ls.push( !hi->end( ) );
            } else {
                ls.push(  );
                ls.push( "Bad handle." );
                return 2;
            }
        } catch ( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_fs_iter_next( lua_State *L )
    {
        LUA_CALL_TRUE_FALSE_PROLOGUE
            utils::handle hdl = m->get_iter_object( L, 1 );
            auto hi( m->get_fs_iter( hdl ) );
            if( !hi ) {
                ls.push(  );
                ls.push( "Bad handle." );
                return 2;
            }
            hi->next( );
        LUA_CALL_TRUE_FALSE_EPILOGUE;
    }

    int lcall_fs_iter_get( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_iter_object( L, 1 );
        auto hi( m->get_fs_iter( h ) );
        if( hi ) {
            ls.push( hi->get( ).path ); // 'get' does never throw
        } else {
            ls.push(  );
            ls.push( "Bad handle." );
            return 2;
        }
        return 1;
    }

    int lcall_fs_iter_clone( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        try {
            utils::handle h = m->get_iter_object( L, 1 );
            if( h ) {
                ls.push( m->clone_fs_iter( h ) );
            } else {
                ls.push(  );
                ls.push( "Bad handle." );
                return 2;
            }
        } catch ( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    utils::handle file_from_string_mode( module *m,
                                         lua::state &ls,
                                         const std::string &path )
    {
        std::string mode( ls.get_opt<std::string>(2) );
        return m->new_file( path, mode, false );
    }

    utils::handle file_from_num_mode( module *m, lua::state &ls,
                                      const std::string &path )
    {
        utils::handle res = nullptr;
        unsigned flags( ls.get_opt<unsigned>(2) );
        unsigned mode ( ls.get_opt<unsigned>(3) );
        return m->new_file( path, flags, mode, false );

        return res;
    }

    int lcall_file_open( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        int n = ls.get_top( );
        std::string path(ls.get_opt<std::string>( 1 ));
        if( path.empty( ) ) {
            ls.push(  );
            ls.push( "Bad path." );
            return 2;
        }

        utils::handle res = nullptr;

        try {
            if( n > 1 ) {
                switch( ls.get_type( 2 ) ) {
                case base::TYPE_STRING:
                    res = file_from_string_mode( m, ls, path );
                    break;
                case base::TYPE_INTEGER:
                case base::TYPE_NUMBER:
                    res = file_from_num_mode( m, ls, path );
                    break;
                }
            } else {
                res = m->new_file( path, "rb", false );
            }
            m->push_file_object( L, res, file_meta );
            //ls.push( res );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_ioctl ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_file_object( L );
        unsigned      c = ls.get_opt<unsigned>( 2 );
        lua_Integer   p = ls.get_opt<lua_Integer>( 3 );

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( false );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            f->ioctl( c, p );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    fiface::seek_whence position_from( lua::state &ls, int pos )
    {
        if( ls.get_type( pos ) == base::TYPE_NUMBER )  {

            return fiface::whence_value2enum( ls.get_opt<unsigned>( pos ) );

        } else if( ls.get_type( pos ) == base::TYPE_STRING ) {

            std::string name = ls.get_opt<std::string>( pos );

            if( !name.compare( "set" ) ) {
                return fiface::POS_SEEK_SET;
            } else if( !name.compare( "cur" ) ) {
                return fiface::POS_SEEK_CUR;
            } else if( !name.compare( "end" ) ) {
                return fiface::POS_SEEK_END;
            }
        }
        return fiface::POS_SEEK_SET;
    }

    int lcall_file_seek ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_file_object( L );
        lua_Integer pos = ls.get_opt<lua_Integer>( 2 );
        fiface::seek_whence whence = position_from( ls, 3 );

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            ls.push( f->seek( pos, whence ) );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;

    }

    int lcall_file_tell ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_file_object( L );

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            ls.push( f->tell( ) );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_flush ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_file_object( L );

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( false );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            f->flush( );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_read ( lua_State *L )
    {
        module *m = get_module( L );

        lua::state ls( L );
        utils::handle h = m->get_file_object( L );

        int t = ls.get_type( 2 );
        unsigned max = ls.get_opt<unsigned>( 2, 44000 );

        if( max > 44000 || t == LUA_TNIL || t == LUA_TNONE ) { // fkn mgk!
            max = 44000;
        }

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            std::vector<char> data( max + 1 );
            size_t res = f->read( &data[0], max );
            if( res ) {
                ls.push( &data[0], res );
            } else {
                ls.push( );
                ls.push( "EOF." );
                return 2;
            }
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_write ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_file_object( L );
        std::string   d = ls.get_opt<std::string>( 2 );

        auto f = m->get_file_hdl(h);
        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        try {
            size_t res = f->write( d.empty( ) ? "" : &d[0], d.size( ) );
            ls.push( res );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_events ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );
        utils::handle h = m->get_file_object( L );
        auto f = m->get_eventor( h );
        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }
        return f->push_state( L );
    }

    int lcall_file_subscribe( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );
        utils::handle h = m->get_file_object( L );

        auto e = m->get_eventor( h );
        auto f = m->get_file_hdl( h );

        if( !f || !e ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        event_container::subscribe_info si;
        e->subscribe( L, 1, &si );

        if( !si.name_.compare( "on_pollin" ) ) {
            if( si.call_ )  {
                m->register_file( f, e );
            } else {
                m->unregister_file( f );
            }
        }
        return si.result_;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

