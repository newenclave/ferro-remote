#include "command-iface.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;

        const char *cmd_name = "fs";

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
