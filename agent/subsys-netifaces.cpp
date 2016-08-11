
#include "application.h"
#include "subsys-netifaces.h"
#include "subsys-logging.h"

#include <ifaddrs.h>

//#include "vtrc-memory.h"

#include "boost/asio/ip/address.hpp"
#include "boost/system/error_code.hpp"
#include "boost/system/system_error.hpp"

#define LOG(lev) log_(lev) << "[netifaces] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        namespace bsys = boost::system;

        const std::string subsys_name( "netifaces" );

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * /*app*/,
                                      vtrc::common::connection_iface_wptr cl )
        {
            ///vtrc::shared_ptr<impl_type_here>
            ///        inst(vtrc::make_shared<impl_type_here>( app, cl ));
            ///return app->wrap_service( cl, inst );

            return application::service_wrapper_sptr( );
        }

        bool enum_ifaces_( netifaces::iface_info_list &out, logger &log_ )
        {
            ifaddrs *addrs = nullptr;
            int res = ::getifaddrs( &addrs );
            if( 0 != res ) {
                bsys::error_code ec(errno, bsys::get_system_category( ));
                LOGERR << "::getifaddrs failed; errno = " << errno
                       << ec.message( );
                return false;
            }
            netifaces::iface_info_list tmp;
            ifaddrs *p = addrs;

            while( p ) {
                switch (p->ifa_addr->sa_family) {
                case AF_INET:
                case AF_INET6:
                    tmp.emplace_back( p );
                    break;
                }
                p = p->ifa_next;
            }
            ::freeifaddrs( addrs );
            out.swap( tmp );
            return true;
        }

    }

    struct netifaces::impl {

        using iface_info_list     = netifaces::iface_info_list;
        using iface_info_list_ptr = std::shared_ptr<netifaces::iface_info_list>;

        application       *app_;
        logger            &log_;

        iface_info_list_ptr  ifaces_;
        std::mutex        ifaces_lock_;
        std::uint64_t     last_enum_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
            ,last_enum_(0)
        {
            ifaces_ = std::make_shared<iface_info_list>( );
            if( enum_ifaces_( *ifaces_, log_ ) ) {
                last_enum_ = application::tick_count( );
            }
        }

        bool enum_ifaces( netifaces::iface_info_list &out )
        {
            return enum_ifaces_( out, log_ );
        }

        void set_ifaces( iface_info_list_ptr &newif )
        {
            std::lock_guard<std::mutex> lck(ifaces_lock_);
            ifaces_.swap( newif );
        }

        void test_addr( )
        {
            ifaddrs *addrs = nullptr;
            ::getifaddrs( &addrs );
            ifaddrs *p = addrs;
            using namespace boost::asio::ip;

            while( p ) {

                address addr;
                address mask;

                if( p->ifa_addr->sa_family == AF_INET ) {
                    auto si = reinterpret_cast<sockaddr_in *>(p->ifa_addr);
                    auto sm = reinterpret_cast<sockaddr_in *>(p->ifa_netmask);
                    addr = address_v4( ntohl(si->sin_addr.s_addr) );
                    mask = address_v4( ntohl(sm->sin_addr.s_addr) );
                } else if( p->ifa_addr->sa_family == AF_INET6 ) {
                    auto si = reinterpret_cast<sockaddr_in6 *>(p->ifa_addr);
                    auto sm = reinterpret_cast<sockaddr_in6 *>(p->ifa_netmask);
                    address_v6::bytes_type bytes;
                    std::copy( &si->sin6_addr.s6_addr[0],
                               &si->sin6_addr.s6_addr[bytes.size( )],
                               bytes.begin( ) );
                    address_v6::bytes_type bytesm;
                    std::copy( &sm->sin6_addr.s6_addr[0],
                               &sm->sin6_addr.s6_addr[bytesm.size( )],
                               bytesm.begin( ) );
                    addr = address_v6( bytes );
                    mask = address_v6( bytesm );
                } else {
                    std::cout << "unkn family "
                              << p->ifa_addr->sa_family << "...";
                }
                std::cout << addr.to_string( ) << "/" << mask << "\n";

                p = p->ifa_next;
            }

            ::freeifaddrs( addrs );

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


    netifaces::netifaces( application *app )
        :impl_(new impl(app))
    { }

    netifaces::~netifaces( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<netifaces> netifaces::create( application *app )
    {
        vtrc::shared_ptr<netifaces> new_inst(new netifaces(app));
        return new_inst;
    }

    const std::string &netifaces::name( )  const
    {
        return subsys_name;
    }

    void netifaces::init( )
    {

    }

    void netifaces::start( )
    {
//        impl_->test_addr( );
//        netifaces::iface_info_list res;
//        impl_->enum_ifaces( res );

//        for( auto &a: res ) {
//            std::cout << a.in4( )->sin_family << "\n";
//        }

//        std::terminate( );
        impl_->LOGINF << "Started.";
    }

    void netifaces::stop( )
    {
        impl_->LOGINF << "Stopped.";
    }

    netifaces::iface_info_list_ptr netifaces::ifaces( ) const
    {
        std::lock_guard<std::mutex> lck(impl_->ifaces_lock_);
        return impl_->ifaces_;
    }

    netifaces::iface_info_list netifaces::enum_ifaces( )
    {
        netifaces::iface_info_list res;
        impl_->enum_ifaces( res );
        return res;
    }

    netifaces::iface_info::iface_info( const ifaddrs *ifa )
    {
        name_.assign( ifa->ifa_name );
        switch (ifa->ifa_addr->sa_family) {
        case AF_INET:
            memcpy(&sockaddr_, ifa->ifa_addr, sizeof(sockaddr_in));
            memcpy(&mask_, ifa->ifa_netmask,  sizeof(sockaddr_in));
            break;
        case AF_INET6:
            memcpy(&sockaddr_, ifa->ifa_addr, sizeof(sockaddr_in6));
            memcpy(&mask_, ifa->ifa_netmask,  sizeof(sockaddr_in6));
            break;
        default:
            memset(&sockaddr_, 0, sizeof(sockaddr_));
            break;
        }
    }

}}}


