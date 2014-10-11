#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IOS.h"

namespace fr { namespace lua {

    namespace {

        typedef client::interfaces::os::iface iface;
        typedef std::unique_ptr<iface> iface_uptr;

        const std::string fs_table_path =
                std::string(lua::names::client_table)
                + '.'
                + os::table_name;

        const char *system_call_name = "system";

        iface *get_os_iface( lua_State *L )
        {
            return reinterpret_cast<iface *>( get_component_iface(L,
                                              fs_table_path.c_str( ) ) );
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

            data( client::core::client_core &cc, lua_State *L )
                :iface_(client::interfaces::os::create(cc))
            { }

            lua::objects::table_sptr get_table( )
            {
                objects::table_sptr ost(objects::new_table( ));
                ost->add(
                    objects::new_string( system_call_name ),
                    objects::new_function( lcall_os_exec )
                )->add( objects::new_string( "__i" ),
                        objects::new_light_userdata( iface_.get( ) ) );
                return ost;
            }
        };
    }

    namespace os {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            lua::state lv( ls );
            return data_sptr( new data( cc, ls ) );
        }
    }

}}

#endif
