
#ifndef FR_SUBSYS_LOGGER_H
#define FR_SUBSYS_LOGGER_H

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

        enum log_level_type {
             level_none = 0
            ,level_info
            ,level_warning
            ,level_debug
        };

        ~logger( );

        static vtrc::shared_ptr<logger> create( application *app );

        const std::string &name( )  const;

        void init( );
        void start( );
        void stop( );

    public:
        void set_level( log_level_type lt );
    };

}}}

#endif

    
