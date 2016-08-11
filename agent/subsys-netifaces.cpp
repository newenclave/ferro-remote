
#include "application.h"
#include "subsys-netifaces.h"
#include "subsys-logging.h"

#include <ifaddrs.h>

//#include "vtrc-memory.h"


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
        namespace ba   = boost::asio;
        namespace bai  = boost::asio::ip;


        const std::string subsys_name( "netifaces" );


        bai::address from_sock_addr4( const sockaddr *sa )
        {
            auto si = reinterpret_cast<const sockaddr_in *>(sa);
            return bai::address_v4( ntohl(si->sin_addr.s_addr) );
        }

        bai::address from_sock_addr6( const sockaddr *sa )
        {
            auto si = reinterpret_cast<const sockaddr_in6 *>(sa);
            bai::address_v6::bytes_type bytes;
            std::copy( &si->sin6_addr.s6_addr[0],
                       &si->sin6_addr.s6_addr[bytes.max_size( )],
                       bytes.begin( ) );
            return bai::address_v6( bytes );
        }

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
        switch( ifa->ifa_addr->sa_family ) {
        case AF_INET:
            sockaddr_ = from_sock_addr4( ifa->ifa_addr );
            mask_     = from_sock_addr4( ifa->ifa_netmask );
            break;
        case AF_INET6:
            sockaddr_ = from_sock_addr6( ifa->ifa_addr );
            mask_     = from_sock_addr6( ifa->ifa_netmask );
            break;
        }
    }

    bool netifaces::iface_info::check( const address_type &test ) const
    {
        if( sockaddr_.is_v4( ) && test.is_v4( ) ) {
            auto sa = sockaddr_.to_v4( ).to_ulong( );
            auto ma = mask_.to_v4( ).to_ulong( );
            auto ta = test.to_v4( ).to_ulong( );
            return (sa & ma) == (ta & ma);
        } else if( sockaddr_.is_v6( ) && test.is_v6( ) ) {
            auto sa = sockaddr_.to_v6( ).to_bytes( );
            auto ma = mask_.to_v6( ).to_bytes( );
            auto ta = test.to_v6( ).to_bytes( );

            /// something wrong!
            static_assert( sa.max_size( ) == 16,
                           "bytes_type::max_size( ) != 16" );

            return ( sa[0x0] & ma[0x0]) == (ta[0x0] & ma[0x0] )
                && ( sa[0x1] & ma[0x1]) == (ta[0x1] & ma[0x1] )
                && ( sa[0x2] & ma[0x2]) == (ta[0x2] & ma[0x2] )
                && ( sa[0x3] & ma[0x3]) == (ta[0x3] & ma[0x3] )
                && ( sa[0x4] & ma[0x4]) == (ta[0x4] & ma[0x4] )
                && ( sa[0x5] & ma[0x5]) == (ta[0x5] & ma[0x5] )
                && ( sa[0x6] & ma[0x6]) == (ta[0x6] & ma[0x6] )
                && ( sa[0x7] & ma[0x7]) == (ta[0x7] & ma[0x7] )
                && ( sa[0x8] & ma[0x8]) == (ta[0x8] & ma[0x8] )
                && ( sa[0x9] & ma[0x9]) == (ta[0x9] & ma[0x9] )
                && ( sa[0xA] & ma[0xA]) == (ta[0xA] & ma[0xA] )
                && ( sa[0xB] & ma[0xB]) == (ta[0xB] & ma[0xB] )
                && ( sa[0xC] & ma[0xC]) == (ta[0xC] & ma[0xC] )
                && ( sa[0xD] & ma[0xD]) == (ta[0xD] & ma[0xD] )
                && ( sa[0xE] & ma[0xE]) == (ta[0xE] & ma[0xE] )
                && ( sa[0xF] & ma[0xF]) == (ta[0xF] & ma[0xF] )
                   ;
        }
        return false;
    }

}}}


