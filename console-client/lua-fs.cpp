#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IOS.h"

namespace fr { namespace lua {

    namespace {

        const char *fsface_name = "fsiface";


        struct data: public base_data {

            typedef client::interfaces::os::iface iface;
            typedef std::unique_ptr<iface> iface_uptr;

            data( client::core::client_core &cc, lua_State *L )
            {

            }

            lua::objects::table_sptr get_table( )
            {
                objects::table_sptr ost(objects::new_table( ));
                return ost;
            }

        };
    }

    namespace fs {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            lua::state lv( ls );
            return data_sptr( new data( cc, ls ) );
        }
    }

}}

#endif
