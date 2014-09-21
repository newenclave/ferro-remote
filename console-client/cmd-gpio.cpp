
#include "interfaces/IFile.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "vtrc-memory.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;

        const char *cmd_name = "gpio";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &client )
            {

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

    
