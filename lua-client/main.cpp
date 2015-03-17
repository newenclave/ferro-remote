#include <iostream>

#include "fr-lua.h"

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "vtrc-client/vtrc-client.h"

#include "boost/program_options.hpp"

namespace vcomm     = vtrc::common;
namespace vclient   = vtrc::client;

namespace po = boost::program_options;

namespace {

    void fill_common_options( po::options_description &desc )
    {
        desc.add_options( )

            ("help,h",   "help message")

            ("script,e", po::value<std::string>( ), "path to script" )

            ("server,s", po::value<std::string>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("id", po::value<std::string>( ),
                    "set client ID for the remote agent")

            ("key", po::value<std::string>( ),
                    "set client KEY for the remote agent")

            ;
    }

}

int main( int argc, const char *argv[] )
{

    return 0;
}
