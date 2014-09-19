#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv
import os

def header_file():
    """
#if !defined( __VTRC_SUBSYS_%ss-name%__ )
#define       __VTRC_SUBSYS_%ss-name%__

#include "subsystem-iface.h"

namespace boost {
    namespace program_options {
        class variables_map;
    }
}

namespace fr { namespace server {

    class application;

namespace subsys {

    class %ss-name%: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        %ss-name%( application *app );

    public:

        ~%ss-name%( );

        static vtrc::shared_ptr<%ss-name%> create( application *app,
                    const boost::program_options::variables_map &vm );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    """
    return header_file.__doc__

def source_file():
    """
#include "subsys-%ss-name%.h"


namespace fr { namespace server { namespace subsys {

    namespace {
        const std::string subsys_name( "%ss-name%" );
    }

    %ss-name%::%ss-name%( application *app )
    { }

    %ss-name%::~%ss-name%( )
    { }

    /// static
    vtrc::shared_ptr<%ss-name%> %ss-name%::create(application *app,
                             const boost::program_options::variables_map &vm )
    {
        vtrc::shared_ptr<%ss-name%> new_inst(new %ss-name%(app));
        return new_inst;
    }

    const std::string &%ss-name%::name( )  const
    {
        return subsys_name;
    }

    void %ss-name%::init( )
    {

    }

    void %ss-name%::start( )
    {

    }

    void %ss-name%::stop( )
    {

    }


}}}

    """
    return source_file.__doc__

def usage(  ):
    """
    usage: addsubsys.py <subsystem-name>
    """
    print( usage.__doc__ )

def fix_iface_inc( ss_name ):
    src_path = os.path.join( 'subsys.inc' )
    s = open( src_path, 'r' );
    content = s.readlines(  )
    s.close()
    content.append( '#include "subsys-'  + ss_name + '.h"\n')
    s = open( src_path, 'w' );
    s.writelines( content )

if __name__ == '__main__':
    if len( argv ) < 2:
        usage( )
        exit( 1 )

    ss_file = argv[1]
    ss_name = ss_file # ss_file.replace( '-', '_' )

    src_name = 'subsys-' + ss_file + '.cpp';
    hdr_name = 'subsys-' + ss_file + '.h';

    if os.path.exists( src_name ) or os.path.exists( hdr_name ):
        print ( "File already exists" )
        exit(1)

    src_content = source_file(  ).replace( '%ss-name%', ss_name )
    hdr_content = header_file(  ).replace( '%ss-name%', ss_name )

    s = open( src_name, 'w' );
    s.write( src_content )

    h = open( hdr_name, 'w' );
    h.write( hdr_content )

    fix_iface_inc( ss_name )
