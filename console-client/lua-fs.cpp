#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IFilesystem.h"

#include "fr-lua/lua-wrapper.hpp"

namespace fr { namespace lua {

    namespace {

        const char *fsface_name = "fsiface";

        typedef client::interfaces::filesystem::iface iface;
        typedef std::unique_ptr<iface>          iface_uptr;

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

        int lcall_fs_pwd( lua_State  *L );
        int lcall_fs_cd( lua_State   *L );
        int lcall_fs_stat( lua_State *L );

        struct data: public base_data {

            iface_uptr iface_;

            data( client::core::client_core &cc, lua_State *L )
                :iface_( client::interfaces::filesystem::create( cc, "" ) )
            {

            }

            lua::objects::table_sptr get_table( )
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
