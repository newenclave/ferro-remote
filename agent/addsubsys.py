#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv
import os

def header_file():
    """
#ifndef FR_SUBSYS_%ss-name%_H
#define FR_SUBSYS_%ss-name%_H

#include "subsystem-iface.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class %ss-name%: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        %ss-name%( application *app );

    public:

        ~%ss-name%( );

        static vtrc::shared_ptr<%ss-name%> create( application *app );

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
#include "application.h"
#include "subsys-%ss-name%.h"
#include "subsystem-log.h"

//#include "vtrc-memory.h"

#define LOG(lev) log_(lev) << "[%ss-name%] "
#define LOGINF   LOG(logger::info)
#define LOGDBG   LOG(logger::debug)
#define LOGERR   LOG(logger::error)
#define LOGWRN   LOG(logger::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {
        const std::string subsys_name( "%ss-name%" );

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * /*app*/,
                                      vtrc::common::connection_iface_wptr cl )
        {
            ///vtrc::shared_ptr<impl_type_here>
            ///        inst(vtrc::make_shared<impl_type_here>( app, cl ));
            ///return app->wrap_service( cl, inst );

            return application::service_wrapper_sptr( );
        }
    }

    struct %ss-name%::impl {

        application     *app_;
        logger          &log_;

        impl( application *app )
            :app_(app)
            ,log_(app_->subsystem<subsys::log>( ).get_logger( ))
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
        }

    };


    %ss-name%::%ss-name%( application *app )
        :impl_(new impl(app))
    { }

    %ss-name%::~%ss-name%( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<%ss-name%> %ss-name%::create( application *app )
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
