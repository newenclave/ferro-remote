
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "interfaces/IOS.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;

        const char *cmd_name = "os";
        typedef vtrc::unique_ptr<fr::client::interfaces::os::iface> os_uptr;

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &client )
            {
                if( vm.count( "exec" ) ) {
                    std::string cmd(vm["exec"].as<std::string>( ));
                    os_uptr inst(fr::client::interfaces::os::create(client));
                    inst->execute( cmd );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r"
                desc.add_options( )
                    ("exec,e", po::value<std::string>( ),
                                        "execute command line; "
                                        "the same as 'system' from stdlib")
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "os command" );
            }

        };
    }

    namespace os {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

    
