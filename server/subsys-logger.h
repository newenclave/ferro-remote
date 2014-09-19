
#if !defined( __VTRC_SUBSYS_LOGGER__ )
#define       __VTRC_SUBSYS_LOGGER__

#include "subsystem-iface.h"

namespace fr { namespace server {

    class application;

namespace subsys {

    class logger: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        logger( application *app );

    public:

        ~logger( );

        static vtrc::shared_ptr<logger> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    
