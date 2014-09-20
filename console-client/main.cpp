#include <iostream>
#include <iomanip>

#include "client-core/fr-client.h"
#include "cmd-list.h"

#include "boost/program_options.hpp"
#include "boost/system/error_code.hpp"

namespace po = boost::program_options;

namespace vcommon = vtrc::common;

namespace {

    typedef std::map<std::string, fr::cc::command_sptr> command_map;
    typedef vtrc::shared_ptr<vcommon::pool_pair> pool_pair_sptr;

    command_map::value_type cmd( fr::cc::command_sptr ptr )
    {
        return std::make_pair( ptr->name( ), ptr );
    }

    void init_map( command_map &m )
    {
        m.insert( cmd( fr::cc::cmd::fs::create( ) ) );
        m.insert( cmd( fr::cc::cmd::gpio::create( ) ) );
    }

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

            ("only-pool,o", "use io pool for io operations and rpc calls")

            ;
    }

    pool_pair_sptr pp_from_cmd( const po::variables_map &vm )
    {
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

        if( use_only_pool ) {
            return vtrc::make_shared<vcommon::pool_pair>( io_size );
        } else {
            return vtrc::make_shared<vcommon::pool_pair>( io_size, rpc_size );
        }
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

    po::variables_map parse_command( int argc, const char **argv,
                                     fr::cc::command_sptr command,
                                     po::options_description &desc )
    {
        command->add_options( desc );
        po::variables_map vm;
        po::parsed_options parsed (
            po::command_line_parser( argc, argv )
                .options(desc)
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
            std::cout << "\t" << b->first
                      << ": " << b->second->desc( )
                      << "\n";
        }
    }

    void show_command_help( const po::options_description &cdesc,
                            fr::cc::command_sptr &command )
    {
        po::options_description desc;
        command->add_options( desc );

        std::cout << cdesc << "\n";
        std::cout << "Usage: ferro_remote_client -c " << command->name( )
                  << " [opts]\n"
                  << "Command options: \n" << desc << "\n";
    }


    void connect_success( boost::system::error_code const &err )
    {
        std::cout << err.message( ) << "\n";
    }
}

int main( int argc, const char **argv ) try
{    
    po::options_description desc("Common options");
    fill_common_options( desc );
    po::variables_map vm( parse_common( argc, argv, desc ) );

    command_map cm;

    init_map( cm );

    fr::cc::command_sptr current_command;

    if( vm.count( "command" ) ) {
        std::string c(vm["command"].as<std::string>( ));
        command_map::iterator f(cm.find( c ));
        if( f == cm.end( ) ) {
            std::cout << "Invalid command '" << c << "'\n";
            show_init_help( desc, cm );
            return 1;
        } else {
            current_command = f->second;
        }
    }

    if( vm.count("help") ) {
        current_command.get( ) ? show_command_help( desc, current_command )
                               : show_init_help( desc, cm );
        return 0;
    }

    if( !current_command.get( ) ) {
        std::cout << "Invalid command '<empty>'\n";
        show_init_help( desc, cm );
        return 2;
    }

    std::string server( vm.count("server")
                        ? vm["server"].as<std::string>( )
                        : "");

    if( server.empty( ) ) {
        std::cout << "Invalid server '<empty>'\n";
        show_init_help( desc, cm );
        return 3;
    }


    pool_pair_sptr pp(pp_from_cmd( vm ));

    fr::client::core::client_core client(*pp);

    client.connect( server );

    vm = parse_command( argc, argv, current_command, desc );

    //client.async_connect( server, connect_success );
    client.disconnect(  );

    current_command->exec( vm, client );

    pp->get_io_pool( ).attach( );

    return 0;

} catch( const std::exception &ex ) {
    std::cerr << "General client error: " << ex.what( ) << "\n";
    return 10;
}
