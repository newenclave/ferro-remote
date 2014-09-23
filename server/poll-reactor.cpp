#include "poll-reactor.h"

#include <map>
#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>

#include "errno-check.h"

#include "vtrc-common/vtrc-exception.h"

#include "vtrc-memory.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "file-keeper.h"

namespace fr { namespace server {

    namespace {

        namespace vcomm = vtrc::common;

        struct handle_reaction {

            handle_reaction( int fd )
                :fd_(fd)
            { }

            file_keeper       fd_;
            reaction_callback call_;

        };

        int add_fd_to_epoll( int ep, int fd, uint32_t events,
                             handle_reaction *react )
        {
            epoll_event epv;

            epv.events   = events;
            if( !react ) {
                epv.data.fd = fd;
            } else {
                epv.data.ptr = react;
            }

            return epoll_ctl( ep, EPOLL_CTL_ADD, fd, &epv );
        }

        int modify_fd_epoll( int ep, int fd, uint32_t events )
        {
            epoll_event epv;

            epv.events   = events;
            epv.data.fd  = fd;

            return epoll_ctl( ep, EPOLL_CTL_MOD, fd, &epv );
        }

        int del_fd_from_epoll( int ep, int fd )
        {
            epoll_event epv;
            epv.data.fd  = fd;
            return epoll_ctl( ep, EPOLL_CTL_DEL, fd, &epv );
        }

        int create_epoll( int stop_event )
        {
            int res = epoll_create( 64 );
            errno_error::errno_assert( -1 != res, "epoll_create" );

            res = add_fd_to_epoll( res, stop_event, EPOLLIN | EPOLLET, NULL );
            if( -1 == res ) {
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

        typedef vtrc::shared_ptr<handle_reaction>   handle_reaction_sptr;
        typedef std::map<int, handle_reaction_sptr> reaction_map;
    }

    struct poll_reactor::impl {

        file_keeper    stop_event_;
        file_keeper    poll_fd_;
        reaction_map   react_;

        mutable vtrc::shared_mutex  react_lock_;

        impl( )
            :stop_event_(create_event( ))
            ,poll_fd_(create_epoll(stop_event_.hdl( )))
        { }

        ~impl( )
        { }

        void stop( )
        {
            int res = eventfd_write( stop_event_.hdl( ), 1 );
            std::cout << "Send stop event " << res << errno << "\n";
        }

        void add_fd( int fd, unsigned events, reaction_callback cb )
        {
            vtrc::upgradable_lock lck(react_lock_);

            reaction_map::iterator f(react_.find(fd));

            if( f == react_.end( ) ) {

                handle_reaction_sptr new_react(
                            vtrc::make_shared<handle_reaction>(fd));

                new_react->call_ = cb;

                int res = add_fd_to_epoll( poll_fd_.hdl( ), fd, events, NULL );

                std::cout << "add failed: " << errno
                          << " " << poll_fd_.hdl( )
                          << " " << res
                          << "\n";

                errno_error::errno_assert( -1 != res, "epoll_ctl" );
                vtrc::upgrade_to_unique utl(lck);
                react_.insert( std::make_pair( fd, new_react ) );

            } else {
                f->second->call_ = cb;
                int res = modify_fd_epoll( poll_fd_.hdl( ), fd, events );
                errno_error::errno_assert( -1 != res, "epoll_ctl" );
            }
        }

        void del_fd( int fd )
        {
            del_fd_from_epoll( poll_fd_.hdl( ), fd );
            vtrc::unique_shared_lock lck(react_lock_);
            react_.erase( fd );
        }

        size_t count( ) const
        {
            vtrc::shared_lock slck(react_lock_);
            return react_.size( );
        }

        void make_callback( int fd, unsigned events )
        {
            vtrc::shared_lock slck(react_lock_);
            reaction_map::iterator f(react_.find( fd ));
            if( f != react_.end( ) ){
                f->second->call_( events );
            }
        }

        size_t run_one( )
        {
            epoll_event rcvd = { 0 };

            int count = epoll_wait( poll_fd_.hdl( ), &rcvd, 1, -1);

            errno_error::errno_assert( -1 != count, "epoll_wait" );

            if( rcvd.data.fd == stop_event_.hdl( ) ) {
                return 0;
            } else {
                make_callback( rcvd.data.fd, rcvd.events );
                return 1;
            }
        }

        size_t run( )
        {
            size_t count = 0;

            while( run_one( ) ) {
                ++count;
            }
            return count;
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

    void poll_reactor::add_fd(int fd, unsigned events, reaction_callback cb )
    {
        impl_->add_fd( fd, events, cb );
    }

    void poll_reactor::del_fd( int fd )
    {
        impl_->del_fd( fd );
    }

    size_t poll_reactor::count( ) const
    {
        return impl_->count( );
    }

    size_t poll_reactor::run_one( )
    {
        return impl_->run_one( );
    }

    size_t poll_reactor::run( )
    {
        return impl_->run( );
    }

    void poll_reactor::stop( )
    {
        impl_->stop( );
    }

}}

