
#include <iostream>
#include <vector>

#include "interfaces/IGPIO.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "vtrc-memory.h"
#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-stdint.h"

#ifdef _MSC_VER
#include <windows.h>
#define sleep_( x ) Sleep( (x) * 1000 ) /// milliseconds
#else
#include <unistd.h>
#define sleep_( x ) sleep( x ) /// seconds
#endif

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;

        typedef vtrc::chrono::high_resolution_clock::time_point time_point;
        namespace igpio = client::interfaces::gpio;

        const char *cmd_name = "gpio";

        struct impl: public command_iface {


            typedef vtrc::shared_ptr<igpio::iface> iface_sptr;
            typedef std::vector<iface_sptr> iface_list;

            const char *name( ) const
            {
                return cmd_name;
            }

            static void event_cb( unsigned err, unsigned val,
                                  iface_list &out, unsigned gpio,
                                  unsigned bin )
            {
                if( !err ) {
                    if( 0 == bin ) {
                        std::cout << "New value for "
                                  << gpio << " = " << val << "\n";
                    } else {
                        char data[2] = {
                             static_cast<char>( gpio )
                            ,static_cast<char>( val  )
                        };

                        //(std::cout << std::hex << data << "-").flush( );
                        std::cout.write( data, sizeof(data) ).flush( );
                    }

                    for( iface_list::iterator b(out.begin( )), e(out.end( ));
                         b != e; ++b )
                    {
                        (*b)->set_value( val );
                    }

                } else {
                    std::cout << "Got " << err << " as error\n";
                }
            }

            static iface_list get_outputs( po::variables_map &vm,
                                           core::client_core &cli )
            {
                iface_list outputs;

                if( vm.count( "out" ) ) {
                    typedef std::vector<unsigned> vec;
                    vec out = vm["out"].as<std::vector<unsigned> >( );
                    for( vec::iterator b(out.begin( )), e(out.end());
                         b!=e; ++b )
                    {
                        iface_sptr out( igpio::create_output( cli, *b ) );
                        outputs.push_back( out );
                    }
                }
                return outputs;
            }

            void exec( po::variables_map &vm, core::client_core &cli )
            {

                unsigned binary = !!vm.count( "bin" );

                if( vm.count( "in" ) ) {

                    unsigned inp(vm["in"].as<unsigned>( ));

                    vtrc::uint64_t to = vm.count( "timeout" )
                                      ? vm["timeout"].as<vtrc::uint64_t>( )
                                      : 0;

                    iface_list outputs(get_outputs( vm, cli ));

                    vtrc::unique_ptr<igpio::iface> ptr
                                        ( igpio::create( cli, inp ) );

                    ptr->export_device( );
                    ptr->set_direction( igpio::DIRECT_IN );
                    ptr->set_edge( igpio::EDGE_BOTH );

                    igpio::value_change_callback cb(vtrc::bind(
                                        &impl::event_cb,
                                        vtrc::placeholders::_1,
                                        vtrc::placeholders::_2,
                                        vtrc::ref(outputs),
                                        inp, binary ));

                    ptr->register_for_change( cb );

                    sleep_( to );

                    ptr->unregister( );

                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r"
                ///
                desc.add_options( )
                    ("in,I", po::value<unsigned>( ), "wait input from...")
                    ("out,O", po::value<std::vector<unsigned> >( ),
                                                     "set output from IN")
                    ("bin,B", "set binary output."
                              " format is: <gpio:1><value:1>")
                    ("timeout,t", po::value<vtrc::uint64_t>( ),
                              "timeout for 'wait'; seconds")
                    ;
            }

            std::string help( ) const
            {
                return std::string( "" );
            }

            std::string desc( ) const
            {
                return std::string( "gpio command" );
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

    
