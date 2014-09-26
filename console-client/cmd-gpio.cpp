
#include <iostream>
#include "interfaces/IGPIO.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "vtrc-memory.h"
#include "vtrc-chrono.h"
#include "vtrc-bind.h"

#ifdef _MSC_VER
#include <windows.h>
#define sleep_ Sleep /// milliseconds
#define MILLISECONDS( x ) x
#else
#include <unistd.h>
#define sleep_ usleep /// microseconds
#define MILLISECONDS( x ) ((x) * 1000)
#endif

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


            void event_cb( unsigned err, unsigned gpio, unsigned val )
            {
                if( !err ) {
                    std::cout << "New value for "
                              << gpio << " = " << val << "\n";
                } else {
                    std::cout << "Got " << err << " as error\n";
                }
            }

            void exec( po::variables_map &vm, core::client_core &client )
            {


                if( vm.count( "wait" ) ) {

                    unsigned g(vm["wait"].as<unsigned>( ));
                    unsigned to = vm.count( "timeout" )
                                ? vm["timeout"].as<unsigned>( )
                                : 0;
                    vtrc::unique_ptr<igpio::iface> ptr
                                        ( igpio::create( client, g ) );

                    igpio::value_change_callback cb(vtrc::bind(
                                        &impl::event_cb, this,
                                        vtrc::placeholders::_1,
                                        vtrc::placeholders::_2,
                                        g ));

                    ptr->export_device( );
                    ptr->set_direction( igpio::DIRECT_IN );
                    ptr->set_edge( igpio::EDGE_BOTH );
                    ptr->register_for_change( cb );

                    sleep( to );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
                ///
                desc.add_options( )
                    ("wait,w", po::value<unsigned>( ), "wait gpio event")
                    ("timeout,t", po::value<unsigned>( ), "timeout for 'wait'"
                                                              "; seconds")
                    ;
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

    
