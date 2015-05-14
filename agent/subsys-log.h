
#ifndef FR_SUBSYS_log_H
#define FR_SUBSYS_log_H

#include "subsystem-iface.h"
#include "logger.hxx"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class log: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        log( application *app );

    public:

        ~log( );

        static vtrc::shared_ptr<log> create( application *app );

        const std::string &name( ) const;

    public:

        logger &get_logger(  );

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    
