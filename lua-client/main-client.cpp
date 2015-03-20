
#include "main-client.h"
#include "lua-names.h"
#include "general-info.h"

#include "fr-lua.h"
#include "fr-client.h"
#include "boost/system/error_code.hpp"

#include <functional>

namespace fr { namespace lua { namespace client {

    namespace {

        void on_connect( general_info *info )
        {
            // std::cout << "connect...";
        }

        void on_disconnect( general_info *info )
        {
            // std::cout << "disconnect...\n";
        }

        void on_ready( general_info *info )
        {
            // std::cout << "ready...\n";
        }

        void on_init_error( const char *message, general_info *info )
        {
            // std::cout << "init error '" << message << "'\n";
        }

    }

    void make_connect( general_info *info,
                       const std::string &server,
                       const std::string &id,
                       const std::string &key,
                       bool async )
    {
        typedef fr::client::core::client_core client_core;
        auto ccl = std::make_shared<client_core>(*info->pp_);

        if( !id.empty( ) ) {
            ccl->set_id( id );
        }
        if( !key.empty( ) ) {
            ccl->set_key( key );
        }

        ccl->on_connect_connect( client_core::on_connect_slot_type(
                                    on_connect, info ) );

        ccl->on_disconnect_connect( client_core::on_disconnect_slot_type(
                                    on_disconnect, info ) );

        ccl->on_ready_connect( client_core::on_ready_slot_type(
                               on_ready, info ) );

        ccl->on_init_error_connect( client_core::on_init_error_slot_type(
                                    on_init_error, _1, info ) );

        info->client_core_.swap( ccl );
        if( async ) {
            info->client_core_->async_connect( server, [ ]( ... ){ } );
        } else {
            info->client_core_->connect( server );
        }

    }

    general_info *get_gen_info( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( FR_CLIENT_GEN_INFO_PATH );
        return static_cast<general_info *>(ptr);
    }

    int lua_call_connect( lua_State *L )
    {
        //general_info *g = get_gen_info( L );

        return 0;
    }

    int lua_call_disconnect( lua_State *L )
    {
        return 0;
    }

    struct main_client::impl {
        general_info &info_;
        impl( general_info &info )
            :info_(info)
        { }
    };

    main_client::main_client( general_info &info )
        :impl_(new impl(info))
    { }

}}}
