
#include <iostream>
#include <unistd.h>

#include "application.h"
#include "subsys-reactor.h"

#include "vtrc-thread.h"
#include "vtrc-bind.h"

#include <fcntl.h>
#include <sys/epoll.h>

namespace fr { namespace server { namespace subsys {

    namespace {
        const std::string subsys_name( "reactor" );

        application::service_wrapper_sptr create_service(
                                          fr::server::application *,
                                          vtrc::common::connection_iface_wptr)
        {
            return application::service_wrapper_sptr( );
        }

        typedef vtrc::shared_ptr<poll_reactor> poll_reactor_sptr;
        typedef vtrc::shared_ptr<vtrc::thread> thread_sptr;
    }

    struct reactor::impl {

        application         *app_;
        poll_reactor         reactor_;
        thread_sptr          thread_;
        bool                 running_;
        size_t               count_;

        void reactor_thread( )
        {
            running_ = true;
            while( 1 ) try {
                while( reactor_.run_one( ) ) { }
                return;
            } catch( ... ) {
                ;;;
            }
        }

        impl( application *app )
            :app_(app)
            ,running_(false)
            ,count_(0)
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
        }

        void start_thread( )
        {
            if( !thread_ ) {
                thread_.reset(new vtrc::thread(
                                  vtrc::bind( &impl::reactor_thread, this ) ) );
                running_ = true;
            }
        }

        void stop_thread( )
        {
            if( !thread_ ) {
                return;
            }
            reactor_.stop( );
            if( thread_->joinable( ) ) {
                thread_->join( );
                thread_.reset( );
                running_ = false;
            }
        }

        void change_print( int fd, unsigned events )
        {
            lseek( fd, 0, SEEK_SET );
            char b = 0;
            ssize_t res = read( fd, &b, 1 );
            std::cout << "read: " << b << " " << res << "\n";
            count_ += (b - '0');

            if( count_ > 20 ) {
                close( fd );
//                reactor_.del_fd( fd );
//                if( 0 == reactor_.count( ) ) {
//                    std::cout << "STOP!\n";
//                    stop_thread( );
//                }
            }
        }

    };


    reactor::reactor( application *app )
        :impl_(new impl(app))
    { }

    reactor::~reactor( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<reactor> reactor::create( application *app )
    {
        vtrc::shared_ptr<reactor> new_inst(new reactor(app));
        return new_inst;
    }

    const std::string &reactor::name( )  const
    {
        return subsys_name;
    }

    void reactor::init( )
    {

    }

    void reactor::start( ) try
    {
        int fd = open( "/sys/class/gpio/gpio3/value", O_RDONLY );

        impl_->start_thread( );

        impl_->reactor_.add_fd( fd,  EPOLLIN | EPOLLET | EPOLLPRI,
                                vtrc::bind( &impl::change_print, impl_, fd,
                                            vtrc::placeholders::_1 ));

    } catch ( const std::exception &ex ) {
        std::cout << "error read: " << ex.what( ) << "\n";
    }

    void reactor::stop( )
    {
        impl_->stop_thread( );
    }

    void reactor::add_fd( int fd, unsigned flags, reaction_callback cb )
    {
        impl_->start_thread( );
        impl_->reactor_.add_fd( fd, flags, cb );
    }

    void reactor::del_fd( int fd )
    {
        impl_->reactor_.del_fd( fd );
    }

}}}

    
