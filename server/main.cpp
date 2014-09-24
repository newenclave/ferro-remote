#include <iostream>

#include "application.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "google/protobuf/descriptor.h"
#include "boost/program_options.hpp"

#include "subsys-list.hxx"

namespace vserver = vtrc::server;
namespace vcommon = vtrc::common;
using namespace fr;

namespace po = boost::program_options;

namespace {

    void fill_options( po::options_description &desc )
    {
        desc.add_options( )
            ("help,?",   "help message")
            ("server,s", po::value< std::vector< std::string> >( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("io-pool-size,i",  po::value<unsigned>( ),
                    "threads for io operations; default = 1")

            ("rpc-pool-size,r", po::value<unsigned>( ),
                    "threads for rpc calls; default = 1")

            ("only-pool,o", "use io pool for io operations and rpc calls")

            ;
    }

    po::variables_map create_cmd_params( int argc, const char **argv,
                                         po::options_description const desc )
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

    void init_subsystems( po::variables_map &vm, server::application &app )
    {
        using namespace server::subsys;

        app.add_subsystem<config>( vm );
        app.add_subsystem<os>( );
        app.add_subsystem<fs>( );
        app.add_subsystem<gpio>( );
        app.add_subsystem<reactor>( );

        app.add_subsystem<listeners>( );

        /// start all subsystems
        app.start_all( );
    }

    void usage( const po::options_description &dsc )
    {
        std::cout << "Usage: ferro_remote_server [options]\n"
                  << dsc << "\n";
    }

}

int main( int argc, const char **argv )
{
    po::options_description description("Allowed options");
    fill_options( description );

    po::variables_map vm;
    try {

        vm = ( create_cmd_params( argc, argv, description ) );

    } catch( const std::exception &ex ) {
        std::cerr << "Command line error: " << ex.what( ) << "\n";
        usage( description );
        return 1;
    }

    if( vm.count( "help" ) ) {
        usage( description );
        return 0;
    }

    try {

        bool use_only_pool = !!vm.count( "only-pool" );

        unsigned io_size = vm.count( "io-pool-size" )
                ? vm["io-pool-size"].as<unsigned>( )
                : ( use_only_pool ? 0 : 1 );

        unsigned rpc_size = vm.count( "rpc-pool-size" )
                ? vm["rpc-pool-size"].as<unsigned>( )
                : 1;

        if( (rpc_size < 1) && !use_only_pool) {
            throw std::runtime_error( "rpc-pool-size must be at least 1" );
        }

        vcommon::pool_pair pp( io_size, rpc_size );

        server::application app( pp );

        init_subsystems( vm, app );

        pp.get_io_pool( ).attach( ); /// RUN!

        pp.join_all( );

    } catch( const std::exception &ex ) {
        std::cerr << "Application failed: " << ex.what( ) << "\n";
        return 2;
    }

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
