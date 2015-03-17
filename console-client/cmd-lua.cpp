
#include "command-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include <iostream>
#include <stdio.h>
#include "interfaces/IOS.h"

#define LUA_WRAPPER_TOP_NAMESPACE fr

#include "client-core/fr-client.h"

#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

#include "boost/program_options.hpp"

#include "lua-interface.h"

#ifdef _MSC_VER
#include <windows.h>
#define sleep_( x ) Sleep( (x) * 1000 ) /// milliseconds
#else
#include <unistd.h>
#define sleep_( x ) sleep( x ) /// seconds
#endif

#include "vtrc-chrono.h"

namespace fr { namespace lua {

    namespace names {
        const char * main_table       = "fr" ;
        const char * client_table     = "fr.client";
        const char * connect_table    = "fr.client.connect";
        const char * disconnect_table = "fr.client.disconnect";
        const char * core_path        = "fr.client.core";
        const char * server_path      = "fr.client.server";
        const char * inst_field       = "__i";
    }
}}

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;
        namespace lo = fr::lua::objects;

        typedef fr::lua::state lua_state;

        const char *cmd_name = "lua";

        typedef client::interfaces::os::iface os_iface;
        typedef std::shared_ptr<os_iface> os_iface_sptr;

        void general_init( const std::string &server,
                           fr::lua::core_data &cd );

        void set_client_info(fr::lua::core_data &cd );

        int global_print_impl( lua_State *L, bool as_integer )
        {
            int n = lua_gettop( L );

            lua::state lv( L );

            if( n < 0 ) {
                n = ( (n * -1) + 1 );
            }

            for ( int b = 1; b <= n; ++b ) {
                std::cout << lv.get_object( b, as_integer )->str( );
            }
            lv.pop( n );

            std::cout.flush( );
            return 0;
        }

        int global_println_impl( lua_State *L, bool as_integer )
        {
            int res = global_print_impl( L, as_integer );
            std::cout << "\n";
            return res;
        }

        int global_print( lua_State *L )
        {
            return global_print_impl( L, false );
        }

        int global_println( lua_State *L )
        {
            return global_println_impl( L, false );
        }

        int global_printi( lua_State *L )
        {
            return global_print_impl( L, true );
        }

        int global_printiln( lua_State *L )
        {
            return global_println_impl( L, true );
        }

        int global_throw( lua_State *L )
        {
            lua::state lv( L );
            int r = lv.get_top( );
            std::string except( "throw from script" );
            if( r ) try {
                except = lv.get<std::string>( );
            } catch( ... ) {
                ;;;
            }
            throw std::runtime_error( except );
        }

        int global_open( lua_State *L )
        {
            lua::state lv( L );
            int r = lv.get_top( );
            int loaded = 0;
            for( int i=1; i<=r; ++i ) {
                try {
                    const char *libname = lv.get<const char *>( i );
                    int res = lv.openlib( libname );
                    if( !res ) {
                        lv.load_file( libname );
                    }
                } catch( ... ) {
                    ;;;
                }
            }
            lv.pop( r );
            lv.push( loaded );
            return 1;
        }

        int global_sleep( lua_State *L )
        {
            lua::state lv( L );
            int r = lv.get_top( );
            for( int i=1; i<=r; ++i ) {
                try {
                    unsigned seconds = lv.get<unsigned>( i );
                    sleep_( seconds );
                } catch( ... ) {
                    ;;;
                }
            }
            lv.pop( r );
            return 0;
        }

        int global_hr_clock( lua_State *L )
        {
            lua::state ls( L );

            using namespace vtrc::chrono;

            high_resolution_clock::time_point
                    value(high_resolution_clock::now( ));

            ls.push( value.time_since_epoch( ).count( ) );

            return 1;
        }

        int global_connect( lua_State *L )
        {
            lua::state ls( L );
            lua::core_data *cd = lua::get_core( L );
            std::string server = ls.get<std::string>( );

            bool success = true;

            try {
                cd->core_->disconnect( );
                ls.set( lua::names::client_table ); // set to nil
                cd->core_->connect( server );
                general_init( server, *cd );
                ls.push( success );
                ls.push( );
            } catch( const std::exception &ex ) {
                success = false;
                ls.push( ex.what( ) );
            }

            set_client_info( *cd );

            return 2;
        }

        int global_disconnect( lua_State *L )
        {
            lua::state ls( L );
            lua::core_data *cd = lua::get_core( L );
            cd->core_->disconnect( );
            ls.set( lua::names::client_table ); // set to nil
            set_client_info( *cd );
            return 0;
        }

#define FR_INTERFACE_PAIR( ns, L, cl )      \
    std::make_pair( lua::ns::table_name( ), lua::ns::init( L, cl ))

        void init( lua::core_data &cd )
        {
            std::map<std::string, lua::data_sptr> tmp;

            typedef lua::core_data::table_map::const_iterator citr;

            tmp.insert( FR_INTERFACE_PAIR( os,   cd.state_, *cd.core_ ) );
            tmp.insert( FR_INTERFACE_PAIR( fs,   cd.state_, *cd.core_ ) );
            tmp.insert( FR_INTERFACE_PAIR( gpio, cd.state_, *cd.core_ ) );
            tmp.insert( FR_INTERFACE_PAIR( i2c,  cd.state_, *cd.core_ ) );

            lo::table_sptr client_table( lo::new_table( ) );

            for( citr b(tmp.begin( )), e(tmp.end( )); b!=e; ++b ) {
                client_table->add(
                    lo::new_string( b->first ),
                    b->second->init_table( )
                );
            }

            cd.state_->set_object( lua::names::client_table,
                                   client_table.get( ) );
            cd.tables_.swap( tmp );

        }

        void general_init( const std::string &server,
                           lua::core_data &cd )
        {
            init( cd );
            cd.state_->set( lua::names::server_path, server );
        }

#undef FR_INTERFACE_PAIR

        void register_globals( lua_State *L )
        {
            lua_state ls( L );

            ls.register_call( "print",          &global_print );
            ls.register_call( "println",        &global_println );
            ls.register_call( "printi",         &global_printi );
            ls.register_call( "printiln",       &global_printiln );
            ls.register_call( "die",            &global_throw );
            ls.register_call( "open",           &global_open );
            ls.register_call( "sleep",          &global_sleep );
            ls.register_call( "hight_clock",    &global_hr_clock );
        }

        void set_client_info( fr::lua::core_data &cd )
        {
            lua::set_core( cd.state_->get_state( ), &cd );

            lo::function connect_func(    &global_connect );
            lo::function disconnect_func( &global_disconnect );

            cd.state_->set_object( lua::names::connect_table,
                                   &connect_func );

            cd.state_->set_object( lua::names::disconnect_table,
                                   &disconnect_func );
        }

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            typedef std::pair<std::string, std::string> string_pair;
            typedef std::string::const_iterator citr;

            string_pair split_str( std::string const &s)
            {
                for( citr b(s.begin( )), e(s.end( )); b != e; ++b) {
                    if( *b == '=' ) {
                        return std::make_pair( std::string( s.begin( ), b ),
                                               std::string( b + 1, s.end( ) ) );
                    }
                }
                return std::make_pair( std::string( ), s );
            }

            lo::base_sptr create_params( po::variables_map &vm )
            {
                typedef std::vector<std::string> strings;
                lo::table_sptr t(lo::new_table( ));
                if( vm.count( "param" ) ) {
                    strings p( vm["param"].as<strings>( ) );
                    for( strings::const_iterator b(p.begin( )), e(p.end( ));
                         b != e; ++b)
                    {
                        string_pair ss( split_str( *b ) );
                        if( ss.first.empty( ) ) {
                            t->add( lo::new_string( ss.second ) );
                        } else {
                            t->add( lo::new_string( ss.first ),
                                    lo::new_string( ss.second ) );
                        }
                    }
                }
                return t;
            }

            std::string load_file( const std::string &path )
            {
                FILE *f = fopen( path.c_str( ), "rb" );
                std::string res;
                if( f ) {
                    std::vector<char> tmp(4096);
                    while(size_t len = fread( &tmp[0], 1, tmp.size( ), f )) {
                        res.append( tmp.begin( ), tmp.begin( ) + len );
                    }
                    fclose( f );
                }
                return res;
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                lua::core_data cd;
                cd.core_ = &cl;
                cd.state_ = std::make_shared<fr::lua::state>( );

                register_globals( cd.state_->get_state( ) );

                std::string server(   vm.count("server")
                                    ? vm["server"].as<std::string>( ) : "" );

                if( !server.empty( ) ) {
                    cl.connect( server );
                    general_init( server, cd );
                }

                set_client_info( cd );

                std::string main_function("main");
                bool custom_main = false;

                if( vm.count( "main" ) )  {
                    main_function.assign( vm["main"].as<std::string>( ) );
                    custom_main = true;
                }

                std::shared_ptr<lua_state> lv = cd.state_;

                if( vm.count( "exec" ) ) {

                    vtrc::common::thread_pool eventor(0);
                    cd.event_thread_ = &eventor;

                    int res = 0;

                    std::string script( vm["exec"].as<std::string>( ) );

                    lv->check_call_error( lv->load_file(script.c_str( ) ) );

                    lo::base_sptr par = create_params( vm );

                    if( lv->exists( main_function.c_str( ) ) ) {
                        int res = lv->exec_function( main_function.c_str( ),
                                                     *par );
                        lv->check_call_error( res );
                    } else if( custom_main ) {
                        std::cout << "Function '" << main_function << "'"
                                  << " was not found in the script.\n";
                        res = 1;
                    }
//                    if( 0 == res ) {
//                        eventor.attach( );
//                    }
                }
                //cl.disconnect( );
            }

            void add_options( po::options_description &desc )
            {
                /// reserved as common
                /// "help,h"
                /// "command,c"
                /// "server,s"
                /// "io-pool-size,i"
                /// "rpc-pool-size,r"
                desc.add_options( )
                    ( "exec,e", po::value<std::string>( ),
                                              "execute the script" )
                    ( "main,m", po::value<std::string>( ),
                                            "main function; defailt = 'main'" )
                    ( "param,p", po::value< std::vector<std::string> >( ),
                                              "params for call" )
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "lua command" );
            }

            bool need_connect( ) const
            {
                return false;
            }

        };
    }

    namespace lua {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

#endif
    
