#include <iostream>

#include "interfaces/IFile.h"
#include "interfaces/IFilesystem.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

#include "interfaces/IOS.h"
#include "vtrc-common/vtrc-exception.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;
        namespace fiface = client::interfaces::filesystem;
        namespace fs = client::interfaces::filesystem;

        const char *cmd_name = "fs";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                if( vm.count( "path" ) ) {
                    std::string p(vm["path"].as<std::string>( ));
                    vtrc::unique_ptr<fs::iface> i( fs::create( cl, p ) );
                    for( auto &d: *i ) {
                        std::cout << d.path << "\n";
                    }
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h" "command,c" "server,s"
                /// "io-pool-size,i" "rpc-pool-size,r" "only-pool,o"
                desc.add_options( )
                    ("path,p", po::value<std::string>( ), "path to file")
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
