
#ifndef FR_SUBSYS_SPI_H
#define FR_SUBSYS_SPI_H

#include "subsystem-iface.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class spi: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        spi( application *app );

    public:

        ~spi( );

        static vtrc::shared_ptr<spi> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

