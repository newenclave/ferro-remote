#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv
import os

def source_file():
    """
#include "command-iface.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;

        const char *cmd_name = "%test-name%";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, client::core::client &client )
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

    namespace %test-name% {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

    """
    return source_file.__doc__

def usage(  ):
    """
    usage: addcmd.py <cmd-name>
    """
    print( usage.__doc__ )

if __name__ == '__main__':
    if len( argv ) < 2:
        usage( )
        exit( 1 )
    
    test_name = argv[1]
    
    src_name = 'cmd-' + test_name + '.cpp';
    
    src_path = os.path.join( src_name )
    
    src_content = source_file(  ).replace( '%test-name%', test_name )
    
    s = open( src_path, 'w' );
    s.write( src_content )

