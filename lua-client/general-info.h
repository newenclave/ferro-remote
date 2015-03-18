#ifndef FR_LUA_CLIENT_GENERALINFO_H
#define FR_LUA_CLIENT_GENERALINFO_H

struct lua_State;

#include "event-caller.h"
#include "modules/iface.h"

#include "boost/program_options/variables_map.hpp"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace fr { namespace lua { namespace client {

    struct general_info {

        lua_State                   *main_;

        fr::lua::event_caller_sptr   eventor_;
        int                          exit_code_;
        m::modules_list              modules_;

        std::shared_ptr<vtrc::client::vtrc_client> client_core_;
        boost::program_options::variables_map      cmd_opts_;

        general_info( )
            :exit_code_(0)
        { }
    };

}}}

#endif // FR_LUA_CLIENT_GENERALINFO_H
