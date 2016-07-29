#ifndef FR_DISCOVER_H
#define FR_DISCOVER_H

#include "vtrc-function.h"
#include "vtrc-memory.h"

#include "boost/asio/ip/udp.hpp"

namespace boost { namespace system {
    class error_code;
}}

namespace boost { namespace asio {
    class io_service;
}}

namespace fr {  namespace client { namespace core {

    struct udp_responce_info {
        boost::asio::ip::udp::endpoint  *from;
        const char                      *data;
        size_t                           length;
        udp_responce_info(  )
            :from(NULL)
            ,data(NULL)
            ,length(0)
        { }
    };

    struct udp_pinger: vtrc::enable_shared_from_this<udp_pinger> {
        typedef vtrc::function<bool( const boost::system::error_code &,
                                     const udp_responce_info * )> handler_type;
        virtual void stop( ) = 0;
        virtual void async_ping( const std::string &, int, handler_type) = 0;
        virtual void async_ping( const std::string &, int, handler_type,
                                 const std::string &local_bind ) = 0;
    };

    typedef vtrc::shared_ptr<udp_pinger> udp_pinger_sptr;

    udp_pinger_sptr create_udp_pinger( boost::asio::io_service &ios );

}}}

#endif // FRDISCOVER_H
