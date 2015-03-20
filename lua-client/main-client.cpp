
#include "main-client.h"
#include "lua-names.h"
#include "general-info.h"

#include "fr-lua.h"
#include "fr-client.h"
#include "event-container.h"

#include "boost/system/error_code.hpp"

#include <functional>

namespace fr { namespace lua { namespace client {

    namespace {

        using namespace objects;

        void on_connect( general_info *info )
        {
            FR_LUA_EVENT_PROLOGUE( "on_connect", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
            // std::cout << "connect...";
        }

        void on_disconnect( general_info *info )
        {
            FR_LUA_EVENT_PROLOGUE( "on_disconnect", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
            // std::cout << "disconnect...\n";
        }

        void on_ready( general_info *info )
        {
            info->connected_ = true;
            FR_LUA_EVENT_PROLOGUE( "on_ready", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
            //std::cout << "ready...\n";
        }

        void on_init_error( const char *message, general_info *info )
        {
            FR_LUA_EVENT_PROLOGUE( "on_init_error", *info->general_events_ );
            result->add( "message", new_string( message ) );
            FR_LUA_EVENT_EPILOGUE;
            // std::cout << "init error '" << message << "'\n";
        }

        std::vector<std::string> events_async( )
        {
            std::vector<std::string> res;
            res.push_back( "on_connect" );
            res.push_back( "on_disconnect" );
            res.push_back( "on_init_error" );
            res.push_back( "on_ready" );
            return res;
        }

        std::vector<std::string> events( )
        {
            std::vector<std::string> res;
            res.push_back( "on_connect" );
            res.push_back( "on_disconnect" );
            res.push_back( "on_init_error" );
            res.push_back( "on_ready" );
            return res;
        }

        int lcall_events( lua_State *L )
        {
            general_info *g = get_gen_info( L );
            return g->general_events_->push_state( L );
        }

        int lcall_exit( lua_State *L )
        {
            general_info *g = get_gen_info( L );
            g->tp_->stop( );
            return 0;
        }

        int lcall_subscribe( lua_State *L )
        {
            general_info *g = get_gen_info( L );
            g->general_events_->subscribe( L );
            return 0;
        }

        int lcall_global_print( lua_State *L )
        {
            lua::state ls( L );
            const int n = ls.get_top( );

            for( int i=1; i<=n; ++i ) {
                objects::base_sptr o(ls.get_object( i, 1 ));
                std::cout << o->str( );
            }
            return 0;
        }

        objects::table_sptr common_table( general_info   *info,
                                          objects::table &main )
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));
            main.add( "events",  new_function( &lcall_events ))
               ->add( "subscribe", new_function( &lcall_subscribe ) );
            return res;
        }

        objects::table_sptr connect_table( general_info   *info,
                                           objects::table &main )
        {
            objects::table_sptr res(common_table(info, main));
            main.add( "disconnect", new_function( &lua_call_disconnect ) );
            return res;
        }

        objects::table_sptr disconnect_table( general_info   *info,
                                              objects::table &main )
        {
            objects::table_sptr res(common_table(info, main));
            main.add( "connect", new_function( &lua_call_connect ) );
            return res;
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
            info->general_events_.reset(
                           new lua::event_container( *info, events_async( ) ) );
            info->client_core_->async_connect( server, [ ]( ... ){ } );
        } else {
            info->general_events_.reset(
                           new lua::event_container( *info, events( ) ) );
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

    int global_init( general_info *info, bool connect )
    {
        lua::state ls(info->main_);
        objects::table_sptr fr_table(std::make_shared<objects::table>( ));

        fr_table->add( "exit", new_function( &lcall_exit ) );
        objects::table_sptr ct(new_table( ));

        if( !connect ) {

            disconnect_table( info, *ct );

        } else {
            std::string server;
            std::string id;
            std::string key;

            if( info->cmd_opts_.count( "server" ) ) {
                server = info->cmd_opts_["server"].as<std::string>( );
            }
            if( info->cmd_opts_.count( "id" ) ) {
                id = info->cmd_opts_["id"].as<std::string>( );
            }
            if( info->cmd_opts_.count( "key" ) ) {
                key = info->cmd_opts_["key"].as<std::string>( );
            }
            make_connect( info, server, id, key, false );
            info->connected_ = true;
            connect_table( info, *ct );
        }

        fr_table->add( "client", ct );

        ls.set_object( FR_CLIENT_LUA_MAIN_TABLE, fr_table.get( ) );

        ls.set_object( FR_CLIENT_LUA_MAIN_TABLE ".print",
                       new_function( &lcall_global_print ) );

        return 0;
    }

//    struct main_client::impl {
//        general_info &info_;
//        impl( general_info &info )
//            :info_(info)
//        { }
//    };

//    main_client::main_client( general_info &info )
//        :impl_(new impl(info))
//    { }

}}}
