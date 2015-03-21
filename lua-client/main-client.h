#ifndef FR_LUA_MAINCLIENT_H
#define FR_LUA_MAINCLIENT_H

#include <memory>

struct lua_State;

namespace fr { namespace lua { namespace client {

    struct general_info;

    general_info *get_gen_info( lua_State *L );

    int lua_call_connect( lua_State *L );
    int lua_call_disconnect( lua_State *L );

    int global_init( general_info *info, bool connect );
    int events_init( general_info *info );

//    class main_client {

//        struct impl;
//        std::unique_ptr<impl> impl_;

//    public:
//        main_client( general_info &info );
//    };

}}}

#endif // MAINCLIENT_H
