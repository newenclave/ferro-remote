
#include <iostream>
#include <unistd.h>

#include "application.h"
#include "subsys-reactor.h"
#include "subsys-logging.h"

#include "vtrc-thread.h"
#include "vtrc-bind.h"

#include <fcntl.h>
#include <sys/epoll.h>

#include "vtrc-atomic.h"
#include "thread-prefix.h"

#define LOG(lev) log_(lev) << "[reactor] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)


namespace fr { namespace agent { namespace subsys {

    namespace {
        const std::string subsys_name( "reactor" );

        using poll_reactor_sptr = vtrc::shared_ptr<poll_reactor>;
        using thread_sptr = std::shared_ptr<vtrc::thread>;
        using thread_uptr = std::unique_ptr<vtrc::thread>;
    }

    struct reactor::impl {

        application          *app_;
        poll_reactor          reactor_;
        thread_uptr           thread_;
        bool                  running_;
        vtrc::atomic<size_t>  count_;
        vtrc::atomic<size_t>  id_;
        logger               &log_;


        void reactor_thread( )
        {
            thread_prefix_keeper _( "!" );
            running_ = true;
            while( 1 ) try {
                while( reactor_.run_one( ) ) { }
                return;
            } catch( const std::exception &ex ) {
                LOGERR << "Exception while run_one: " << ex.what( );
            }
        }

        impl( application *app )
            :app_(app)
            ,running_(false)
            ,count_(0)
            ,id_(100)
            ,log_(app->get_logger( ))
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
        }

        void start_thread( )
        {
            if( !running_ ) {
                thread_.reset( new vtrc::thread( [this]( ) {
                    reactor_thread( );
                } ) );
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

    
