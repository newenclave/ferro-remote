
#include <iostream>
#include "interfaces/IGPIO.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "vtrc-memory.h"
#include "vtrc-chrono.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;

        typedef vtrc::chrono::high_resolution_clock::time_point time_point;
        namespace igpio = client::interfaces::gpio;

        const char *cmd_name = "gpio";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &client )
            {
                vtrc::unique_ptr<igpio::iface> ptr
                                    ( igpio::create( client, 3 ) );

                std::cout << "1\n";
                ptr->export_device( );
                std::cout << "2\n";
                ptr->set_direction( igpio::DIRECT_IN );
                std::cout << "3\n";
                ptr->set_edge( igpio::EDGE_BOTH );
                std::cout << "4 "
                          << " " << ptr->value( )
                          << " " << ptr->direction( )
                          << "\n";
                ptr->register_for_change( );
                std::cout << "5\n";

                sleep( 30 );
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
//                desc.add_options( )
//                    ("path,p", po::value<std::string>( ), "path to file")
//                    ;
            }

            std::string help( ) const
            {
                return std::string( "" );
            }

            std::string desc( ) const
            {
                return std::string( "fs command" );
            }

        };
    }

    namespace gpio {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

    
