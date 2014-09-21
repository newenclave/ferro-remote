#include "poll-reactor.h"

#include <map>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>

#include "vtrc-common/vtrc-exception.h"

#include "vtrc-memory.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

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

        int del_fd_from_epoll( int ep, int fd )
        {
            epoll_event epv;
            epv.data.fd  = fd;
            return epoll_ctl( ep, EPOLL_CTL_DEL, fd, &epv );
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

            handle_reaction( int fd )
                :fd_(fd)
            { }

            fd_holder                   fd_;
            vtrc::function<void (void)> call_;

        };

        typedef vtrc::shared_ptr<handle_reaction>   handle_reaction_sptr;
        typedef std::map<int, handle_reaction_sptr> reaction_map;
    }

    struct poll_reactor::impl {

        fd_holder           stop_event_;
        fd_holder           poll_fd_;
        reaction_map        react_;
        vtrc::shared_mutex  react_lock_;

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

        void add_fd( int fd, unsigned flags, reaction_callback cb )
        {
            vtrc::upgradable_lock lck(react_lock_);

            reaction_map::iterator f(react_.find(fd));

            if( f == react_.end( ) ) {
                handle_reaction_sptr new_reaction(
                            vtrc::make_shared<handle_reaction>(fd));
                new_reaction->call_ = cb;

                vtrc::upgrade_to_unique utl(lck);

                react_[fd] = new_reaction;
                add_fd_to_epoll( poll_fd_.hdl( ), fd, flags );
            } else {
                f->second->call_ = cb;
            }
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


    void poll_reactor::add_fd( int fd, unsigned flags, reaction_callback cb )
    {
        impl_->add_fd( fd, flags, cb );
    }

}}

