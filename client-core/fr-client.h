#ifndef FR_CLIENT_H
#define FR_CLIENT_H

#include <string>

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-function.h"
#include "vtrc-common/vtrc-closure.h"

namespace boost { namespace system {
    class error_code;
}}

namespace fr {  namespace client { namespace core {

    class client {

        struct  impl;
        impl   *impl_;

    public:

        client( vtrc::common::pool_pair &pp );
        ~client(  );

    public:

        /// format is:
        ///  /path/with/file_name for UNIX socket
        ///  server:port          for tcp
        void connect( const std::string &server );
        void connect( const std::string &server, bool tcp_nowait );

        ///
        /// here is:
        /// vtrc::common::system_closure_type ==
        ///         void func( const boost::system::error_code &err )
        ///
        typedef vtrc::common::system_closure_type async_closure_func;

        void async_connect( const std::string &server, async_closure_func f );
        void async_connect( const std::string &server,
                            bool tcp_nowait, async_closure_func f );

        void reconnect( );
        void async_reconnect( async_closure_func f );
        void disconnect( );
    };

}}}

#endif // FRCLIENT_H
