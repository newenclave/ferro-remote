#ifndef NET_IFACES_H
#define NET_IFACES_H

#include <vector>
#include "boost/asio/ip/address.hpp"

struct sockaddr;

namespace utilities {

    class iface_info {

        using address_type = boost::asio::ip::address;
        address_type         sockaddr_;
        address_type         mask_;
        std::string          name_;
        size_t               id_;

    public:

        iface_info( const sockaddr *sa, const sockaddr *mask,
                    const std::string &name, size_t id );

        const std::string name( ) const
        {
            return name_;
        }

        const address_type &addr( ) const
        {
            return sockaddr_;
        }

        const address_type &mask( ) const
        {
            return mask_;
        }

        bool is_v4( ) const
        {
            return sockaddr_.is_v4( );
        }

        bool is_v6( ) const
        {
            return sockaddr_.is_v6( );
        }

        size_t id( ) const
        {
            return id_;
        }

        bool check( const address_type &test ) const;
    };

    typedef std::vector<iface_info> iface_info_list;
    iface_info_list get_system_ifaces( );
    std::ostream & operator << ( std::ostream &o, const iface_info &info );

}


#endif // NETIFACES_H
