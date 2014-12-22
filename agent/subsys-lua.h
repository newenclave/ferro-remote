
#ifndef FR_SUBSYS_lua_H
#define FR_SUBSYS_lua_H

#include "subsystem-iface.h"

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

    