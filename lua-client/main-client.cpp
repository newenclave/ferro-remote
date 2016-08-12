#include <iostream>
#include <functional>

#include "main-client.h"
#include "lua-names.h"
#include "general-info.h"

#include "fr-lua.h"
#include "fr-client.h"
#include "event-container.h"
#include "modules/iface.h"

#include "vtrc-common/vtrc-hash-iface.h"

#include "boost/system/error_code.hpp"
#include "boost/asio.hpp"
#include "boost/filesystem.hpp"

#include "interfaces/IInternal.h"
#include "vtrc-memory.h"

#include "net-ifaces.h"

namespace fr { namespace lua { namespace client {

    namespace {

        using namespace objects;
        namespace vcomm = vtrc::common;
        namespace fs = boost::filesystem;

        objects::table_sptr common_table( general_info   *info,
                                          objects::table &main );

        void on_connect( general_info *info )
        {
            FR_LUA_EVENT_PROLOGUE( "on_connect", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
        }

        void on_disconnect( general_info *info )
        {
            info->connected_ = false;
            global_init( info, false );
            FR_LUA_EVENT_PROLOGUE( "on_disconnect", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
        }

        void on_ready( general_info *info )
        {
            info->connected_ = true;
            global_init( info, false );

            FR_LUA_EVENT_PROLOGUE( "on_ready", *info->general_events_ );
            FR_LUA_EVENT_EPILOGUE;
        }

        void on_init_error( const char *message, general_info *info )
        {
            FR_LUA_EVENT_PROLOGUE( "on_init_error", *info->general_events_ );
            result->add( "message", new_string( message ) );
            FR_LUA_EVENT_EPILOGUE;
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
            if( g->eventor_->get_enable( ) ) {
                lua::state ls(L);
                g->exit_code_ = ls.get_opt<int>( 1 );
                g->tp_->stop( );
                g->eventor_->set_enable( false );
            }
            return 0;
        }

//        int lcall_eval( lua_State *L )
//        {
//            lua::state ls( L );
//            std::string buf(ls.get_opt<std::string>( 1 ));
//            return ls.load_buffer( buf.c_str( ), buf.size( ) );
//        }

        int lcall_subscribe( lua_State *L )
        {
            general_info *g = get_gen_info( L );
            g->general_events_->subscribe( L );
            return 0;
        }

        int lcall_start_events( lua_State *L )
        {
            general_info *g = get_gen_info( L );
            g->eventor_->set_enable( true );
            lua_pushboolean( L, true );
            return 1;
        }

        int lcall_global_print( lua_State *L )
        {
            lua::state ls( L );
            const int n = ls.get_top( );

            for( int i=1; i<=n; ++i ) {
                objects::base_sptr o(ls.get_object( i, 1 ));
                std::cout << o->str( );
            }
            std::cout.flush( );
            return 0;
        }

        int lcall_global_netifaces( lua_State *L )
        {
            lua::state ls( L );
            objects::table t;
            auto ifaces = utilities::get_system_ifaces( );
            using objects::new_string;
            using objects::new_boolean;
            for( auto &i: ifaces ) {
                auto &addr(i.addr( ));
                auto &mask(i.addr( ));
                t.add( objects::new_table( )
                   ->add( "name",        new_string( i.name( ) ) )
                   ->add( "address",     new_string( addr.to_string( ) ) )
                   ->add( "mask",        new_string( mask.to_string( ) ) )
                   ->add( "is_loopback", new_boolean( addr.is_loopback( ) ) )
                   ->add( "is_v4",       new_boolean( addr.is_v4( ) ) )
                   ->add( "is_v6",       new_boolean( addr.is_v4( ) ) )
                 );
            }
            t.push( L );
            return 1;
        }

        objects::table_sptr common_table( general_info   *info,
                                          objects::table &main )
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));
            main.add( "events",  new_function( &lcall_events ))
               ->add( "subscribe", new_function( &lcall_subscribe ) );

            if( info->connected_ ) {
                for( auto &m: info->modules_ ) {
                    m->reinit( );
                    main.add( m->name( ), m->table( ) );
                }
            } else {
                for( auto &m: info->modules_ ) {
                    if( !m->connection_required( ) ) {
                        main.add( m->name( ), m->table( ) );
                    }
                }
            }

            return res;
        }

        objects::table_sptr connect_table( general_info   *info,
                                           objects::table &main )
        {
            objects::table_sptr res(common_table(info, main));
            main.add( "connect",    new_function( &lua_call_connect ) );
            main.add( "ping",       new_function( &lua_call_ping ) );
            main.add( "info",       new_function( &lua_call_info ) );
            //main.add( "quit",       new_function( &lua_call_exit ) );
            main.add( "shutdown",   new_function( &lua_call_exit ) );
            main.add( "disconnect", new_function( &lua_call_disconnect ) );
            main.add( "connected",  new_boolean ( true ) );

            return res;
        }

        objects::table_sptr disconnect_table( general_info   *info,
                                              objects::table &main )
        {
            objects::table_sptr res(common_table(info, main));
            main.add( "connect",    new_function( &lua_call_connect ) );
            main.add( "connected",  new_boolean( false ) );
            return res;
        }

    } // namespace

    void make_connect( general_info *info,
                       const std::string &server,
                       const std::string &id,
                       const std::string &key,
                       bool async )
    {
        typedef fr::client::core::client_core client_core;

        auto ccl = std::make_shared<client_core>( std::ref(*info->pp_) );
                                                  /// crutch for VS2013
        if( !id.empty( ) ) {
            ccl->set_id( id );
        }
        if( !key.empty( ) ) {
            ccl->set_key( key );
        }

        boost::asio::io_service &ios( info->tp_->get_io_service( ) );

        ccl->on_connect_connect( ios.wrap(
                                        client_core::on_connect_slot_type(
                                        on_connect, info ) ) );

        ccl->on_disconnect_connect( ios.wrap(
                                        client_core::on_disconnect_slot_type(
                                        on_disconnect, info ) ) );

        ccl->on_ready_connect( ios.wrap(
                                        client_core::on_ready_slot_type(
                                        on_ready, info ) ) );

        ccl->on_init_error_connect( ios.wrap(
                                        client_core::on_init_error_slot_type(
                                        on_init_error, _1, info ) ) );

        info->client_core_.swap( ccl );

        if( ccl ) {
            ccl->disconnect( );
        }

        if( async ) {
            const fr::client::core::client_core::async_closure_func async_cb;
            info->client_core_->async_connect( server, async_cb );
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

    std::string hash_key( const std::string &id, const std::string &key )
    {
        std::string key_info( id + key );
        vcomm::hash_iface_uptr s(vcomm::hash::sha2::create256( ));
        std::string hs(s->get_data_hash( &key_info[0], key_info.size( ) ));
        return hs;
    }

    int lua_call_ping( lua_State *L )
    {
        general_info *info = get_gen_info( L );
        lua::state ls(L);
        namespace iproto = fr::client::interfaces::internal;
        typedef iproto::iface iface_type;
        vtrc::unique_ptr<iface_type> i;
        try {
            i.reset( iproto::create( *info->client_core_ ) );
            ls.push( i->ping( ) );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lua_call_info( lua_State *L )
    {
        general_info *info = get_gen_info( L );
        lua::state ls(L);
        namespace iproto = fr::client::interfaces::internal;
        typedef iproto::iface iface_type;
        vtrc::unique_ptr<iface_type> i;
        try {
            i.reset( iproto::create( *info->client_core_ ) );
            iproto::agent_info inf = i->info( );
            objects::table ti;
            ti.add( "name",         new_string ( inf.name ) );
            ti.add( "now",          new_integer( inf.now ) );
            ti.add( "tick_count",   new_integer( inf.tick_count ) );
            ti.push( L );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lua_call_exit( lua_State *L )
    {
        general_info *info = get_gen_info( L );
        lua::state ls(L);
        namespace iproto = fr::client::interfaces::internal;
        typedef iproto::iface iface_type;
        vtrc::unique_ptr<iface_type> i;
        try {
            i.reset( iproto::create( *info->client_core_ ) );
            i->exit_process( );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lua_call_connect( lua_State *L )
    {
        general_info *info = get_gen_info( L );

        lua::state ls(L);

        std::string server( ls.get_opt<std::string>( 1 ) );
        std::string id    ( ls.get_opt<std::string>( 2 ) );
        std::string key   ( ls.get_opt<std::string>( 3 ) );

        if( ls.get_top( ) > 2 && !key.empty( ) ) {
            key.assign( hash_key( id, key ) );
        }
        make_connect( info, server, id, key, true );

        return 0;
    }

    int lua_call_disconnect( lua_State *L )
    {
        general_info *info = get_gen_info( L );
        info->client_core_->disconnect( );
        return 0;
    }

    int global_init( general_info *info, bool connect )
    {
        lua::state ls(info->main_);
        objects::table_sptr fr_table(std::make_shared<objects::table>( ));

        //fr_table->add( "eval", new_function( &lcall_eval ) );
        fr_table->add( "exit", new_function( &lcall_exit ) );
        fr_table->add( "run",  new_function( &lcall_start_events ) );

        objects::table_sptr ct(new_table( ));

        if( connect ) {

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

            bool empty = key.empty( );
            key = hash_key( id, key );

            make_connect( info, server, id, empty ? "" : key, false );
            info->connected_ = true;
        }

        if( !info->connected_ ) {
            disconnect_table( info, *ct );
        } else {
            connect_table( info, *ct );
        }

        fr_table->add( "client", ct );
        fr_table->add( "print", new_function( &lcall_global_print ) );
        fr_table->add( "netifaces", new_function( &lcall_global_netifaces ) );

        ls.set_object( FR_CLIENT_LUA_MAIN_TABLE, fr_table.get( ) );

        if( info->connected_ ) {
            for( auto &m: info->modules_ ) {
                m->reinit( );
            }
        }

        return 0;
    }

    int lcall_import( lua_State *L )
    {
        lua::state ls(L);
        general_info *g = get_gen_info( L );

        auto path = ls.get_opt<std::string>( 1 );
        if( fs::exists( path ) ) {
            ls.check_call_error(ls.load_file( path.c_str( ) ));
        } else {
            fs::path scr(g->script_path_);
            scr.remove_leaf( );
            scr /= path;
            ls.check_call_error(ls.load_file( scr.string( ).c_str( ) ));

        }
        return 0;
    }

    int events_init( general_info *info )
    {
        info->general_events_.reset(
                    new lua::event_container( *info, events( ) ) );
        return 0;
    }

}}}
