#include "poll-reactor.h"

#include <map>
#include <iostream>
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

        int add_fd_to_epoll( int ep, int fd, uint32_t flags,
                             handle_reaction *react )
        {
            epoll_event epv;

            epv.events   = flags;
            if( !react ) {
                epv.data.fd = fd;
            } else {
                epv.data.ptr = react;
            }

            return epoll_ctl( ep, EPOLL_CTL_ADD, fd, &epv );
        }

        int modify_fd_epoll( int ep, int fd, uint32_t flags )
        {
            epoll_event epv;

            epv.events   = flags;
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
            if( -1 == res ) {
                vcomm::throw_system_error( errno, "epoll_create" );
            }
            if( -1 == add_fd_to_epoll( res, stop_event,
                                       EPOLLIN | EPOLLET, NULL ) )
            {
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

        fd_holder           stop_event_;
        fd_holder           poll_fd_;
        reaction_map        react_;

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

        void add_fd( int fd, unsigned flags, reaction_callback cb )
        {
            std::cout << "Add " << fd << "\n";

            vtrc::upgradable_lock lck(react_lock_);

            reaction_map::iterator f(react_.find(fd));

            if( f == react_.end( ) ) {

                handle_reaction_sptr new_react(
                            vtrc::make_shared<handle_reaction>(fd));
                new_react->call_ = cb;

                vtrc::upgrade_to_unique utl(lck);

                react_[fd] = new_react;
                add_fd_to_epoll( poll_fd_.hdl( ), fd, flags, new_react.get( ) );
            } else {
                f->second->call_ = cb;
                modify_fd_epoll( poll_fd_.hdl( ), fd, flags );
            }
        }

        void del_fd( int fd )
        {
            del_fd_from_epoll( poll_fd_.hdl( ), fd );
            vtrc::unique_shared_lock lck(react_lock_);
            std::cout << "Erase fd " << react_.size( ) << "\n";
            react_.erase( fd );
            std::cout << "Erase fd " << react_.size( ) << "\n";
        }

        size_t count( ) const
        {
            vtrc::shared_lock slck(react_lock_);
            return react_.size( );
        }

        size_t run_one( )
        {
            epoll_event rcvd[1] = {0};
            int count = epoll_wait( poll_fd_.hdl( ), &rcvd[0], 1, -1);
            if( -1 == count ) {
                vcomm::throw_system_error( errno, "epoll_wait" );
            }

            std::cout << "Got " << count << " " << rcvd[0].data.fd
                      << " events\n";

            if( rcvd[0].data.fd == stop_event_.hdl( ) ) {
                std::cout << "Exit event rcved!\n";
                return 0;
            } else {
                handle_reaction *react =
                        static_cast<handle_reaction *>(rcvd[0].data.ptr);
                if( react ) {
                    react->call_( );
                }
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

    void poll_reactor::add_fd( int fd, unsigned flags, reaction_callback cb )
    {
        impl_->add_fd( fd, flags, cb );
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

