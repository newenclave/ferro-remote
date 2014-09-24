
#include <vector>
#include <algorithm>
#include <iostream>

#include "application.h"
#include "subsys-listeners.h"

#include "subsys-config.h"

#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-local.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"

#include "boost/lexical_cast.hpp"
#include "boost/program_options.hpp"
#include "boost/system/error_code.hpp"

namespace fr { namespace server { namespace subsys {

    namespace {

        namespace po = boost::program_options;

        namespace vserv = vtrc::server;
        namespace vcomm = vtrc::common;

        typedef vtrc::server::listener_sptr listener_sptr;
        typedef std::vector<listener_sptr>  listener_vector;

        const std::string subsys_name( "listeners" );

        listener_sptr listener_from_string( const std::string &name,
                                            application &app )
        {
            /// result endpoint
            listener_sptr result;

            std::vector<std::string> params;

            size_t delim_pos = name.find_last_of( ':' );
            if( delim_pos == std::string::npos ) {

                /// local: <localname>
                params.push_back( name );
#ifndef _WIN32
                ::unlink( name.c_str( ) ); /// unlink old file socket
#endif
                result = vserv::listeners::local::create( app, name );

            } else {

                /// tcp: <addres>:<port>
                std::string addr( std::string( name.begin( ),
                                               name.begin( ) + delim_pos ) );

                std::string port( std::string( name.begin( ) + delim_pos + 1,
                                               name.end( ) ) );

                result = vserv::listeners::tcp::create( app, addr,
                                boost::lexical_cast<unsigned short>(port),
                                true );
            }
            return result;
        }

    }

    struct listeners::impl {

        application     *app_;
        subsys::config  *config_;

        listener_vector  listenrs_;

        vtrc::atomic<size_t> counter_;

        impl( application *app )
            :app_(app)
            ,counter_(0)
        { }


        void on_new_connection( vserv::listener *l,
                                const vcomm::connection_iface *c )
        {
            std::cout << "New connection: "
                      << "\n\tep:     " << l->name( )
                      << "\n\tclient: " << c->name( )
                      << "\n\ttotal:  " << ++counter_
                      << "\n"
                        ;
        }

        void on_stop_connection( vserv::listener *l,
                                 const vcomm::connection_iface *c )
        {
            std::cout << "Close connection: "
                      << c->name( )
                      << "; count: " << --counter_
                      << "\n";
        }

        void on_accept_failed( vserv::listener *l,
                               unsigned retry_to,
                               const boost::system::error_code &code )
        {
            std::cout << "Accept failed at " << l->name( )
                      << " due to '" << code.message( ) << "'\n";
            //start_retry_accept( l->shared_from_this( ), retry_to );
        }


        void add_listener( const std::string &name )
        {
            listener_sptr list(listener_from_string( name, *app_ ));

            list->on_new_connection_connect(
                   vtrc::bind( &impl::on_new_connection, this,
                               list.get( ), vtrc::placeholders::_1 ));

            list->on_stop_connection_connect(
                   vtrc::bind( &impl::on_stop_connection, this,
                               list.get( ), vtrc::placeholders::_1 ));

            list->on_accept_failed_connect(
                   vtrc::bind( &impl::on_accept_failed, this,
                               list.get( ), 0,
                               vtrc::placeholders::_1 ) );

            listenrs_.push_back(list);
        }

        void start_all(  )
        {
            size_t count = 0;
            for( listener_vector::iterator b(listenrs_.begin( )),
                 e(listenrs_.end( )); b!=e; ++b )
            {
                try {
                    (*b)->start( );
                    ++count;
                    std::cout << (*b)->name( )
                              << " started\n";
                } catch( const std::exception &ex ) {
                    std::cerr << "Listener " << (*b)->name( )
                              << " failed to start; "
                              << ex.what( );
                }
            }
        }

        void stop_all(  )
        {
            for( listener_vector::iterator b(listenrs_.begin( )),
                 e(listenrs_.end( )); b!=e; ++b )
            {
                (*b)->stop( );
                std::cout << (*b)->name( )
                          << " stopped\n";
            }
        }
    };


    listeners::listeners( application *app )
        :impl_(new impl(app))
    { }

    listeners::~listeners( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<listeners> listeners::create( application *app )
    {
        vtrc::shared_ptr<listeners> new_inst(new listeners(app));
        return new_inst;
    }

    const std::string &listeners::name( )  const
    {
        return subsys_name;
    }

    void listeners::init( )
    {
        impl_->config_ = &impl_->app_->subsystem<subsys::config>( );
    }

    void listeners::start( )
    {
        const po::variables_map &vars(impl_->config_->variables( ));

        if( vars.count( "server" ) ) {
            typedef std::vector<std::string> slist;
            slist servs(vars["server"].as<slist>( ));
            std::for_each( servs.begin( ), servs.end( ),
                           vtrc::bind( &impl::add_listener, impl_,
                                       vtrc::placeholders::_1 ));
        }

        impl_->start_all( );
    }

    void listeners::stop( )
    {
        impl_->stop_all( );
    }

}}}

    
