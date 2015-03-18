#include <iostream>

#include "fr-lua.h"

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "vtrc-client/vtrc-client.h"

#include "boost/program_options.hpp"

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
{
    po::options_description description("Allowed common options");

    fill_common_options( description );

    vcomm::pool_pair pp( 1, 1 );

    po::variables_map vm;

    try {
        po::options_description desc = description;
        po::parsed_options parsed (
            po::command_line_parser( argc, argv )
                .options(desc)
                .allow_unregistered( )
                .run( ));
        po::store(parsed, vm);
    } catch ( const std::exception &ex ) {
        std::cerr << "Options error: " << ex.what( )
                  << "\n";
        show_help( description );
        return 1;
    }

    lua::state general_state;

    return 0;
}