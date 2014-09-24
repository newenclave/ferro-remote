#include <iostream>

#include "interfaces/IFile.h"
#include "interfaces/IFilesystem.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "interfaces/IOS.h"
#include "vtrc-common/vtrc-exception.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po    = boost::program_options;
        namespace core  = client::core;
        namespace fs    = client::interfaces::filesystem;

        const char *cmd_name = "fs";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
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
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
                desc.add_options( )
                    ("list,l", po::value<std::string>( ), "show directory list")
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
