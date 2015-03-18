#ifndef FR_LUA_CLIENT_GENERALINFO_H
#define FR_LUA_CLIENT_GENERALINFO_H

struct lua_State;

#include "event-caller.h"
#include "modules/iface.h"

namespace fr { namespace lua { namespace client {

    struct general_info {

        lua_State                   *main_;
        fr::lua::event_caller_sptr   eventor_;
        int                          exit_code_;
        m::modules_list              modules_;

        general_info( )
            :exit_code_(0)
        { }
    };

}}}

#endif // FR_LUA_CLIENT_GENERALINFO_H
