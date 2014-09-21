#include "poll-reactor.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "vtrc-common/vtrc-exception.h"

namespace fr { namespace server {

    namespace {
        namespace vcomm = vtrc::common;

        int add_fd_to_epoll( int ep, int fd, uint32_t flags )
        {
            epoll_event epv;

            epv.events   = flags;
            epv.data.fd  = fd;

            return epoll_ctl( ep, EPOLL_CTL_ADD, fd, &epv );
        }

        int create_epoll( int stop_event )
        {
            int res = epoll_create( 5 );
            if( -1 == res ) {
                vcomm::throw_system_error( errno, "epoll_create" );
            }
            if( -1 == add_fd_to_epoll( res, stop_event, EPOLLIN | EPOLLET ) ) {
                close( res );
                vcomm::throw_system_error( errno, "epoll_ctl" );
            }
            return res;
        }

        int create_event(  )
        {
            int res = eventfd( 0, 0 );
            if( -1 == res ) {
                vcomm::throw_system_error( errno, "eventfd" );
            }
            return res;
        }

        struct fd_holder {
            int id_;

            fd_holder( int fd )
                :id_(fd)
            { }

            ~fd_holder( )
            {
                close( id_ );
            }

            int hdl( )
            {
                return id_;
            }
        };

        struct handle_reaction {

        };
    }

    struct poll_reactor::impl {
        fd_holder stop_event_;
        fd_holder poll_fd_;
        impl( )
            :stop_event_(create_event( ))
            ,poll_fd_(create_epoll(stop_event_.hdl( )))
        { }

        ~impl( )
        { }

        void stop( )
        {
            eventfd_write( stop_event_.hdl( ), 1 );
        }

    };

    poll_reactor::poll_reactor( )
        :impl_(new impl)
    {

    }

    poll_reactor::~poll_reactor( )
    {
        delete impl_;
    }


}}

