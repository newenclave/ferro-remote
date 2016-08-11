
#ifndef FR_SUBSYS_netifaces_H
#define FR_SUBSYS_netifaces_H

#include "subsystem-iface.h"
#include <sys/types.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <memory>

#include "boost/asio/ip/address.hpp"

struct ifaddrs;

namespace fr { namespace agent {

    class application;

namespace subsys {

    class netifaces: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        netifaces( application *app );

    public:

        class iface_info {

            using address_type = boost::asio::ip::address;
            address_type    sockaddr_;
            address_type    mask_;
            std::string     name_;

        public:

            iface_info( const ifaddrs * );

            const std::string name( ) const
            {
                return name_;
            }

            const address_type &addr( ) const { return sockaddr_; }
            const address_type &mask( ) const { return mask_;     }

            bool is_v4( ) const { return sockaddr_.is_v4( ); }
            bool is_v6( ) const { return sockaddr_.is_v6( ); }

            bool check( const address_type &test ) const;

        };

        using iface_info_list = std::vector<iface_info>;
        using iface_info_list_ptr = std::shared_ptr<iface_info_list>;

        ~netifaces( );

        static vtrc::shared_ptr<netifaces> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        iface_info_list_ptr ifaces( ) const;
        iface_info_list enum_ifaces( );
    };

    std::ostream & operator << ( std::ostream &o,
                                 const netifaces::iface_info &info );

}}}

#endif

    
