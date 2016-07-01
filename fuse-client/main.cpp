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
            ("opt,o", po::value<std::vector<std::string> >( ),
                    "fuse: option for fuse client")
            ("foreground,f", "fuse: run foreground")
            ("debug,d", "fuse: show debug messages")
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

    std::vector<std::string> fuse_opts_holder;
    std::vector<char *> fuse_opts;

    try {

        fr::fuse::g_opts = ( create_cmd_params( argc, argv, description ) );

    } catch( const std::exception &ex ) {
        std::cerr << "Command line error: " << ex.what( ) << "\n";
        usage( description );
        return 1;
    }

    if( fr::fuse::g_opts.count( "help" ) ) {
        usage( description );
        char help[] = "--help";

        char *p[] = { argv[0], help };


        fuse_main( 2, &p[0], (fuse_operations *)NULL );

        return 0;
    }

    if( !fr::fuse::g_opts.count( "server" ) ) {
        usage( description );
        return 2;
    }

    if( fr::fuse::g_opts.count( "foreground" ) ) {
        fuse_opts_holder.push_back( "-f" );
    }

    if( fr::fuse::g_opts.count( "debug" ) ) {
        fuse_opts_holder.push_back( "-d" );
    }

    if( fr::fuse::g_opts.count( "opt" ) ) {
        auto opts = fr::fuse::g_opts["opt"].as<std::vector<std::string > >( );
        for( auto &o: opts ) {
            fuse_opts_holder.push_back( "-o" );
            fuse_opts_holder.push_back( o );
        }
    }

    if( !fr::fuse::g_opts.count( "point" ) ) {
        usage( description );
        return 1;
    } else {
        auto p = fr::fuse::g_opts["point"].as<std::string>( );
        fuse_opts_holder.push_back( p );
    }

    fuse_opts.push_back( argv[0] );

    for( auto &a: fuse_opts_holder ) {
        fuse_opts.push_back( &a[0] );
    }

    fuse_operations op = fr::fuse::application::set_operations( );
    fuse_main( fuse_opts.size( ), &fuse_opts[0], &op );

    return 0;
}


