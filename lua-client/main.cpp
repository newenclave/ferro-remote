#include <iostream>

#include "fr-lua.h"

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "vtrc-client/vtrc-client.h"

#include "boost/program_options.hpp"

#include "general-info.h"
#include "modules/mlist.hpp"

#include "utils.h"
#include "lua-names.h"
#include "main-client.h"

#include "boost/filesystem.hpp"
#include "boost/asio/io_service.hpp"

#include "3rd/struct.h"

#include "client-core/fr-discover.h"

#include "net-ifaces.h"

namespace vcomm     = vtrc::common;
namespace vclient   = vtrc::client;
namespace fs        = boost::filesystem;

namespace po = boost::program_options;
using namespace fr;

namespace {

    void show_help( po::options_description &description )
    {
        std::cout << description << "\n";
    }

    typedef std::vector<std::string> string_list;

    void fill_common_options( po::options_description &desc )
    {
        desc.add_options( )

            ("help,h",   "help message")

            ("script,e", po::value<std::string>( ), "path to script" )
            ( "main,m",  po::value<std::string>( ),
                         "main function; default = 'main'" )

            ("server,s", po::value<std::string>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("id", po::value<std::string>( ),
                    "set client ID for the remote agent")

            ("key", po::value<std::string>( ),
                    "set client KEY for the remote agent")

            ("param,p",  po::value<string_list>( ),
                    "parameters for script; -p\"name=value\"")
            ;
    }

    typedef std::pair<std::string, std::string> string_pair;

    string_pair split_string( std::string const &str, char delim = '=' )
    {
        size_t pos = str.find( delim );
        if( pos == std::string::npos ) {
            return std::make_pair( str, std::string( ) );
        } else {
            return std::make_pair(
                        std::string( str.begin( ), str.begin( ) + pos ),
                        std::string( str.begin( ) + pos + 1, str.end( ) ) );
        }
    }

    lua::objects::table_sptr create_params( po::variables_map const &vm )
    {
        lua::objects::table_sptr t(lua::objects::new_table( ));
        if( vm.count( "param" ) ) {
            string_list par = vm["param"].as<string_list>( );
            for( auto &p: par ) {
                string_pair sp = split_string( p );
                if( sp.second.empty( ) ) {
                    t->add( lua::objects::new_string( sp.first ) );
                } else {
                    t->add( lua::objects::new_string( sp.first ),
                            lua::objects::new_string( sp.second ) );
                }
            }
        }
        return t;
    }
}

int main( int argc, const char *argv[] )
{ try {

    po::options_description description("Allowed common options");

    fill_common_options( description );

    po::variables_map vm;
    int res = 0;

    try {
        po::options_description desc = description;
        po::parsed_options parsed (
            po::command_line_parser( argc, argv )
                .options(desc)
                .run( ));
        po::store(parsed, vm);
    } catch ( const std::exception &ex ) {
        std::cerr << "Options error: " << ex.what( )
                  << "\n";
        show_help( description );
        return 1;
    }

    if( !vm.count( "script" ) ) {
        std::cerr << "script name isn't specified.\n";
        show_help( description );
        return 2;
    }

    lua::state ls;

    std::string script_path( vm["script"].as<std::string>( ) );

    fs::path sp( script_path );
    ls.set( FR_CLIENT_WD_PATH, sp.parent_path( ).string( ));

    bool custom_main = !!vm.count( "main" );
    std::string main_name( custom_main
                        ?  vm["main"].as<std::string>( )
                        :  std::string( "main" ) );

    lua::objects::base_sptr par = create_params( vm );


    do {

        lua::client::general_info ci;
        vcomm::pool_pair pp( 1, 1 );
        vcomm::thread_pool et( 0 );

        ci.main_    = ls.get_state( );
        ci.pp_      = &pp;
        ci.tp_      = &et;
        ci.eventor_ = std::make_shared<lua::event_caller>(
                                        ci.main_,
                                        std::ref(et.get_io_service( ) ) );

        ci.eventor_->set_enable( false );
        //ci.client_core_
        ci.cmd_opts_.swap( vm );

        ci.modules_ = lua::m::create_all( ci );

        ls.openlibs( );
        ls.openlib( "struct", luaopen_struct, 1 );
        ls.register_call( "import", &::fr::lua::client::lcall_import );

        //luaopen_struct( ls.get_state( ) );
        ls.set( FR_CLIENT_GEN_INFO_PATH, &ci );

        try {

            lua::client::events_init( &ci );
            lua::client::global_init( &ci, ci.cmd_opts_.count( "server" ) > 0 );

            for( auto &m: ci.modules_ ) {
                m->init( );
            }

            if( fs::exists( script_path ) ) {
                ci.script_path_ = script_path;
                ls.check_call_error( ls.load_file( script_path.c_str( ) ) );
            } else {
                ls.check_call_error( ls.load_buffer( script_path.c_str( ),
                                                     script_path.size( ),
                                                     "cmd" ) );
            }

            if( ls.exists( main_name.c_str( ) ) ) {
                int res = ls.exec_function( main_name.c_str( ), *par );
                ls.check_call_error( res );

                if( ci.eventor_->get_enable( ) ) {
#if 0 //// test
                    auto ni = utilities::get_system_ifaces( );
                    for( auto &n: ni ) {
                        std::cout << n << "\n";
                    }
                    auto pinger = client::core::create_udp_pinger( et.get_io_service( ) );
                    auto hdl = [&et]( const boost::system::error_code &e,
                                  const client::core::udp_responce_info *inf )
                    {
                        if( e ) {
                            et.stop( );
                            return false;
                        } else {
                            std::cout << inf->length << "\n";
                            std::cout << std::string( inf->data, inf->length )
                                      << std::endl;
                            std::cout << inf->from->address( ).to_string( )
                                      << std::endl;

                        }
                        return true;
                    };
                    pinger->async_ping( "ff08::13:18853", 2000, hdl );
#endif
                    et.attach( );
                }

            } else if( custom_main ) {
                std::cerr << "Function '" << main_name << "'"
                          << " was not found in the script.\n";
                res = 4;
            }
        } catch( const std::exception &ex ) {
            std::cerr << "Execution failed: " << ex.what( ) << "\n";
            res = 5;
        }

        lua_gc( ls.get_state( ), LUA_GCCOLLECT, 0 );

        for( auto &m: ci.modules_ ) {
            m->deinit( );
        }

        res = ci.exit_code_;

        ci.client_core_.reset( );

        pp.stop_all( );
        pp.join_all( );

        ci.modules_.clear( );

    } while(0);

    google::protobuf::ShutdownProtobufLibrary( );
    return res;

} catch( const std::exception &ex ) {
    std::cerr << "General error: " << ex.what( ) << "\n";
    return 3;
}}

