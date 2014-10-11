#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include <mutex>
#include <map>
#include "interfaces/IFilesystem.h"

#include "fr-lua/lua-wrapper.hpp"

namespace fr { namespace lua {

    namespace {

        using namespace client::interfaces;
        typedef filesystem::iface                    iface;
        typedef std::unique_ptr<iface>               iface_uptr;
        typedef std::shared_ptr<
                filesystem::directory_iterator_impl> fsiterator_sptr;

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

        int lcall_fs_pwd(  lua_State  *L );
        int lcall_fs_cd(   lua_State  *L );
        int lcall_fs_stat(  lua_State *L );
        int lcall_fs_info(  lua_State *L );
        int lcall_fs_close( lua_State *L );

        int lcall_fs_iter_begin( lua_State *L );
        int lcall_fs_iter_next( lua_State *L );
        int lcall_fs_iter_end( lua_State *L );

        struct data: public base_data {

            iface_uptr iface_;
            std::map<void *, fsiterator_sptr> iterators_;
            std::mutex                        iterators_lock_;

            data( client::core::client_core &cc, lua_State *L )
                :iface_( client::interfaces::filesystem::create( cc, "" ) )
            { }

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
                    ->add( objects::new_string( "close" ),
                           objects::new_function( &lcall_fs_close ))
                    /* ==== iterators ==== */
                    ->add( objects::new_string( "iter_begin" ),
                           objects::new_function( &lcall_fs_iter_begin ))
                    ->add( objects::new_string( "iter_end" ),
                           objects::new_function( &lcall_fs_iter_end ))
                    ->add( objects::new_string( "iter_next" ),
                           objects::new_function( &lcall_fs_iter_next ))
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
            std::lock_guard<std::mutex> lck(i->iterators_lock_);
            i->iterators_.erase( p );

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
    }

    namespace fs {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            //lua::state lv( ls );
            return data_sptr( new data( cc, ls ) );
        }
    }

}}

#endif
