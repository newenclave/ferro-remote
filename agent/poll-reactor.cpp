#include "poll-reactor.h"

#include <map>
#include <chrono>
#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>

#include "errno-check.h"

#include "vtrc/common/exception.h"

#include "vtrc/common/config/vtrc-memory.h"
#include "vtrc/common/config/vtrc-mutex.h"
#include "file-keeper.h"

namespace fr { namespace agent {

    namespace {

        namespace vcomm = vtrc::common;

        struct handle_reaction {

            handle_reaction( int fd )
                :fd_(fd)
            { }

            file_keeper       fd_;
            reaction_callback call_;

        };

        int add_fd_to_epoll( int ep, int fd, uint32_t events )
        {
            epoll_event epv = {0};

            epv.events  = events;
            epv.data.fd = fd;

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
            int fd = epoll_create( 64 );

            errno_error::errno_assert( -1 != fd, "epoll_create" );

            int res = add_fd_to_epoll( fd, stop_event, EPOLLIN | EPOLLET );

            if( -1 == res ) {
                close( fd );
                vcomm::throw_system_error( errno, "epoll_ctl" );
            }
            return fd;
        }

        int create_event(  )
        {
            int res = eventfd( 0, 0 );
            if( -1 == res ) {
                vcomm::throw_system_error( errno, "eventfd" );
            }
            return res;
        }

        using handle_reaction_sptr = vtrc::shared_ptr<handle_reaction>;
        using reaction_map = std::map<int, handle_reaction_sptr>;
    }

    struct poll_reactor::impl {

        file_keeper    stop_event_;
        file_keeper    poll_fd_;
        reaction_map   react_;
        std::uint64_t  app_start_;

        mutable vtrc::mutex  react_lock_;
        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

        impl( std::uint64_t app_start )
            :stop_event_(create_event( ))
            ,poll_fd_(create_epoll(stop_event_.hdl( )))
            ,app_start_(app_start)
        { }

        ~impl( )
        { }

        std::uint64_t now( ) const
        {
            using std::chrono::duration_cast;
            using microsec = std::chrono::microseconds;
            auto n = std::chrono::high_resolution_clock::now( );
            return duration_cast<microsec>(n.time_since_epoch( )).count( );
        }

        std::uint64_t tick_count( ) const
        {
            return now( ) - app_start_;
        }

        void stop( )
        {
            /*int res = */
            eventfd_write( stop_event_.hdl( ), 1 );
        }

        void add_fd( int fd, unsigned events, reaction_callback cb )
        {
            locker_type lck(react_lock_);

            reaction_map::iterator f(react_.find(fd));

            if( f == react_.end( ) ) {

                auto new_react(vtrc::make_shared<handle_reaction>(fd));

                new_react->call_ = cb;

                int res = add_fd_to_epoll( poll_fd_.hdl( ), fd, events );

                errno_error::errno_assert( -1 != res, "epoll_ctl" );
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
            locker_type lck(react_lock_);
            react_.erase( fd );
        }

        void del_fd_unsafe( int fd )
        {
            del_fd_from_epoll( poll_fd_.hdl( ), fd );
            react_.erase( fd );
        }

        size_t count( ) const
        {
            locker_type slck(react_lock_);
            return react_.size( );
        }

        void make_callback( int fd, unsigned events, std::uint64_t ticks )
        {
            locker_type slck(react_lock_);

            auto f( react_.find( fd ) );

            if( f != react_.end( ) ) {
                bool res = f->second->call_( events, ticks );
                if( !res ) {
                    std::cout << "No res! delete\n";
                    del_fd_unsafe( fd );
                }
            } else {
                del_fd_from_epoll( poll_fd_.hdl( ), fd );
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
                make_callback( rcvd.data.fd, rcvd.events, tick_count( ) );
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

    poll_reactor::poll_reactor(std::uint64_t app_start)
        :impl_(new impl(app_start))
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

