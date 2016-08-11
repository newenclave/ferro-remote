
#ifndef FR_SUBSYS_netifaces_H
#define FR_SUBSYS_netifaces_H

#include "subsystem-iface.h"
#include <sys/types.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <memory>

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

            sockaddr_storage sockaddr_;
            sockaddr_storage mask_;
            std::string      name_;

        public:

            iface_info( const ifaddrs * );

            const std::string name( ) const
            {
                return name_;
            }

            const sockaddr_in *in4( ) const
            {
                return reinterpret_cast<const sockaddr_in *>(&sockaddr_);
            }

            const sockaddr_in6 *in6( ) const
            {
                return reinterpret_cast<const sockaddr_in6 *>(&sockaddr_);
            }

            const sockaddr_in *mask4( ) const
            {
                return reinterpret_cast<const sockaddr_in *>(&mask_);
            }

            const sockaddr_in6 *mask6( ) const
            {
                return reinterpret_cast<const sockaddr_in6 *>(&mask_);
            }

            bool is4( ) const { return sockaddr_.ss_family == AF_INET; }
            bool is6( ) const { return sockaddr_.ss_family == AF_INET6; }

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

}}}

#endif

    
