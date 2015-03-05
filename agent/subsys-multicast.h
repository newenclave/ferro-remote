#ifndef FR_SUBSYS_multicast_H
#define FR_SUBSYS_multicast_H

#include "subsystem-iface.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class multicast: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        multicast( application *app );

    public:

        ~multicast( );

        static vtrc::shared_ptr<multicast> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    
