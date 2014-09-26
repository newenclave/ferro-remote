
#include <iostream>
#include <unistd.h>

#include "application.h"
#include "subsys-reactor.h"

#include "vtrc-thread.h"
#include "vtrc-bind.h"

#include <fcntl.h>
#include <sys/epoll.h>

#include "vtrc-atomic.h"

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
        vtrc::atomic<size_t> count_;
        vtrc::atomic<size_t> id_;


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
            ,id_(100)
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
            if( !running_ ) {
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

    void reactor::start( )
    {
        impl_->start_thread( );
    }

    void reactor::stop( )
    {
        impl_->stop_thread( );
    }

    void reactor::add_fd( int fd, unsigned flags, reaction_callback cb )
    {
        impl_->reactor_.add_fd( fd, flags, cb );
    }

    void reactor::del_fd( int fd )
    {
        impl_->reactor_.del_fd( fd );
    }

    size_t reactor::next_op_id( )
    {
        return ++impl_->id_;
    }

}}}

    
