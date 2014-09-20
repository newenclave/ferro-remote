
#ifndef FR_SUBSYS_listeners_H
#define FR_SUBSYS_listeners_H

#include "subsystem-iface.h"

namespace fr { namespace server {

    class application;

namespace subsys {

    class listeners: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        listeners( application *app );

    public:

        ~listeners( );

        static vtrc::shared_ptr<listeners> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    