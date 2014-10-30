
#ifndef FR_SUBSYS_i2c_H
#define FR_SUBSYS_i2c_H

#include "subsystem-iface.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class i2c: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        i2c( application *app );

    public:

        ~i2c( );

        static vtrc::shared_ptr<i2c> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    
