#ifndef FR_SUBSYSCONFIG_H
#define FR_SUBSYSCONFIG_H

#include "subsystem-iface.h"


namespace fr { namespace server {

    class application;

namespace subsys {

    class config: public subsystem_iface {
    protected:
        config( application &app );

    public:

        static vtrc::shared_ptr<config> create( application &app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

    };

}}}


#endif // FR_SUBSYSCONFIG_H
