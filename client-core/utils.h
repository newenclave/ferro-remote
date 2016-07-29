#ifndef TA_UTILS_H
#define TA_UTILS_H

#include <string>
#include <cstdint>
#include <ostream>

#include "common/result.hpp"

namespace fr { namespace utilities {

    template <typename T>
    struct type_uid {
        typedef T type;
        static std::uintptr_t uid( )
        {
            static const char i = '!';
            return reinterpret_cast<std::uintptr_t>(&i);
        }
    };

    using h2b_result = result<std::string, const char *>;

    h2b_result bin2hex( void const *bytes, size_t length );
    h2b_result bin2hex( std::string const &input );
    h2b_result hex2bin( std::string const &input );

    namespace console {
        std::ostream &light ( std::ostream &s );
        std::ostream &red   ( std::ostream &s );
        std::ostream &green ( std::ostream &s );
        std::ostream &blue  ( std::ostream &s );
        std::ostream &cyan  ( std::ostream &s );
        std::ostream &yellow( std::ostream &s );
        std::ostream &none  ( std::ostream &s );
    }

    struct endpoint_info {
        enum ep_type {
             ENDPOINT_NONE   = 0
            ,ENDPOINT_LOCAL  = 1
            ,ENDPOINT_IP     = 2
            ,ENDPOINT_UDP    = 2 // hm
            ,ENDPOINT_TCP    = 2 //
        };

        enum ep_flags {
             FLAG_SSL   = 0x01
            ,FLAG_DUMMY = 0x01 << 1
        };

        std::string    addpess;
        std::uint16_t  service = 0;
        unsigned       flags   = 0;
        ep_type        type    = ENDPOINT_NONE;

        bool is_local( ) const NOEXCEPT
        {
            return type == ENDPOINT_LOCAL;
        }

        bool is_ip( ) const NOEXCEPT
        {
            return type == ENDPOINT_IP;
        }

        bool is_none( ) const NOEXCEPT
        {
            return type == ENDPOINT_NONE;
        }

        bool is_ssl( ) const NOEXCEPT
        {
            return !!(flags & FLAG_SSL);
        }

        bool is_dummy( ) const NOEXCEPT
        {
            return !!(flags & FLAG_DUMMY);
        }

        operator bool( ) const NOEXCEPT
        {
            return !is_none( );
        }

    };

    std::ostream & operator << ( std::ostream &os, const endpoint_info &ei );

    ///  0.0.0.0:12345             - tcp  endpoint (address:port)
    ///  0.0.0.0:12345             - tcp6 endpoint (address:port)
    /// @0.0.0.0:12345             - tcp  + ssl endpoint( !address:port )
    /// @:::12345                  - tcp6 + ssl endpoint( !address:port )
    ///  /home/data/teranyina.sock - local endpoint
    ///                                (/local/socket or \\.\pipe\name )
    endpoint_info get_endpoint_info( const std::string &ep );

}}


#endif // UTILS_H
