#ifndef FR_DISCOVER_H
#define FR_DISCOVER_H

#include "vtrc-function.h"
#include "vtrc-memory.h"

namespace boost { namespace system {
    class error_code;
}}

namespace boost { namespace asio {
    class io_service;
}}

namespace fr {  namespace client { namespace core {

    class discover {
        struct impl;
        impl  *impl_;
    public:

        typedef boost::system::error_code error_code;

        typedef vtrc::function<bool( const error_code &,
                                     const char *, size_t )> handler_type;

        struct pinger: vtrc::enable_shared_from_this<pinger> {
            virtual void stop( ) { }
        };

        typedef vtrc::shared_ptr<pinger> pinger_sptr;

        discover( boost::asio::io_service &ios );
        ~discover(  );

        pinger_sptr add_ping( const std::string &addr, int timeout,
                                  handler_type hdlr );
        pinger_sptr add_mcast( const std::string &addr, int timeout,
                                   handler_type hdlr );
        pinger_sptr add_mcast( const std::string &addr, int timeout,
                                   handler_type hdlr,
                                   const std::string &local_bind );

    };

}}}

#endif // FRDISCOVER_H
