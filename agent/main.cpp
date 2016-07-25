#include <iostream>

#include "application.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "google/protobuf/descriptor.h"
#include "boost/program_options.hpp"

#include "subsys-list.hxx"

#include "ferro-remote-config.h"

namespace vserver = vtrc::server;
namespace vcommon = vtrc::common;
using namespace fr;

namespace po = boost::program_options;

namespace {

    void fill_options( po::options_description &desc )
    {
        agent::subsys::config::all_options( desc );
    }

    po::variables_map create_cmd_params( int argc, const char **argv,
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

    void init_subsystems( po::variables_map &vm, agent::application &app )
    {
        using namespace agent::subsys;

        std::vector<std::string> loggers;

        if( vm.count( "log" ) ) {
            loggers = vm["log"].as<decltype(loggers)>( );
        }

        app.add_subsystem<config>( vm );
        app.add_subsystem<logging>( loggers );

#if FR_WITH_LUA
        app.add_subsystem<lua>( );
#endif
        app.add_subsystem<os>( );
        app.add_subsystem<fs>( );
        app.add_subsystem<i2c>( );
        app.add_subsystem<spi>( );
        app.add_subsystem<gpio>( );
        app.add_subsystem<reactor>( );
        app.add_subsystem<multicast>( );

        app.add_subsystem<listeners>( );

        /// start all subsystems
        app.start_all( );
    }

    void usage( const po::options_description &dsc )
    {
        std::cout << "Usage: ferro_remote_agent [options]\n"
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
                : 0;

        unsigned rpc_size = vm.count( "rpc-pool-size" )
                ? vm["rpc-pool-size"].as<unsigned>( )
                : 1;

        if( (rpc_size < 1) && !use_only_pool) {
            throw std::runtime_error( "rpc-pool-size must be at least 1" );
        }

        vtrc::unique_ptr<vcommon::pool_pair> pp;

        if( use_only_pool ) {
            pp.reset( new vcommon::pool_pair( io_size ) );
        } else {
            pp.reset( new vcommon::pool_pair(io_size, rpc_size) );
        }

        agent::application app( *pp );

        init_subsystems( vm, app );

        agent::logger &lgger( app.get_logger( ) );

        lgger(agent::logger::level::info) << "[main] Agent started.";

        pp->get_io_pool( ).attach( ); /// RUN!
        pp->join_all( );

        lgger(agent::logger::level::info) << "[main] Agent stopped.";

    } catch( const std::exception &ex ) {
        std::cerr << "Application failed: " << ex.what( ) << "\n";
        return 2;
    }

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
