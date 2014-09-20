
#ifndef FR_SUBSYS_fs_H
#define FR_SUBSYS_fs_H

#include "subsystem-iface.h"

namespace fr { namespace server {

    class application;

namespace subsys {

    class fs: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        fs( application *app );

    public:

        ~fs( );

        static vtrc::shared_ptr<fs> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    