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

        struct data: public base_data {

            iface_uptr iface_;

            data( client::core::client_core &cc, lua_State *L )
                :iface_( client::interfaces::filesystem::create( cc, "" ) )
            {

            }

            lua::objects::table_sptr get_table( )
            {
                objects::table_sptr ost(objects::new_table( ));
                ost->add( objects::new_string( names::inst_field ),
                          objects::new_light_userdata( iface_.get( ) )
                );
                return ost;
            }
        };
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
