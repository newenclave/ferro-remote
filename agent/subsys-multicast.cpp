#include "application.h"
#include "subsys-multicast.h"
#include "subsys-config.h"

#include "boost/asio/ip/multicast.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/strand.hpp"
#include "boost/bind.hpp"

#include "utils.h"

#include <functional>

#include "protocol/multicast.pb.h"

//#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#define LOG(lev) log_(lev) << "[mcast] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name = "multicast";
        const std::string any_ipv4    = "0.0.0.0";
        const std::string any_ipv6    = "::";

        namespace ba   = boost::asio;
        namespace bip  = ba::ip;
        using mcast_socket = ba::ip::udp::socket;
        using buffer_type  = std::vector<std::uint8_t>;

        struct point_info {
            mcast_socket         sock;
            buffer_type          buf;
            bip::udp::endpoint   from;
            point_info( ba::io_service &ios, size_t bufsize )
                :sock(ios)
                ,buf(bufsize)
            { }
        };

        using point_sptr = std::shared_ptr<point_info>;
        using point_wptr = std::weak_ptr<point_info>;
        using point_map  = std::map<std::string, point_sptr>;

    }

    struct multicast::impl {

        application         *app_;
        multicast           *parent_;
        subsys::config      *config_;
        agent::logger       &log_;

        point_map            points_;
        std::mutex           points_lock_;

        ba::io_service::strand dispatcher_;

        impl( application *app )
            :app_(app)
            ,parent_(NULL) /// avaid warnings
            ,config_(NULL)
            ,log_(app_->get_logger( ))
            ,dispatcher_(app_->get_io_service( ))
        { }

        void init( )
        {
            config_ = &app_->subsystem<config>( );
        }

        void add_point( std::string const &name )
        {
            auto info = utilities::get_endpoint_info( name );
            if( !info.is_ip( ) ) {
                LOGERR << "Endpoint format'" << name << "' is not valid";
                return;
            }

            try {

                auto addr = bip::address::from_string( info.addpess );

                bip::udp::endpoint listen_ep (
                            bip::address::from_string( addr.is_v6( )
                                                        ? any_ipv6
                                                        : any_ipv4 ),
                            info.service );

                LOGINF << "Adding endpoint '" << name << "'; "
                       << ( addr.is_v6( ) ? "ipv6" : "ipv4" )
                       << "; port: " << info.service;

                auto pinfo = std::make_shared<point_info>
                                ( app_->get_io_service( ), 256 );

                pinfo->sock.open( addr.is_v6( )
                                  ? bip::udp::v6( )
                                  : bip::udp::v4( ) );

                pinfo->sock.set_option( mcast_socket::reuse_address(true) );
                pinfo->sock.set_option( bip::multicast::join_group( addr ) );
                pinfo->sock.bind( listen_ep );
                {
                    std::lock_guard<std::mutex> l(points_lock_);
                    points_[name] = pinfo;
                }
                LOGDBG << "Store endpoint " << name << ". Ok";
            } catch( const std::exception &ex ) {
                LOGERR << "Add endpoint '" << name
                       << "' failed: " << ex.what( );
            }
        }

        void req_res( point_info &pinfo, size_t len )
        {
            multicast_request  req;
            multicast_response res;

            req.data    = &pinfo.buf[0];
            req.length  = len;

            req.from    = &pinfo.from;
            req.sender  = &pinfo.sock;

            fr::proto::mcast_response resp;
            resp.set_name( config_->cfgs( ).name );
            try {
                parent_->on_request_( vtrc::ref(req), vtrc::ref(res) );

                resp.set_gpio_available( res.gpio_available );
                for( auto &ep: res.endpoints ) {
                    resp.add_endpoints( ep );
                }

                std::string serial = resp.SerializeAsString(  );
                pinfo.sock.send_to( ba::buffer(serial), pinfo.from );
                LOGDBG << "Response to " << pinfo.from.address( ).to_string( )
                       << "\n{\n" << resp.DebugString( ) << "}";
                //LOGDBG << "Send response success.";
            } catch( const std::exception &ex ) {
                LOGERR << "Exception while sending signal: " << ex.what( );
            } catch( ... ) {
                LOGERR << "Exception while sending signal: ...";
            }
        }

        void handle_receive( const boost::system::error_code& error,
                             size_t bytes_recvd,
                             point_wptr info )
        {
            if( error ) {
                LOGERR << "Recv error: " << error.message( )
                       << " (code: " << error.value() << ")";
                return;
            }

            auto lck = info.lock( );
            if( lck ) {
                LOGDBG << "Recv request from "
                       << lck->from.address( ).to_string( )
                       << ":" << lck->from.port( )
                          ;
                ///
                req_res( *lck, bytes_recvd );
                ///
                start_recv( lck );
            }
        }

        bool start_recv( point_sptr info )
        {
            namespace ph = ba::placeholders;
            info->from = ba::ip::udp::endpoint( );
            try {
                info->sock.async_receive_from( ba::buffer(info->buf),
                    info->from,
                    dispatcher_.wrap(
                        boost::bind( &impl::handle_receive, this,
                                      ph::error, ph::bytes_transferred,
                                      point_wptr(info) ) ) );
                return true;
            } catch( const std::exception& ex ) {
                LOGERR << "async_receive exception: " << ex.what( );
            }
            return false;
        }

        void start_all( )
        {
            std::lock_guard<std::mutex> l(points_lock_);
            for( auto &p: points_ ) {
                LOGINF << "Starting point: " << p.first;
                start_recv( p.second );
            }
        }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
        }

    };

    multicast::multicast( application *app )
        :impl_(new impl(app))
    {
        impl_->parent_ = this;
    }

    multicast::~multicast( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<multicast> multicast::create( application *app )
    {
        vtrc::shared_ptr<multicast> new_inst(new multicast(app));
        return new_inst;
    }

    const std::string &multicast::name( )  const
    {
        return subsys_name;
    }

    void multicast::init( )
    {
        impl_->init( );
    }

    void multicast::start( )
    {
        for( auto &a: impl_->config_->cfgs( ).multicast ) {
            impl_->add_point( a );
        }
        impl_->start_all( );
    }

    void multicast::stop( )
    {

    }


}}}


