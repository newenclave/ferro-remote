#include "lua-interface.h"

#if FR_WITH_LUA

#include "interfaces/IOS.h"

namespace fr { namespace lua {

    namespace {

        typedef client::interfaces::os::iface iface;
        typedef std::unique_ptr<iface> iface_uptr;

        struct data: public base_data {

            iface_uptr iface_;

            data(  )
            {

            }

            virtual lua::objects::table_sptr get_table( )
            {
                return lua::objects::table_sptr( );
            }
        };
    }

    namespace os {
        data_sptr init( lua_State *ls, const char *core_id )
        {
            lua::state lv( ls );
        }
    }

}}

#endif
