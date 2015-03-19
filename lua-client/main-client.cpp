
#include "main-client.h"
#include "lua-names.h"
#include "general-info.h"

#include "fr-lua.h"
#include "fr-client.h"

namespace fr { namespace lua { namespace client {

    namespace {
        void on_connect( )
        {
            // std::cout << "connect...";
        }

        void on_disconnect( )
        {
            // std::cout << "disconnect...\n";
        }

        void on_ready( )
        {
            // std::cout << "ready...\n";
        }

        void on_init_error( const char *message )
        {
            // std::cout << "init error '" << message << "'\n";
        }

    }

    void make_connect( general_info *info,
                       const std::string &server,
                       const std::string &id,
                       const std::string &key )
    {
        info->
    }

    general_info *get_gen_info( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( FR_CLIENT_GEN_INFO_PATH );
        return static_cast<general_info *>(ptr);
    }

    int lua_call_connect( lua_State *L )
    {
        general_info *g = get_gen_info( L );

        return 0;
    }

    int lua_call_disconnect( lua_State *L )
    {
        return 0;
    }

}}}
