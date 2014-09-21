
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

        void reactor_thread( )
        {
            running_ = true;
            while( 1 ) try {
                while( reactor_.run_one( ) )  { }
            } catch( ... ) {
                ;;;
            }
        }

        impl( application *app )
            :app_(app)
            ,running_(false)
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

    static void change_print( int fd )
    {
        lseek( fd, 0, SEEK_SET );
        char b = 0;
        read( fd, &b, 1 );
        std::cout << "read: " << b << "\n";
    }

    void reactor::init( )
    {

    }

    void reactor::start( ) try
    {
        int fd = open( "/sys/class/gpio/gpio3/value", O_RDONLY );

        impl_->start_thread( );

        impl_->reactor_.add_fd( fd,  EPOLLIN | EPOLLET | EPOLLPRI,
                                vtrc::bind( change_print, fd ));
    } catch ( const std::exception &ex ) {
        std::cout << "error read: " << ex.what( ) << "\n";
    }

    void reactor::stop( )
    {

    }

    void add_fd( int fd, unsigned flags, reaction_callback cb )
    {

    }

    void del_fd( int fd )
    {

    }

}}}

    
