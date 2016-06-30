#include <iostream>
#include <string>

#include "boost/program_options.hpp"

#include "application.h"

namespace po = boost::program_options;

namespace {

    void all_options( po::options_description &desc )
    {
        desc.add_options( )
            ("help,?",   "help message")

            ("server,s", po::value<std::string>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("point,m", po::value<std::string>( ),
                    "mountpoint name for fuse directory")
                ;
    }

    po::variables_map create_cmd_params( int argc, char **argv,
                                         po::options_description const &desc )
    {
        po::variables_map vm;
        po::parsed_options parsed (
            po::command_line_parser(argc, argv)
                .options(desc)
                //.allow_unregistered( )
                .run( ));
        po::store( parsed, vm );
        po::notify( vm);
        return vm;
    }

    void usage( const po::options_description &dsc )
    {
        std::cout << "Usage: frfuse_client [options]\n"
                  << dsc << "\n";
    }
}

int main( int argc, char **argv )
{
    po::options_description description("Allowed options");
    all_options( description );

    try {

        fr::fuse::g_opts = ( create_cmd_params( argc, argv, description ) );

    } catch( const std::exception &ex ) {
        std::cerr << "Command line error: " << ex.what( ) << "\n";
        usage( description );
        return 1;
    }

    if( fr::fuse::g_opts.count( "help" ) ) {
        usage( description );
        return 0;
    }

    if( !fr::fuse::g_opts.count( "point" ) ) {
        usage( description );
        return 1;
    }

    if( !fr::fuse::g_opts.count( "server" ) ) {
        usage( description );
        return 2;
    }

    auto point = fr::fuse::g_opts["point"].as<std::string>( );

    char *params[] = { argv[0], &point[0] };

    fuse_operations op = fr::fuse::application::set_operations( );
    fuse_main( 2, params, &op );

    return 0;
}


