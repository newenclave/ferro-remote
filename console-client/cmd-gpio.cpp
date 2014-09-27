
#include <iostream>
#include <vector>

#include "interfaces/IGPIO.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "vtrc-memory.h"
#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-stdint.h"

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
                        vtrc::int32_t data;

                        data = static_cast<vtrc::uint16_t>( gpio ) << 16
                             | static_cast<vtrc::uint16_t>( val  );

                        //(std::cout << std::hex << data << "-").flush( );
                        std::cout.write( &data, sizeof(data) ).flush( );
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

                    unsigned to = vm.count( "timeout" )
                                ? vm["timeout"].as<unsigned>( )
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
                                        outputs,
                                        inp, binary ));

                    ptr->register_for_change( cb );

                    sleep_( MILLISECONDS(to) * 1000 );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
                ///
                desc.add_options( )
                    ("in,I", po::value<unsigned>( ), "wait input from...")
                    ("out,O", po::value<std::vector<unsigned> >( ),
                                                     "set output from IN")
                    ("bin,B", "set binary output."
                              " format is: <gpionumber:2bytes><value:2bytes>")
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

    
