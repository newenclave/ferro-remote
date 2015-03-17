#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IOS.h"

namespace fr { namespace lua {

    namespace {

        typedef client::interfaces::os::iface iface;
        typedef std::unique_ptr<iface> iface_uptr;

        const char *system_call_name = "system";
        const char *exec_call_name   = "execute";

        iface *get_os_iface( lua_State *L )
        {
            return reinterpret_cast<iface *>( get_component_iface(L,
                                              os::table_path( ) ) );
        }

        int lcall_os_exec( lua_State *L )
        {
            lua::state lv( L );
            std::string command( lv.get<std::string>( ) );
            iface *i( get_os_iface( L ) );
            lv.push( i->execute( command ) );
            return 1;
        }

        struct data: public base_data {

            iface_uptr iface_;

            data( client::core::client_core &cc, std::shared_ptr<lua::state> )
                :iface_(client::interfaces::os::create(cc))
            { }

            lua::objects::table_sptr init_table( )
            {
                objects::table_sptr ost(objects::new_table( ));
                ost->add( system_call_name,
                          objects::new_function( &lcall_os_exec ))
                   ->add( exec_call_name,
                          objects::new_function( &lcall_os_exec ))
                   ->add( names::inst_field,
                          objects::new_light_userdata( iface_.get( ) ) );

                return ost;
            }
        };
    }

    namespace os {
        const char *table_name( ) { return "os"; }
        const char *table_path( )
        {
            static const std::string path =
                         std::string( names::client_table )
                       + '.'
                       + table_name( );
            return path.c_str( );
        }
        data_sptr init( std::shared_ptr<lua::state> &ls,
                        client::core::client_core &cc )
        {
            return data_sptr( new data( cc, ls ) );
        }
    }

}}

#endif
