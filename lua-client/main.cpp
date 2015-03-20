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

namespace vcomm     = vtrc::common;
namespace vclient   = vtrc::client;

namespace po = boost::program_options;
using namespace fr;

namespace {

    void show_help( po::options_description &description )
    {
        std::cout << description << "\n";
    }

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
            ;
    }

}

int main( int argc, const char *argv[] )
{ try {
    po::options_description description("Allowed common options");

    fill_common_options( description );

    vcomm::pool_pair pp( 1, 1 );

    po::variables_map vm;

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

    lua::state general_state;

    do {

        vcomm::thread_pool et( 0 );
        lua::client::general_info ci;

        ci.main_    = general_state.get_state( );
        ci.pp_      = &pp;
        ci.eventor_ = std::make_shared<lua::event_caller>(
                                        ci.main_,
                                        std::ref(et.get_io_service( ) ) );

        //ci.client_core_
        ci.cmd_opts_.swap( vm );

        ci.modules_ = lua::client::m::create_all( ci );

        general_state.set( FR_CLIENT_GEN_INFO_PATH, &ci );

//        for( auto &m: ci.modules_ ) {
//            m->init( );
//        }

        et.attach( );

        return ci.exit_code_;

    } while(0);

    return 0;

} catch( const std::exception &ex ) {
    std::cerr << "General error: " << ex.what( ) << "\n";
    return 3;
}}

