
#ifndef FR_SUBSYS_reactor_H
#define FR_SUBSYS_reactor_H

#include "subsystem-iface.h"
#include "poll-reactor.h"

#include <sys/epoll.h>

namespace fr { namespace server {

    class application;

namespace subsys {

    class reactor: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        reactor( application *app );

    public:

        ~reactor( );

        static vtrc::shared_ptr<reactor> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        void add_fd( int fd, unsigned flags, reaction_callback cb );
        void del_fd( int fd );

        size_t next_op_id( );

    };

}}}

#endif

    
