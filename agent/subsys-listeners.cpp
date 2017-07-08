
#include <vector>
#include <algorithm>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "application.h"
#include "subsys-listeners.h"
#include "subsys-netifaces.h"
#include "subsys-multicast.h"

#include "subsys-config.h"

#include "vtrc/server/listener-tcp.h"
#include "vtrc/server/listener-local.h"
#include "vtrc/common/connection-iface.h"

#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"

#include "boost/lexical_cast.hpp"
#include "boost/program_options.hpp"
#include "boost/system/error_code.hpp"
#include "boost/filesystem.hpp"
#include "boost/asio/ip/address.hpp"

#include "subsys-logging.h"

#include "utils.h"

#define LOG(lev) log_(lev) << "[listener] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        namespace po = boost::program_options;

        namespace vserv = vtrc::server;
        namespace vcomm = vtrc::common;
        namespace fs = boost::filesystem;
        namespace ba = boost::asio;

        typedef vtrc::server::listener_sptr listener_sptr;
        struct listener_info {
            listener_sptr            ptr;
            utilities::endpoint_info info;
            ba::ip::address          addr;
        };

        typedef std::vector<listener_info>  listener_vector;
        //typedef std::vector<listener_sptr>  listener_vector;

        const std::string subsys_name( "listeners" );

        listener_info listener_from_string( const std::string &name,
                                            application &app )
        {
            /// result endpoint
            listener_info result;

            using namespace vserv::listeners;
            auto &log_(app.get_logger( ));

            auto ep = result.info = utilities::get_endpoint_info( name );

            LOGDBG << "Got: " << ep;

            if( ep.is_local( ) ) {
                if( !fs::exists( ep.addpess ) ) {
                    result.ptr = local::create( app, ep.addpess );
                    return result;
                }

                struct stat st = {0};
                int r = ::stat( ep.addpess.c_str( ), &st );
                if( -1 != r ) {
                    if( S_ISSOCK( st.st_mode ) ) {
                        LOGINF << "Socket '" << ep.addpess
                               << "' found. unlinking it...";
                        ::unlink( name.c_str( ) ); /// unlink old file socket
                        result.ptr = local::create( app, ep.addpess );
                    } else {
                        LOGERR << "File " << name << " found. "
                               << "But it is not a socket.";
                        throw std::runtime_error( "Bad path." );
                    }
                } else {
                    std::error_code ec( errno, std::system_category( ));
                    LOGDBG << "::stat( ) for '"
                           << name << "' failed. "
                           << ec.message( );
                    throw std::runtime_error( ec.message( ) );
                }
            } else if( ep.is_ip( ) ) {
                result.addr = result.addr.from_string( ep.addpess );
                result.ptr = tcp::create( app, ep.addpess, ep.service, true );
            }

            return std::move(result);
        }

    }

    struct listeners::impl {

        application     *app_;
        subsys::config  *config_;

        listener_vector  listenrs_;

        vtrc::atomic<size_t> counter_;

        logger &log_;

        impl( application *app )
            :app_(app)
            ,counter_(0)
            ,log_(app_->get_logger( ))
        { }

        void on_new_connection( vserv::listener *l,
                                const vcomm::connection_iface *c )
        {
            LOGINF << "New connection:"
                   << " ep: "     << l->name( )
                   << " client: " << c->name( )
                   << " total: "  << ++counter_
                   ;
        }

        void on_stop_connection( vserv::listener *l,
                                 const vcomm::connection_iface *c )
        {
            LOGINF << l->name( )
                   << " Close connection: "
                   << c->name( )
                   << "; count: " << --counter_
                   ;
        }

        void on_accept_failed( vserv::listener *l,
                               unsigned /*retry_to*/,
                               const boost::system::error_code &code )
        {
            LOGERR << "Accept failed at " << l->name( )
                   << " due to '" << code.message( ) << "'";
            //start_retry_accept( l->shared_from_this( ), retry_to );
        }

        void add_listener( const std::string &name )
        {
            namespace ph = vtrc::placeholders;
            auto list = listener_from_string( name, *app_ );

            list.ptr->on_new_connection_connect(
                    vtrc::bind( &impl::on_new_connection, this,
                                list.ptr.get( ), ph::_1 ));

            list.ptr->on_stop_connection_connect(
                    vtrc::bind( &impl::on_stop_connection, this,
                                list.ptr.get( ), ph::_1 ));

            list.ptr->on_accept_failed_connect(
                    vtrc::bind( &impl::on_accept_failed, this,
                                list.ptr.get( ), 0, ph::_1 ) );

            listenrs_.emplace_back( std::move(list) );
        }

        void start_all(  )
        {
            for( auto &l: listenrs_ ) {
                try {
                    l.ptr->start( );
                    LOGINF << l.ptr->name( ) << " started";
                } catch( const std::exception &ex ) {
                    LOGERR << l.ptr->name( )
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
                (*b).ptr->stop( );
                LOGINF << (*b).ptr->name( )
                       << " stopped";
            }
        }

        static bool check_listener( const netifaces::iface_info &inf,
                                    const listener_info &lst )
        {
            return lst.info.is_ip( ) &&
                 ( lst.addr.is_v4( ) == inf.is_v4( ) ) &&
                 ( lst.addr.is_unspecified( ) || inf.check( lst.addr ) );
        }

        void mcast_res( const subsys::multicast_request &req,
                              subsys::multicast_response &res )
        {
            auto ep = req.from;
            auto ifaces = app_->subsystem<netifaces>( ).ifaces( );
            for( auto &i: *ifaces ) {
                if( i.check( ep->address( ) ) ) {
//                    LOGDBG << "Found network interface '" << i
//                           << "' for client " << ep->address( );
                    for( auto &lst: listenrs_ ) {
                        if( check_listener( i, lst ) ) {
                            std::ostringstream oss;
                            oss << i.addr( ) << ":" << lst.info.service;
                            LOGDBG << "Found endpoint '" << lst.info
                                   << "' for client " << ep->address( )
                                   << ". Adding response: '"
                                   << oss.str( ) << "'";
                            res.endpoints.insert( oss.str( ) );
                        }
                    }
                }
            }
            LOGDBG << "Response size: " << res.endpoints.size( );
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
        impl_->app_->subsystem<subsys::multicast>( ).on_request_connect(
            [this]( const subsys::multicast_request &req,
                          subsys::multicast_response &res )
            {
                impl_->mcast_res( req, res );
            });

    }

    void listeners::start( )
    {
        using slist = std::vector<std::string>;

        const slist &servs( impl_->config_->cfgs( ).endpoints );
        std::for_each( servs.begin( ), servs.end( ),
                       vtrc::bind( &impl::add_listener, impl_,
                                   vtrc::placeholders::_1 ));

        impl_->start_all( );
        impl_->LOGINF << "Started.";
    }

    void listeners::stop( )
    {
        impl_->stop_all( );
        impl_->LOGINF << "Stopped.";
    }

}}}


