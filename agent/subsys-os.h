
#ifndef FR_SUBSYS_os_H
#define FR_SUBSYS_os_H

#include "subsystem-iface.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class os: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        os( application *app );

    public:

        ~os( );

        static vtrc::shared_ptr<os> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    