
#ifndef FR_SUBSYS_gpio_H
#define FR_SUBSYS_gpio_H

#include "subsystem-iface.h"

namespace fr { namespace server {

    class application;

namespace subsys {

    class gpio: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        gpio( application *app );

    public:

        ~gpio( );

        static vtrc::shared_ptr<gpio> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    