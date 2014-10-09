#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IOS.h"

namespace fr { namespace lua {

    namespace {

        typedef client::interfaces::os::iface iface;
        typedef std::unique_ptr<iface> iface_uptr;

        const char *os_iface_name    = "osiface";
        const char *system_call_name = "system";

        iface *get_os_iface( lua_State *L )
        {
            lua::state lv( L );
            void *p = lv.get_from_global<void *>( lua::names::main_table,
                                                  os_iface_name );
            return reinterpret_cast<iface *>(p);
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
            {
                lua::state lv( L );
                lv.set_in_global( lua::names::main_table,
                                  os_iface_name,
                                  reinterpret_cast<void *>( iface_.get( )));
            }

            lua::objects::table_sptr get_table( )
            {
                objects::table_sptr ost(objects::new_table( ));
                ost->add(
                    objects::new_string( system_call_name ),
                    objects::new_function( lcall_os_exec )
                );
                return ost;
            }
        };
    }

    namespace os {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            lua::state lv( ls );
            return std::make_shared<data>( std::ref( cc ), ls );
        }
    }

}}

#endif
