#include <iostream>
#include <iomanip>

#include "client-core/fr-client.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

namespace po = boost::program_options;

namespace {

    typedef std::map<std::string, fr::cc::command_sptr> command_map;

    void fill_common_options( po::options_description &desc )
    {
        desc.add_options( )

            ("help,h",   "help message")

            ( "command,c", po::value<std::string>( ), "command to exec" )

            ("server,s", po::value<std::string>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("io-pool-size,i",  po::value<unsigned>( ),
                    "threads for io operations; default = 1")

            ("rpc-pool-size,r", po::value<unsigned>( ),
                    "threads for rpc calls; default = 1")

            ;
    }

    po::variables_map parse_common( int argc, const char **argv,
                                    const po::options_description &desc )
    {
        po::variables_map vm;
        po::parsed_options parsed (
            po::command_line_parser( argc, argv )
                .options(desc)
                .allow_unregistered( )
                .run( ));
        po::store(parsed, vm);
        po::notify(vm);
        return vm;
    }

    void show_init_help( const po::options_description &desc,
                         const command_map &cm)
    {
        std::cout << "Usage: ferro_remote_client -c <command> [command opts]\n"
            << "      'ferro_remote_client -h' for this help\n"
            << "      'ferro_remote_client -c <command> -h'"
            << " for command's help\n"
            << desc << "\n";

        std::cout << "Commands (" << cm.size( ) << " total)" << ": \n";

        for( command_map::const_iterator b(cm.begin( )),
             e(cm.end( )); b!=e; ++b )
        {
            std::cout << "\t" << std::setw(20) << b->first
                      << ": " << b->second->desc( )
                      << "\n";
        }
    }

    void show_command_help( fr::cc::command_sptr &command )
    {
        po::options_description desc;
        command->add_options( desc );

        std::cout << "Usage: ferro_remote_client -c " << command->name( )
                  << " [opts]\n"
                  << "Options:\n" << desc << "\n";
    }
}

int main( int argc, const char **argv )
{    
    po::options_description desc("Allowed options");
    fill_common_options( desc );
    po::variables_map vm( parse_common( argc, argv, desc ) );

    command_map cm;

    if( vm.count("help") ) {
        show_init_help( desc, cm );
        return 1;
    }

    return 0;
}
