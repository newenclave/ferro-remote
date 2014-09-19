#include <iostream>

#include "application.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "google/protobuf/descriptor.h"
#include "boost/program_options.hpp"

namespace vserver = vtrc::server;
namespace vcommon = vtrc::common;
using namespace fr;

namespace po = boost::program_options;

namespace {

    void fill_options( po::options_description &desc )
    {
        desc.add_options( )
            ("help,?",   "help message")
            ("server,s", po::value<std::vector< std::string> >( ),
                    "endpoint name; <tcp address>:<port> or <pipe/file name>")
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

}

int main( int argc, const char **argv )
{
    po::options_description description("Allowed options");
    fill_options( description );

    po::variables_map vm( create_cmd_params( argc, argv, description ) );

    if( vm.count( "help" ) ) {
        std::cout << "Usage: " << description << "\n";
        return 0;
    }

    vcommon::pool_pair pp( 0 );
    server::application app( pp );

    pp.stop_all( );
    pp.join_all( );

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
