#include <iostream>

#include "interfaces/IFile.h"
#include "interfaces/IFilesystem.h"

#include "command-iface.h"

#include "boost/program_options.hpp"

#include "interfaces/IOS.h"
#include "vtrc-common/vtrc-exception.h"
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

        namespace po    = boost::program_options;
        namespace core  = client::core;
        namespace fs    = client::interfaces::filesystem;
        namespace fsf   = client::interfaces::file;

        const char *cmd_name = "fs";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void event_cb( unsigned err, const std::string &data )
            {
                if( !err ) {
                    std::cout << "Got " << data.size( ) << " bytes of data\n";
                } else {
                    std::cout << "Got " << err << " as error\n";
                }
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                if( vm.count( "list" ) ) {
                    std::string p(vm["list"].as<std::string>( ));
                    vtrc::unique_ptr<fs::iface> i( fs::create( cl, p ) );
                    for( auto &d: *i ) {
                        std::cout << d.path << "\n";
                    }
                    std::cout << "\n";
                }

                if( vm.count( "wait" ) ) {
                    std::string p(vm["wait"].as<std::string>( ));

                    unsigned to = vm.count( "timeout" )
                                ? vm["timeout"].as<unsigned>( )
                                : 0;

                    vtrc::unique_ptr<fsf::iface> i(
                                fsf::create( cl, p, fsf::flags::RDONLY ) );

                    fsf::file_event_callback cb(vtrc::bind(
                                                    &impl::event_cb, this,
                                                    vtrc::placeholders::_1,
                                                    vtrc::placeholders::_2 ));
                    i->register_for_events( cb );

                    sleep_( MILLISECONDS(to) * 1000 );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
                desc.add_options( )
                    ("list,l", po::value<std::string>( ), "show directory list")
                    ("wait,w", po::value<std::string>( ), "wait file event")
                    ("timeout,t", po::value<unsigned>( ), "timeout for 'wait'"
                                                          "; seconds")
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "fs command" );
            }

        };
    }

    namespace fs {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}
