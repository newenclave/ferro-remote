
#ifndef FR_SUBSYS_LUA_H
#define FR_SUBSYS_LUA_H

#include "subsystem-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA


namespace fr { namespace agent {

    class application;

namespace subsys {

    class lua: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        lua( application *app );

    public:

        ~lua( );

        static vtrc::shared_ptr<lua> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif
#endif // LUA
    
