#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include <mutex>
#include <map>
#include <functional>

#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"

#include "fr-lua/lua-wrapper.hpp"

namespace fr { namespace lua {

    namespace {

        using namespace client::interfaces;
        typedef filesystem::iface                    iface;
        typedef std::unique_ptr<iface>               iface_uptr;
        typedef std::shared_ptr<
                filesystem::directory_iterator_impl> fsiterator_sptr;
        typedef std::shared_ptr<file::iface>         file_sptr;

        const std::string fs_table_path =
                std::string(lua::names::client_table)
                + '.'
                + fs::table_name;

        struct data;

        data *get_iface( lua_State *L )
        {
            return reinterpret_cast<data *>( get_component_iface(L,
                                             fs_table_path.c_str( ) ) );
        }

        int lcall_fs_pwd(    lua_State *L );
        int lcall_fs_cd(     lua_State *L );
        int lcall_fs_rename( lua_State *L );
        int lcall_fs_mkdir(  lua_State *L );
        int lcall_fs_del(    lua_State *L );
        int lcall_fs_stat(   lua_State *L );
        int lcall_fs_info(   lua_State *L );
        int lcall_fs_close(  lua_State *L );

        int lcall_fs_iter_begin( lua_State *L );
        int lcall_fs_iter_next(  lua_State *L );
        int lcall_fs_iter_get(   lua_State *L );
        int lcall_fs_iter_end(   lua_State *L );

        int lcall_fs_file_flags( lua_State *L );
        int lcall_fs_file_open(  lua_State *L );
        int lcall_fs_file_seek(  lua_State *L );
        int lcall_fs_file_tell(  lua_State *L );
        int lcall_fs_file_flush( lua_State *L );

        int lcall_fs_file_ev_reg(   lua_State *L );
        int lcall_fs_file_ev_unreg( lua_State *L );

        int lcall_fs_file_read(  lua_State *L );
        int lcall_fs_file_write( lua_State *L );

        struct data: public base_data {

            iface_uptr iface_;
            std::map<void *, fsiterator_sptr> iterators_;
            std::mutex                        iterators_lock_;

            std::map<void *, file_sptr> files_;
            std::mutex                  files_lock_;

            data( client::core::client_core &cc, lua_State *L )
                :iface_( client::interfaces::filesystem::create( cc, "" ) )
            { }

#define ADD_FILE_TABLE_FLAG( f ) \
    objects::new_string( #f ), objects::new_integer( file::flags::f )

#define ADD_FILE_TABLE_MODE( m ) \
    objects::new_string( #m ), objects::new_integer( file::mode::m )

            objects::table_sptr create_file_table( )
            {
                objects::table_sptr f( objects::new_table( ) );

                f->add( ADD_FILE_TABLE_FLAG( RDONLY ) );
                f->add( ADD_FILE_TABLE_FLAG( WRONLY ) );
                f->add( ADD_FILE_TABLE_FLAG( RDWR ) );
                f->add( ADD_FILE_TABLE_FLAG( CREAT ) );
                f->add( ADD_FILE_TABLE_FLAG( EXCL ) );
                f->add( ADD_FILE_TABLE_FLAG( APPEND ) );
                f->add( ADD_FILE_TABLE_FLAG( NONBLOCK ) );
                f->add( ADD_FILE_TABLE_FLAG( ASYNC ) );
                f->add( ADD_FILE_TABLE_FLAG( SYNC ) );

                f->add( ADD_FILE_TABLE_MODE( IRWXU ) );
                f->add( ADD_FILE_TABLE_MODE( IRUSR ) );
                f->add( ADD_FILE_TABLE_MODE( IWUSR ) );
                f->add( ADD_FILE_TABLE_MODE( IXUSR ) );
                f->add( ADD_FILE_TABLE_MODE( IRWXG ) );
                f->add( ADD_FILE_TABLE_MODE( IRGRP ) );
                f->add( ADD_FILE_TABLE_MODE( IWGRP ) );
                f->add( ADD_FILE_TABLE_MODE( IXGRP ) );
                f->add( ADD_FILE_TABLE_MODE( IRWXO ) );
                f->add( ADD_FILE_TABLE_MODE( IROTH ) );
                f->add( ADD_FILE_TABLE_MODE( IWOTH ) );
                f->add( ADD_FILE_TABLE_MODE( IXOTH ) );

                f->add( objects::new_string( "SEEK_SET" ),
                        objects::new_integer( file::POS_SEEK_SET ) );
                f->add( objects::new_string( "SEEK_CUR" ),
                        objects::new_integer( file::POS_SEEK_CUR ) );
                f->add( objects::new_string( "SEEK_END" ),
                        objects::new_integer( file::POS_SEEK_END ) );

                f->add( objects::new_string( "flags" ),
                        objects::new_function( &lcall_fs_file_flags ));
                f->add( objects::new_string( "seek" ),
                        objects::new_function( &lcall_fs_file_seek ));
                f->add( objects::new_string( "open" ),
                        objects::new_function( &lcall_fs_file_open ));
                f->add( objects::new_string( "tell" ),
                        objects::new_function( &lcall_fs_file_tell ));
                f->add( objects::new_string( "flush" ),
                        objects::new_function( &lcall_fs_file_flush ));
                f->add( objects::new_string( "read" ),
                        objects::new_function( &lcall_fs_file_read ));
                f->add( objects::new_string( "write" ),
                        objects::new_function( &lcall_fs_file_write ));
                f->add( objects::new_string( "register_for_events" ),
                        objects::new_function( &lcall_fs_file_ev_reg ));
                f->add( objects::new_string( "unregister" ),
                        objects::new_function( &lcall_fs_file_ev_unreg ));


                return f;
            }

#undef ADD_FILE_TABLE_MODE
#undef ADD_FILE_TABLE_FLAG

            lua::objects::table_sptr init_table( )
            {
                objects::table_sptr tabl( objects::new_table( ) );

                tabl->add( objects::new_string( names::inst_field ),
                           objects::new_light_userdata( this ))
                    /* ==== calls ==== */
                    ->add( objects::new_string( "pwd" ),
                           objects::new_function( &lcall_fs_pwd ))
                    ->add( objects::new_string( "cd" ),
                           objects::new_function( &lcall_fs_cd ))
                    ->add( objects::new_string( "stat" ),
                           objects::new_function( &lcall_fs_stat ))
                    ->add( objects::new_string( "info" ),
                           objects::new_function( &lcall_fs_info ))
                    ->add( objects::new_string( "mkdir" ),
                           objects::new_function( &lcall_fs_mkdir ))
                    ->add( objects::new_string( "del" ),
                           objects::new_function( &lcall_fs_del ))
                    ->add( objects::new_string( "rename" ),
                           objects::new_function( &lcall_fs_rename ))
                    ->add( objects::new_string( "close" ),
                           objects::new_function( &lcall_fs_close ))
                    /* ==== iterators ==== */
                    ->add( objects::new_string( "iter_begin" ),
                           objects::new_function( &lcall_fs_iter_begin ))
                    ->add( objects::new_string( "iter_get" ),
                           objects::new_function( &lcall_fs_iter_end ))
                    ->add( objects::new_string( "iter_next" ),
                           objects::new_function( &lcall_fs_iter_next ))
                    ->add( objects::new_string( "iter_end" ),
                           objects::new_function( &lcall_fs_iter_end ))
                    /* ==== files ==== */
                    ->add( objects::new_string( "file" ),
                           create_file_table( ))


                ;
                return tabl;
            }
        };

        int lcall_fs_pwd( lua_State *L )
        {
            data *i = get_iface( L );
            std::string path( i->iface_->pwd( ) );
            lua::state ls(L);
            ls.push( path.c_str( ) );
            return 1;
        }

        int lcall_fs_cd( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );
                std::string path( ls.get<std::string>( ) );
                ls.pop( );
                i->iface_->cd( path );
            }
            return 0;
        }

        int lcall_fs_close( lua_State *L )
        {
            lua::state ls(L);
            void *p = ls.get<void *>( );
            ls.pop( );
            fsiterator_sptr ni;
            data *i = get_iface( L );
            {
                std::lock_guard<std::mutex> lck(i->iterators_lock_);
                size_t r = i->iterators_.erase( p );
                if( r ) return 0;
            }

            {
                std::lock_guard<std::mutex> lck(i->files_lock_);
                size_t r = i->files_.erase( p );
                if( r ) return 0;
            }

            return 0;
        }

        int lcall_fs_rename( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );
                std::string path( ls.get<std::string>( 1 ) );
                std::string out;
                if( params > 1 ) {
                    out = ls.get<std::string>( 2 );
                }
                ls.pop( params );
                if( out.empty( ) ) {
                    i->iface_->del( path );
                } else {
                    i->iface_->rename( path, out );
                }
            }
            return 0;

        }

        int lcall_fs_mkdir( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );
                std::string path( ls.get<std::string>( ) );
                ls.pop( );
                i->iface_->mkdir( path );
            }
            return 0;
        }

        int lcall_fs_del( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );
                std::string path( ls.get<std::string>( ) );
                ls.pop( );
                i->iface_->del( path );
            }
            return 0;
        }


#define ADD_TABLE_STAT_FIELD( sd, name ) \
    objects::new_string( #name ), objects::new_integer( sd.name )

        int lcall_fs_stat( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );
                std::string path( ls.get<std::string>( ) );
                ls.pop( );
                client::interfaces::filesystem::stat_data sd;
                i->iface_->stat( path, sd );

                objects::table_sptr t(objects::new_table( ) );
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
                return 1;
            }
            return 0;
        }
#undef ADD_TABLE_STAT_FIELD


#define ADD_TABLE_INFO_FIELD( sd, name ) \
    objects::new_string( #name ), objects::new_boolean( sd.name )

        int lcall_fs_info( lua_State *L )
        {
            lua::state ls(L);
            int params = ls.get_top( );
            if( params ) {
                data *i = get_iface( L );

                std::string path( ls.get<std::string>( ) );

                ls.pop( );

                client::interfaces::filesystem::info_data id;
                objects::table_sptr t(objects::new_table( ) );

                try {
                    i->iface_->info( path, id );
                    t->add( ADD_TABLE_INFO_FIELD( id, is_exist ) );
                    t->add( ADD_TABLE_INFO_FIELD( id, is_directory ) );
                    t->add( ADD_TABLE_INFO_FIELD( id, is_empty ) );
                    t->add( ADD_TABLE_INFO_FIELD( id, is_regular ) );
                    t->add( ADD_TABLE_INFO_FIELD( id, is_symlink ) );

                } catch( ... ) {
                   t->add( objects::new_string( "failed" ),
                           objects::new_boolean( true ) );
                }
                t->push( L );

                return 1;
            }
            return 0;
        }
#undef ADD_TABLE_INFO_FIELD

        const char *path_leaf( const char *path )
        {
            const char *p  = path;
            const char *sp = path;
            for( ; *p; ++p ) {
                if( *p == '/' || *p == '\\' ) {
                    sp = p;
                }
            }
            return sp == path ? sp : sp + 1;
        }

        int lcall_fs_iter_begin( lua_State *L )
        {
            data *i = get_iface( L );
            fsiterator_sptr ni(i->iface_->begin_iterate( ));
            {
                std::lock_guard<std::mutex> lck(i->iterators_lock_);
                i->iterators_.insert( std::make_pair(ni.get( ), ni) );
            }
            lua::state ls(L);
            ls.push( path_leaf(ni->get( ).path.c_str( )) );
            ls.push( reinterpret_cast<void *>( ni.get( )) );
            return 2;
        }

        fsiterator_sptr get_iterator( lua_State *L )
        {
            lua::state ls(L);
            void *p = ls.get<void *>( );
            ls.pop( );
            fsiterator_sptr ni;
            data *i = get_iface( L );
            {
                std::lock_guard<std::mutex> lck(i->iterators_lock_);
                std::map<
                        void *,
                        fsiterator_sptr
                >::const_iterator f( i->iterators_.find( p ) );

                if( f != i->iterators_.end( ) ) {
                    ni = f->second;
                }
            }
            return ni;
        }

        int lcall_fs_iter_next( lua_State *L )
        {
            lua::state ls(L);
            fsiterator_sptr ni( get_iterator( L ) );
            if( ni ) {
                ni->next( );
                ls.push( path_leaf( ni->get( ).path.c_str( ) ) );
                return 1;
            }
            return 0;
        }

        int lcall_fs_iter_get(   lua_State *L )
        {
            lua::state ls(L);
            fsiterator_sptr ni( get_iterator( L ) );
            if( ni ) {
                ls.push( ni->get( ).path.c_str( ) );
                return 1;
            }
            return 0;
        }

        int lcall_fs_iter_end( lua_State *L )
        {
            lua::state ls(L);
            fsiterator_sptr ni( get_iterator( L ) );
            if( ni ) {
                ls.push( ni->end( ) );
                return 1;
            }
            ls.push( true );
            return 1;
        }

        /*  FILES */

        file_sptr get_file( lua_State *L, int id = -1 )
        {
            lua::state ls(L);
            void *p = ls.get<void *>( id );
            file_sptr ni;
            data *i = get_iface( L );
            {
                std::lock_guard<std::mutex> lck(i->files_lock_);
                std::map<
                        void *,
                        file_sptr
                >::const_iterator f( i->files_.find( p ) );

                if( f != i->files_.end( ) ) {
                    ni = f->second;
                }
            }
            return ni;
        }

        int lcall_fs_file_flags( lua_State *L )
        {
            lua::state ls(L);

            int n = ls.get_top( );

            unsigned result = 0;

            for ( int b = 1; b <= n; ++b ) {
                try {
                    result |= ls.get<unsigned>( b );
                } catch( ... ) {
                    ;;;
                }
            }
            ls.pop( n );

            ls.push( result );
            return 1;
        }

        int lcall_fs_file_open( lua_State *L )
        {
            lua::state ls(L);
            int n = ls.get_top( );
            std::string path;
            unsigned flags = 0;
            unsigned mode = 0;

            if( n > 0 ) {
                path = ls.get<std::string>( 1 );
            }
            if( n > 1 ) {
                flags = ls.get<unsigned>( 2 );
            }
            if( n > 2 ) {
                mode = ls.get<unsigned>( 3 );
            }

            ls.pop( n );
            data *i = get_iface( L );
            client::core::client_core *cc = lua::get_core( L );
            try {
                file_sptr f(file::create( *cc, path, flags, mode ));

                std::lock_guard<std::mutex> lg(i->files_lock_);
                i->files_.insert( std::make_pair( f.get( ), f ) );

                ls.push( f.get( ) );
                return 1;
            } catch( ... ) {

            }
            return 0;
        }

        file::seek_whence from_unsigned( unsigned v )
        {
            switch ( v ) {
            case file::POS_SEEK_CUR:
                return file::POS_SEEK_CUR;
            case file::POS_SEEK_SET:
                return file::POS_SEEK_SET;
            case file::POS_SEEK_END:
                return file::POS_SEEK_END;
            }
            return file::POS_SEEK_SET;

        }

        int lcall_fs_file_seek(  lua_State *L )
        {
            lua::state ls(L);
            unsigned whence = 0;
            lua_Integer pos = 0;

            int n = ls.get_top( );

            file_sptr f( get_file( L, 1 ) );

            if( n > 1 ) {
                whence = ls.get<unsigned>( 2 );
            }
            if( n > 2 ) {
                pos = ls.get<lua_Integer>( 3 );
            }
            ls.pop( n );

            ls.push( f->seek( pos, from_unsigned( whence ) ) );
            return 1;
        }

        int lcall_fs_file_tell(  lua_State *L )
        {
            lua::state ls(L);

            file_sptr f(get_file( L ));
            ls.clean( );

            ls.push( f->tell( ) );
            return 1;
        }

        int lcall_fs_file_flush( lua_State *L )
        {
            lua::state ls(L);
            file_sptr f(get_file( L ));
            ls.clean( );
            f->flush( );
            return 0;
        }

        int lcall_fs_file_read(  lua_State *L )
        {
            lua::state ls(L);
            int n = ls.get_top( );
            unsigned maximum = 44000;
            file_sptr f(get_file( L, 1 ));
            if( n > 1 ) {
                maximum = ls.get<unsigned>( 2 );
                if( maximum > 44000 ) maximum = 44000;
            }
            ls.pop( n );
            if( maximum ) {
                std::vector<char> data(maximum);
                size_t r = f->read( &data[0], maximum );
                ls.push( &data[0], r );
                return 1;
            }
            return 0;
        }

        int lcall_fs_file_write( lua_State *L )
        {
            lua::state ls(L);

            file_sptr f(get_file( L, 1 ));
            std::string data( ls.get<std::string>( 2 ) );

            ls.clean( );

            size_t pos = 0;
            const size_t w = data.size( );
            while( pos != w ) {
                pos += f->write( &data[pos], w - pos );
            }
            ls.push( w );
            return 1;
        }

        void file_event_handler( unsigned error, const std::string &data,
                                 lua_State *L, const char *fcall ) try
        {
            lua::state ls(L);
            ls.exec_function( fcall, error, data );
        } catch( ... ) {
            std::cout << "call erro " << "\n";
        }

        int lcall_fs_file_ev_reg( lua_State *L )
        {
            lua::state ls(L);

            file_sptr f(get_file( L, 1 ));
            const char * call = ls.get<const char *>( 2 );
            ls.clean( );
            f->register_for_events( std::bind( file_event_handler,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               L, call ) );
            return 0;
        }

        int lcall_fs_file_ev_unreg( lua_State *L )
        {
            lua::state ls(L);
            file_sptr f(get_file( L, 1 ));
            ls.clean( );
            f->unregister( );
            return 0;
        }

    }

    namespace fs {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            return data_sptr( new data( cc, ls ) );
        }
    }

}}

#endif
