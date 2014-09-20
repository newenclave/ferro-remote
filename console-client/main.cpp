#include <iostream>

#include "client-core/fr-client.h"
#include "command-iface.h"

#include "boost/program_options.hpp"

namespace po = boost::program_options;

namespace {

    void fill_common_options( po::options_description &desc )
    {
        desc.add_options( )
            ("help,?",   "help message")
            ("server,s", po::value<std::string>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")
            ;
    }

    po::variables_map parce_common( int argc, const char **argv,
                                    const po::options_description &desc )
    {
        po::variables_map vm;
        po::parsed_options parsed (
            po::command_line_parser(argc, argv)
                .options(desc)
                .allow_unregistered( )
                .run( ));
        po::store(parsed, vm);
        po::notify(vm);
        return vm;
    }

}

int main( int argc, const char **argv )
{
    return 0;
}
