
#include "command-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include "interfaces/IOS.h"

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"

#include "boost/program_options.hpp"

namespace fr { namespace cc { namespace cmd {

    namespace {

        const std::string main_table_name( "fr" );
        const std::string client_table_name( "client" );

        namespace po = boost::program_options;
        namespace core = client::core;

        typedef fr::lua::state lua_state;

        const char *cmd_name = "lua";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &client )
            {
                lua_state lv;
                if( vm.count( "exec" ) ) {
                    std::string script( vm["exec"].as<std::string>( ) );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserved as common
                /// "help,h"
                /// "command,c"
                /// "server,s"
                /// "io-pool-size,i"
                /// "rpc-pool-size,r"
                desc.add_options( )
                    ( "exec,e", po::value<std::string>( ),
                                                    "execute the script" )
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "lua command" );
            }

        };
    }

    namespace lua {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

#endif
    
